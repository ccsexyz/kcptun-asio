//
// Created by ccsexyz on 17-5-3.
//

#ifndef KCPTUN_SESS_H
#define KCPTUN_SESS_H

#include "config.h"
#include "encrypt.h"
#include "ikcp.h"
#include "matrix.h"
#include "utils.h"

class FEC;

class Session : public std::enable_shared_from_this<Session>,
                public AsyncReadWriter,
                public AsyncInOutputer {
public:
    Session(asio::io_service &service, uint32_t convid, OutputHandler o);
    void run();
    ~Session();

private:
    void run_timer();
    static int output_wrapper(const char *buffer, int len, struct IKCPCB *kcp,
                              void *user);
    void update();
    void run_peeksize_checker();

public:
    void input(char *buffer, std::size_t len);
    void async_input(char *buffer, std::size_t len, Handler handler) override;
    void async_read_some(char *buffer, std::size_t len, Handler handler) override;
    void async_write(char *buffer, std::size_t len, Handler handler) override;

private:
    asio::io_service &service_;
    std::shared_ptr<asio::deadline_timer> timer_;
    Task rtask_;
    Task wtask_;

private:
    uint32_t convid_ = 0;
    ikcpcb *kcp_ = nullptr;
    char stream_buf_[65535];
    std::size_t streambufsiz_ = 0;
};

#endif
