#ifndef KCPTUN_KCPTUN_SERVER_H
#define KCPTUN_KCPTUN_SERVER_H

#include "config.h"
#include "server.h"

class kcptun_server_session final
    : public std::enable_shared_from_this<kcptun_server_session> {
public:
    kcptun_server_session(asio::io_service &io_service,
                          std::shared_ptr<smux_sess> sess,
                          asio::ip::tcp::endpoint target_endpoint);
    void run();

private:
    void do_pipe1();
    void do_pipe2();
    void destroy();

private:
    char buf1_[4096];
    char buf2_[4096];
    asio::io_service &service_;
    std::shared_ptr<smux_sess> sess_;
    asio::ip::tcp::socket socket_;
    asio::ip::tcp::endpoint target_endpoint_;
};

class kcptun_server final : public std::enable_shared_from_this<kcptun_server> {
public:
    kcptun_server(asio::io_service &io_service,
                  asio::ip::udp::endpoint local_endpoint,
                  asio::ip::tcp::endpoint target_point);
    void run();
    void do_receive();

private:
    void accept_handler(std::shared_ptr<smux_sess> sess);

private:
    bool isfec_;
    char buf_[2048];
    asio::io_service &service_;
    asio::ip::udp::socket usocket_;
    asio::ip::udp::endpoint ep_;
    asio::ip::tcp::endpoint target_endpoint_;
    std::unique_ptr<BaseDecEncrypter> dec_or_enc_;
    std::map<asio::ip::udp::endpoint, std::shared_ptr<Server>> servers_;
    Buffers buffers_;
};

#endif
