#include "local.h"

Local::Local(asio::io_service &io_service, asio::ip::udp::endpoint ep)
    : service_(io_service),
      usocket_(std::make_shared<asio::ip::udp::socket>(io_service, asio::ip::udp::endpoint(asio::ip::udp::v4(), 0))),
      sess_(std::make_shared<Session>(io_service, usocket_, ep, uint32_t(rand()))) {}

void Local::run() {
    sess_->run();
    do_receive();
}

void Local::do_receive() {
    auto self = shared_from_this();
    usocket_->async_receive(asio::buffer(buf_, sizeof(buf_)),
                            [this, self](std::error_code ec, std::size_t len) {
                                if (ec) {
                                    return;
                                }
                                sess_->input(buf_, len);
                                do_receive();
                            });
}

void Local::async_read_some(char *buffer, std::size_t len, Handler handler) {
    sess_->async_read_some(buffer, len, handler);
}

void Local::async_write(char *buffer, std::size_t len, Handler handler) {
    sess_->async_write_some(buffer, len, handler);
}
