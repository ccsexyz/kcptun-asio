//
// Created by ccsexyz on 17-5-3.
//

#ifndef KCPTUN_SESS_H
#define KCPTUN_SESS_H

#include "config.h"
#include "encrypt.h"
#include "ikcp.h"
#include "utils.h"

class Session : public std::enable_shared_from_this<Session> {
public:
    Session(asio::io_service &service,
            std::shared_ptr<asio::ip::udp::socket> usocket,
            asio::ip::udp::endpoint ep, uint32_t convid);
    void run();
    ~Session();

private:
    void run_timer();
    static int output_wrapper(const char *buffer, int len, struct IKCPCB *kcp,
                              void *user);
    ssize_t output(const char *buffer, std::size_t len);
    void update();
    void run_peeksize_checker();

public:
    void input(char *buffer, std::size_t len);
    void async_read_some(char *buffer, std::size_t len, Handler handler);
    void async_write_some(char *buffer, std::size_t len, Handler handler);

private:
    asio::io_service &service_;
    std::shared_ptr<asio::ip::udp::socket> usocket_;
    asio::ip::udp::endpoint ep_;
    std::shared_ptr<asio::deadline_timer> timer_;
    Task rtask_;
    Task wtask_;

private:
    uint32_t convid_ = 0;
    ikcpcb *kcp_ = nullptr;
    char rbuf_[2048];
    char wbuf_[2048];
    char stream_buf_[65535];
    std::size_t streambufsiz_ = 0;
    std::unique_ptr<BaseDecEncrypter> dec_or_enc_;
};

#endif
