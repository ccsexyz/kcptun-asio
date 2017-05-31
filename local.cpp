#include "local.h"
#include "async_fec.h"
#include "sess.h"
#include "smux.h"
#include "snappy_stream.h"

Local::Local(asio::io_service &io_service, asio::ip::udp::endpoint ep)
    : service_(io_service), ep_(ep),
      usock_(std::make_shared<UsocketReadWriter>(
          asio::ip::udp::socket(
              io_service, asio::ip::udp::endpoint(asio::ip::udp::v4(), 0)),
          ep)) {}

void Local::run() {
    auto self = shared_from_this();
    auto fec = DataShard > 0 && ParityShard > 0;

    in = [this](char *buf, std::size_t len, Handler handler) {
        sess_->async_input(buf, len, handler);
    };
    if (fec) {
        auto fec_in = std::make_shared<AsyncFECInputer>(in);
        in = [this, fec_in](char *buf, std::size_t len, Handler handler) {
            fec_in->async_input(buf, len, handler);
        };
    }
    auto dec = getAsyncDecrypter(
        getDecEncrypter(Crypt, pbkdf2(Key)),
        [inb(in)](char *buf, std::size_t len, Handler handler) {
            auto n = nonce_size + crc_size;
            buf += n;
            len -= n;
            inb(buf, len, handler);
        });
    in = [this, dec](char *buf, std::size_t len, Handler handler) {
        dec->async_input(buf, len, handler);
    };

    out = [this](char *buf, std::size_t len, Handler handler) {
        usock_->async_write(buf, len, handler);
    };
    auto enc = getAsyncEncrypter(getDecEncrypter(Crypt, pbkdf2(Key)), out);
    out = [this, enc](char *buf, std::size_t len, Handler handler) {
        char *buffer = buffers_.get();
        auto n = nonce_size + crc_size;
        memcpy(buffer + n, buf, len);
        // info("capacity: %lu size: %lu\n", buffers_.capacity(), buffers_.size());
        auto crc = crc32c_ieee(0, (byte *)buf, len);
        encode32u((byte *)(buffer + nonce_size), crc);
        enc->async_input(buffer, len + n, [handler, buffer, len, this](
                                              std::error_code ec, std::size_t) {
            buffers_.push_back(buffer);
            // info("capacity: %lu size: %lu\n", buffers_.capacity(), buffers_.size());
            if (handler) {
                handler(ec, len);
            }
        });
    };
    if (fec) {
        auto fec_out = std::make_shared<AsyncFECOutputer>(out);
        out = [this, fec_out](char *buf, std::size_t len, Handler handler) {
            fec_out->async_input(buf, len, handler);
        };
    }
    sess_ = std::make_shared<Session>(service_, uint32_t(rand()), out);
    sess_->run();

    out2 = [this](char *buf, std::size_t len, Handler handler) {
        sess_->async_write(buf, len, handler);
    };
    if (!NoComp) {
        auto snappy_writer =
            std::make_shared<snappy_stream_writer>(service_, out2);
        out2 = [this, snappy_writer](char *buf, std::size_t len,
                                     Handler handler) {
            snappy_writer->async_input(buf, len, handler);
        };
    }
    smux_ = std::make_shared<smux>(service_, out2);
    smux_->run();

    in2 = [this](char *buf, std::size_t len, Handler handler) {
        smux_->async_input(buf, len, handler);
    };
    if (!NoComp) {
        auto snappy_reader =
            std::make_shared<snappy_stream_reader>(service_, in2);
        in2 = [this, snappy_reader](char *buf, std::size_t len,
                                    Handler handler) {
            snappy_reader->async_input(buf, len, handler);
        };
    }

    do_usocket_receive();
    do_sess_receive();
}

void Local::do_usocket_receive() {
    auto self = shared_from_this();
    usock_->async_read_some(
        buf_, sizeof(buf_), [this, self](std::error_code ec, std::size_t sz) {
            if (ec) {
                return;
            }
            in(buf_, sz, [this, self](std::error_code ec, std::size_t) {
                if (ec) {
                    return;
                }
                do_usocket_receive();
            });
        });
}

void Local::do_sess_receive() {
    auto self = shared_from_this();
    sess_->async_read_some(
        sbuf_, sizeof(sbuf_), [this, self](std::error_code ec, std::size_t sz) {
            if (ec) {
                return;
            }
            in2(sbuf_, sz, [this, self](std::error_code ec, std::size_t) {
                if (ec) {
                    return;
                }
                do_sess_receive();
            });
        });
}

void Local::async_connect(
    std::function<void(std::shared_ptr<smux_sess>)> handler) {
    smux_->async_connect(handler);
}

bool Local::is_destroyed() const { return smux_->is_destroyed(); }

void Local::run_scavenger() {
    if (ScavengeTTL <= 0) {
        return;
    }
    auto self = shared_from_this();
    auto timer = std::make_shared<asio::deadline_timer>(
        service_, boost::posix_time::seconds(ScavengeTTL));
    timer->async_wait([this, timer, self](const std::error_code &) {
        if (smux_) {
            smux_->destroy();
        }
    });
}