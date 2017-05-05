#include "sess.h"
#include "encrypt.h"

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
    ikcp_nodelay(kcp_, 1, 20, 2, 1);
    ikcp_wndsize(kcp_, 1024, 1024);
    dec_or_enc_ = getDecEncrypter(global_config.crypt, global_config.key);
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
    auto timer = std::make_shared<asio::deadline_timer>(service_, boost::posix_time::seconds(1));
    timer->async_wait([this, self, timer](const std::error_code &){
        run_peeksize_checker();
    });
}

void Session::input(char *buffer, std::size_t len) {
    dec_or_enc_->decrypt(buffer, len, buffer, len);
    auto n = ikcp_input(kcp_, buffer + nonce_size + crc_size,
                        int(len - (nonce_size + crc_size)));
    TRACE
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
    char *buf = static_cast<char *>(malloc(len + nonce_size + crc_size));
    memcpy(buf + nonce_size + crc_size, buffer, len);
    auto crc = crc32c_ieee(0, (byte *) buffer, len);
    encode32u((byte *)(buf + nonce_size), crc);
    dec_or_enc_->encrypt(buf, len + nonce_size + crc_size, buf, len + nonce_size + crc_size);
    usocket_->async_send_to(
        asio::buffer(buf, len + nonce_size + crc_size), ep_,
        [buf](std::error_code ec, std::size_t len) { free(buf); });
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
