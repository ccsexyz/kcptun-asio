#ifndef KCPTUN_LOCAL_H
#define KCPTUN_LOCAL_H

#include "config.h"
#include "sess.h"

class smux_sess;
class smux;

class Local final :
        public std::enable_shared_from_this<Local>,
        public kvar_,
        public Destroy {
public:
    Local(asio::io_service &io_service, asio::ip::udp::endpoint ep);
    void run();
    void async_connect(std::function<void(std::shared_ptr<smux_sess>)> handler);
    void run_scavenger();

private: 
    void do_usocket_receive();
    void do_sess_receive();
    void call_this_on_destroy() override;

private:
    char buf_[2048];
    char sbuf_[2048];
    asio::io_service &service_;
    asio::ip::udp::endpoint ep_;
    std::shared_ptr<Session> sess_;
    std::shared_ptr<smux> smux_;
    std::shared_ptr<AsyncReadWriter> usock_;
    OutputHandler in;
    OutputHandler out;
    OutputHandler in2;
    OutputHandler out2;
    Buffers buffers_;
};

#endif
