//
// Created by ccsexyz on 17-5-3.
//

#ifndef KCPTUN_LOCAL_H
#define KCPTUN_LOCAL_H

#include "config.h"
#include "sess.h"

class Local final : public std::enable_shared_from_this<Local> {
public:
    Local(asio::io_service &io_service, asio::ip::udp::endpoint ep);
    void run();
    void async_read_some(char *buffer, std::size_t len, Handler handler);
    void async_write(char *buffer, std::size_t len, Handler handler);

private:
    void do_receive();

private:
    char buf_[65536];
    asio::io_service &service_;
    std::shared_ptr<asio::ip::udp::socket> usocket_;
    std::shared_ptr<Session> sess_;
};

#endif
