#include "sess.h"
#include "encrypt.h"
#include "fec.h"

Session::Session(asio::io_service &service,
                 std::shared_ptr<asio::ip::udp::socket> usocket,
                 asio::ip::udp::endpoint ep, uint32_t convid)
    : service_(service), usocket_(usocket), ep_(ep), convid_(convid) {}

Session::~Session() {
    TRACE
    if (kcp_ != nullptr) {
        ikcp_release(kcp_);
    }
}

void Session::run() {
    kcp_ = ikcp_create(convid_, static_cast<void *>(this));
    kcp_->output = Session::output_wrapper;
    kcp_->stream = 1;
    ikcp_nodelay(kcp_, NoDelay, Interval, Resend, Nc);
    ikcp_wndsize(kcp_, SndWnd, RcvWnd);
    ikcp_setmtu(kcp_, Mtu);
    dec_or_enc_ = getDecEncrypter(Crypt, pbkdf2(Key));
    if (DataShard > 0 && ParityShard > 0) {
        fec_ = std::make_unique<FEC>(
            FEC::New(3 * (DataShard + ParityShard),
                     DataShard, ParityShard));
        shards_.resize(DataShard + ParityShard,
                       nullptr);
    }
    run_timer();
    //    run_peeksize_checker();
}

void Session::run_timer() {
    auto self = shared_from_this();
    std::weak_ptr<Session> ws = self;
    if (!timer_) {
        timer_ = std::make_shared<asio::deadline_timer>(
            service_, boost::posix_time::milliseconds(10));
    } else {
        timer_->expires_at(timer_->expires_at() +
                           boost::posix_time::milliseconds(10));
    }
    timer_->async_wait([this, ws](const std::error_code &) {
        auto s = ws.lock();
        if (!s) {
            return;
        }
        update();
        run_timer();
    });
}

void Session::run_peeksize_checker() {
    auto self = shared_from_this();
    auto timer = std::make_shared<asio::deadline_timer>(
        service_, boost::posix_time::seconds(1));
    timer->async_wait([this, self, timer](const std::error_code &) {
        run_peeksize_checker();
    });
}

void Session::input(char *buffer, std::size_t len) {
    dec_or_enc_->decrypt(buffer, len, buffer, len);
    if (len <= nonce_size + crc_size) {
        return;
    }
    buffer += nonce_size + crc_size;
    len -= nonce_size + crc_size;
    if (!fec_) {
        auto n = ikcp_input(kcp_, buffer, int(len));
        TRACE
        if (rtask_.check()) {
            update();
        }
        return;
    }
    auto pkt = fec_->Decode((byte *)buffer, len);
    if (pkt.flag == typeData) {
        auto ptr = pkt.data->data();
        ikcp_input(kcp_, (char *)(ptr + 2), pkt.data->size() - 2);
    }
    if (pkt.flag == typeData || pkt.flag == typeFEC) {
        auto recovered = fec_->Input(pkt);
        for (auto &r : recovered) {
            if (r->size() > 2) {
                auto ptr = r->data();
                uint16_t sz;
                decode16u(ptr, &sz);
                if (sz >= 2 && sz <= r->size()) {
                    ikcp_input(kcp_, (char *)(ptr + 2), sz - 2);
                }
            }
        }
    }
    if (rtask_.check()) {
        update();
    }
}

void Session::async_read_some(char *buffer, std::size_t len, Handler handler) {
    if (streambufsiz_ > 0) {
        auto n = streambufsiz_;
        if (n > len) {
            n = len;
        }
        memcpy(buffer, stream_buf_, n);
        streambufsiz_ -= n;
        if (streambufsiz_ != 0) {
            memmove(stream_buf_, stream_buf_ + n, streambufsiz_);
        }
        if (handler) {
            handler(std::error_code(0, std::generic_category()),
                    static_cast<std::size_t>(n));
        }
        return;
    }
    auto psz = ikcp_peeksize(kcp_);
    if (psz <= 0) {
        rtask_.buf = buffer;
        rtask_.len = len;
        rtask_.handler = handler;
        return;
    }
    auto n = ikcp_recv(kcp_, buffer, int(len));
    if (handler) {
        handler(std::error_code(0, std::generic_category()),
                static_cast<std::size_t>(n));
    }
    if (psz > len) {
        n = ikcp_recv(kcp_, stream_buf_, sizeof(stream_buf_));
        streambufsiz_ = n;
    }
    return;
}

void Session::async_write_some(char *buffer, std::size_t len, Handler handler) {
    auto n = ikcp_send(kcp_, buffer, int(len));
    if (handler) {
        handler(std::error_code(0, std::generic_category()),
                static_cast<std::size_t>(n));
    }
}

int Session::output_wrapper(const char *buffer, int len, struct IKCPCB *kcp,
                            void *user) {
    assert(user != nullptr);
    Session *sess = static_cast<Session *>(user);
    sess->output(buffer, static_cast<std::size_t>(len));
    return 0;
}

ssize_t Session::output(const char *buffer, std::size_t len) {
    if (!fec_) {
        return output_no_fec(buffer, len);
    }
    memcpy(buf_ + fecHeaderSizePlus2, buffer, len);
    fec_->MarkData((byte *)buf_, len + fecHeaderSizePlus2);
    output_no_fec((const char *)buf_, len + fecHeaderSizePlus2);
    auto slen = len + 2;
    shards_[pkt_idx_] = std::make_shared<std::vector<byte>>(
        &(buf_[fecHeaderSize]), &(buf_[fecHeaderSize + slen]));
    pkt_idx_++;
    if (pkt_idx_ == DataShard) {
        fec_->Encode(shards_);
        for (size_t i = DataShard;
             i < DataShard + ParityShard; i++) {
            memcpy(buf_ + fecHeaderSize, shards_[i]->data(),
                   shards_[i]->size());
            fec_->MarkFEC(buf_);
            output_no_fec((const char *)buf_,
                          shards_[i]->size() + fecHeaderSize);
        }
        pkt_idx_ = 0;
    }
    return len;
}

void Session::update() {
    ikcp_update(kcp_, iclock());
    if (!rtask_.check()) {
        return;
    }
    auto psz = ikcp_peeksize(kcp_);
    if (psz <= 0) {
        return;
    }
    auto n = ikcp_recv(kcp_, rtask_.buf, int(rtask_.len));
    auto rtask_handler = rtask_.handler;
    rtask_.reset();
    if (rtask_handler) {
        rtask_handler(std::error_code(0, std::generic_category()),
                      static_cast<std::size_t>(n));
    }
}

ssize_t Session::output_no_fec(const char *buffer, std::size_t len) {
    char *buf = static_cast<char *>(malloc(len + nonce_size + crc_size));
    memcpy(buf + nonce_size + crc_size, buffer, len);
    auto crc = crc32c_ieee(0, (byte *)buffer, len);
    encode32u((byte *)(buf + nonce_size), crc);
    dec_or_enc_->encrypt(buf, len + nonce_size + crc_size, buf,
                         len + nonce_size + crc_size);
    usocket_->async_send_to(
        asio::buffer(buf, len + nonce_size + crc_size), ep_,
        [buf](std::error_code ec, std::size_t len) { free(buf); });
    return len;
}
