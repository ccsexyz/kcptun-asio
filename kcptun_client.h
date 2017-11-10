#ifndef KCPTUN_KCPTUN_CLIENT_H
#define KCPTUN_KCPTUN_CLIENT_H

#include "config.h"
#include "encrypt.h"
#include "local.h"
#include "smux.h"

class snappy_stream_writer;
class snappy_stream_reader;

class kcptun_client_session final
    : public std::enable_shared_from_this<kcptun_client_session>,
      public kvar_,
      public Destroy {
public:
    kcptun_client_session(asio::io_service &io_service,
                          std::shared_ptr<asio::ip::tcp::socket> sock,
                          std::shared_ptr<smux_sess> sess);
    ~kcptun_client_session();
    void run();

private:
    void do_pipe1();
    void do_pipe2();
    void call_this_on_destroy() override;

private:
    char buf1_[4096];
    char buf2_[4096];
    asio::io_service &service_;
    std::shared_ptr<asio::ip::tcp::socket> sock_;
    std::shared_ptr<smux_sess> sess_;
};
class kcptun_client final : public std::enable_shared_from_this<kcptun_client> {
public:
    kcptun_client(asio::io_service &io_service,
                  asio::ip::tcp::endpoint local_endpoint,
                  asio::ip::udp::endpoint target_endpoint);
    void run();

private:
    void do_accept();
    void async_choose_local(std::function<void(std::shared_ptr<Local>)> f);

private:
    asio::io_service &service_;
    asio::ip::tcp::socket socket_;
    asio::ip::udp::endpoint target_endpoint_;
    asio::ip::tcp::acceptor acceptor_;
    std::vector<std::weak_ptr<Local>> locals_;
};

#endif
