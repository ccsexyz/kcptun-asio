#include "kcptun_server.h"
#include "fec.h"
#include "server.h"
#include "sess.h"
#include "smux.h"

kcptun_server::kcptun_server(asio::io_service &io_service,
                             asio::ip::udp::endpoint local_endpoint,
                             asio::ip::tcp::endpoint target_endpoint)
    : service_(io_service), target_endpoint_(target_endpoint),
      usocket_(io_service, local_endpoint) {}

void kcptun_server::run() {
    isfec_ = DataShard > 0 && ParityShard > 0;
    dec_or_enc_ = getDecEncrypter(Crypt, pbkdf2(Key));
    do_receive();
}

void kcptun_server::accept_handler(std::shared_ptr<smux_sess> sess) {
    std::make_shared<kcptun_server_session>(service_, sess, target_endpoint_)
        ->run();
}

void kcptun_server::do_receive() {
    auto self = shared_from_this();
    usocket_.async_receive_from(
        asio::buffer(buf_, sizeof(buf_)), ep_,
        [this, self](std::error_code ec, std::size_t len) {
            if (ec) {
                return;
            }
            if (len <= nonce_size + crc_size) {
                do_receive();
                return;
            }
            dec_or_enc_->decrypt(buf_, len, buf_, len);
            char *buf = buf_ + (nonce_size + crc_size);
            len -= nonce_size + crc_size;
            auto it = servers_.find(ep_);
            std::shared_ptr<Server> server;
            if (it != servers_.end()) {
                server = it->second;
            } else {
                uint32_t convid;
                if (isfec_) {
                    uint16_t fec_type;
                    decode16u((byte *)(buf + 4), &fec_type);
                    if (fec_type != typeData) {
                        do_receive();
                        return;
                    }
                    decode32u((byte *)(buf + fecHeaderSizePlus2), &convid);
                } else {
                    decode32u((byte *)buf, &convid);
                }
                asio::ip::udp::endpoint ep = ep_;
                server = std::make_shared<Server>(
                    service_, [this, self, ep](char *buf, std::size_t len,
                                               Handler handler) {
                        char *buffer = buffers_.get();
                        memcpy(buffer + nonce_size + crc_size, buf, len);
                        auto crc = crc32c_ieee(0, (byte *)buf, len);
                        encode32u((byte *)(buffer + nonce_size), crc);
                        dec_or_enc_->encrypt(
                            buffer, len + nonce_size + crc_size, buffer,
                            len + nonce_size + crc_size);
                        usocket_.async_send_to(
                            asio::buffer(buffer, len + nonce_size + crc_size),
                            ep, [handler, this, self, len,
                                 buffer](std::error_code ec, std::size_t) {
                                buffers_.push_back(buffer);
                                if (handler) {
                                    handler(ec, len);
                                }
                            });
                    });
                server->run(
                    [this, self](std::shared_ptr<smux_sess> sess) {
                        accept_handler(sess);
                    },
                    convid);
                servers_.emplace(ep, server);
            }
            server->async_input(
                buf, len,
                [this, self](std::error_code, std::size_t) { do_receive(); });
        });
}

kcptun_server_session::kcptun_server_session(
    asio::io_service &io_service, std::shared_ptr<smux_sess> sess,
    asio::ip::tcp::endpoint target_endpoint)
    : service_(io_service), sess_(sess), socket_(io_service),
      target_endpoint_(target_endpoint) {}

void kcptun_server_session::run() {
    auto self = shared_from_this();
    socket_.async_connect(target_endpoint_, [this, self](std::error_code ec) {
        if (ec) {
            return;
        }
        do_pipe1();
        do_pipe2();
    });
}

void kcptun_server_session::destroy() {
    socket_.close();
    if(sess_) {
        sess_->destroy();
    }
}

void kcptun_server_session::do_pipe1() {
    auto self = shared_from_this();
    socket_.async_read_some(
        asio::buffer(buf1_, sizeof(buf1_)),
        [this, self](std::error_code ec, std::size_t len) {
            if (ec) {
                destroy();
                return;
            }
            sess_->async_write(
                buf1_, len, [this, self](std::error_code ec, std::size_t len) {
                    if (ec) {
                        destroy();
                        return;
                    }
                    do_pipe1();
                });
        });
}

void kcptun_server_session::do_pipe2() {
    auto self = shared_from_this();
    sess_->async_read_some(buf2_, sizeof(buf2_), [this,
                                                  self](std::error_code ec,
                                                        std::size_t len) {
        if (ec) {
            destroy();
            return;
        }
        asio::async_write(socket_, asio::buffer(buf2_, len),
                          [this, self](std::error_code ec, std::size_t len) {
                              if (ec) {
                                  destroy();
                                  return;
                              }
                              do_pipe2();
                          });
    });
}
