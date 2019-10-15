#ifndef KCPTUN_SERVER_H
#define KCPTUN_SERVER_H

#include "config.h"
#include "sess.h"

class smux;
class smux_sess;

using AcceptHandler = std::function<void(std::shared_ptr<smux_sess>)>;

class Server final : public std::enable_shared_from_this<Server>,
                     public AsyncInOutputer,
                     public kvar_,
                     public Destroy {
public:
    Server(asio::io_service &io_service, OutputHandler handler);
    ~Server() override;
    void run(AcceptHandler handler, uint32_t convid);
    void async_input(char *buf, std::size_t len, Handler handler) override;

private:
    void do_sess_receive();
    void call_this_on_destroy() override;

private:
    char sbuf_[2048];
    asio::io_service &service_;
    std::shared_ptr<Session> sess_;
    std::shared_ptr<smux> smux_;
    OutputHandler in;
    OutputHandler out;
    OutputHandler in2;
    OutputHandler out2;
};

#endif
