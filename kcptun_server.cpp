#include "kcptun_server.h"


kcptun_server::kcptun_server(asio::io_service &io_service, asio::ip::udp::endpoint local_endpoint, asio::ip::tcp::endpoint target_endpoint)
    : service_(io_service), target_endpoint_(target_endpoint), server_(std::make_shared<Server>(io_service, local_endpoint)) {}

void kcptun_server::run() {
    auto self = shared_from_this();
    server_->run([this, self](std::shared_ptr<Session> sess){
        accept_handler(sess);
    });
}

void kcptun_server::accept_handler(std::shared_ptr<Session> sess) {
    std::make_shared<kcptun_server_session>(service_, sess, target_endpoint_)->run();
}

kcptun_server_session::kcptun_server_session(asio::io_service &io_service, std::shared_ptr<Session> sess, asio::ip::tcp::endpoint target_endpoint)
    : service_(io_service), sess_(sess), socket_(io_service), target_endpoint_(target_endpoint) {}

void kcptun_server_session::run() {
    auto self = shared_from_this();
    socket_.async_connect(target_endpoint_, [this, self](std::error_code ec){
        if(ec){
            return;
        }
        do_pipe1();
        do_pipe2();
    });
}

void kcptun_server_session::do_pipe1() {
    auto self = shared_from_this();
    socket_.async_read_some(asio::buffer(buf1_, sizeof(buf1_)),  [this, self](std::error_code ec, std::size_t len){
        if(ec) {
            return;
        }
        sess_->async_write_some(buf1_, len, [this, self](std::error_code ec, std::size_t len){
            if(ec) {
                socket_.cancel();
                return;
            }
            do_pipe1();
        });
    });
}

void kcptun_server_session::do_pipe2() {
    auto self = shared_from_this();
    sess_->async_read_some(buf2_, sizeof(buf2_), [this, self](std::error_code ec, std::size_t len){
        if(ec) {
            socket_.cancel();
            return;
        }
        asio::async_write(socket_, asio::buffer(buf2_, len), [this, self](std::error_code ec, std::size_t len){
            if(ec) {
                return;
            }
            do_pipe2();
        });
    });
}
