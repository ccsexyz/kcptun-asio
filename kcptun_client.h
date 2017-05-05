#ifndef KCPTUN_KCPTUN_CLIENT_H
#define KCPTUN_KCPTUN_CLIENT_H

#include "local.h"
#include "smux.h"
#include "config.h"
#include "encrypt.h"

class snappy_stream_writer;
class snappy_stream_reader;

class kcptun_client_session final
    : public std::enable_shared_from_this<kcptun_client_session> {
public:
    kcptun_client_session(asio::io_service &io_service,
                          std::shared_ptr<asio::ip::tcp::socket> sock,
                          std::shared_ptr<smux_sess> sess);
    void run();

private:
    void do_pipe1();
    void do_pipe2();

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
    void do_receive();
    void do_accept();
    void try_write_task();
    void output_handler(char *buf, std::size_t len, Handler handler);
    void snappy_stream_reader_output_handler(char *buf, std::size_t len, Handler handler);
    void snappy_stream_writer_output_handler(char *buf, std::size_t len, Handler handler);

private:
    char buf_[65536];
    asio::io_service &service_;
    asio::ip::tcp::socket socket_;
    asio::ip::udp::endpoint target_endpoint_;
    asio::ip::tcp::acceptor acceptor_;
    std::shared_ptr<Local> local_;
    std::shared_ptr<smux> smux_;
    std::deque<Task> tasks_;
    bool writing_ = false;
    std::shared_ptr<snappy_stream_writer> snappy_writer_;
    std::shared_ptr<snappy_stream_reader> snappy_reader_;
};

#endif
