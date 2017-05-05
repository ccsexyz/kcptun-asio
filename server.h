//
// Created by ccsexyz on 17-5-3.
//

#ifndef KCPTUN_SERVER_H
#define KCPTUN_SERVER_H

#include "config.h"
#include "sess.h"

using AcceptHandler = std::function<void(std::shared_ptr<Session>)>;

class Server final : public std::enable_shared_from_this<Server> {
public:
    Server(asio::io_service &io_service, asio::ip::udp::endpoint ep);
    void run(AcceptHandler handler);

private:
    void do_receive();

private:
    AcceptHandler acceptHandler_;

private:
    char buf_[65536];
    asio::io_service &service_;
    asio::ip::udp::endpoint ep_;
    std::shared_ptr<asio::ip::udp::socket> usocket_;
    std::map<asio::ip::udp::endpoint, std::shared_ptr<Session>> sessions_;
};

#endif
