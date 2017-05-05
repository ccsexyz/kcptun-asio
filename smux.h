#ifndef KCPTUN_SMUX_H
#define KCPTUN_SMUX_H

#include "frame.h"
#include "utils.h"

class smux;

class smux_sess final : public std::enable_shared_from_this<smux_sess> {
public:
    smux_sess(asio::io_service &io_service, uint32_t id, uint8_t version,
              std::weak_ptr<smux> sm);
    ~smux_sess();
    void destroy();
    void input(char *buf, std::size_t len, Handler handler);
    void async_read_some(char *buf, std::size_t len, Handler handler);
    void async_write(char *buf, std::size_t len, Handler handler);

private:
    uint8_t version_;
    uint32_t id_;
    char data[70000];
    bool destroy_ = false;
    asio::io_service &service_;
    Task read_task_;
    Task input_task_;
    std::weak_ptr<smux> sm_;
};

class smux final : public std::enable_shared_from_this<smux> {
public:
    smux(asio::io_service &io_service, OutputHandler handler)
        : service_(io_service), out_handler_(handler) {
    }

    void run();
    void destroy();
    void input(char *buf, std::size_t len, Handler handler);
    void
    async_accept(std::function<void(std::shared_ptr<smux_sess>)> acceptHandler);
    void async_write(char *buf, std::size_t len, Handler handler);
    void async_connect(
        std::function<void(std::shared_ptr<smux_sess>)> connectHandler);
    void async_write_frame(frame f, Handler handler);

private:
    void do_keepalive_checker();
    void do_receive_frame();
    void do_stat_checker();
    void handle_frame(frame f);
    void async_read_full(char *buf, std::size_t len, Handler handler);

private:
    bool data_ready_ = true;
    bool destroy_ = false;
    char frame_header_[headerSize];
    char frame_data_[65536];
    uint16_t nextStreamID_ = 1;
    asio::io_service &service_;
    Task read_task_;
    Task input_task_;
    OutputHandler out_handler_;
    std::function<void(std::shared_ptr<smux_sess>)> acceptHandler_;
    std::unordered_map<uint32_t, std::weak_ptr<smux_sess>> sessions_;
    std::shared_ptr<asio::deadline_timer> keepalive_timer_;
    bool frame_flag = false;
};

#endif
