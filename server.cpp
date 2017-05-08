#include "server.h"
#include "async_fec.h"
#include "sess.h"
#include "smux.h"
#include "snappy_stream.h"

Server::Server(asio::io_service &io_service, OutputHandler handler)
    : AsyncInOutputer(handler), service_(io_service) {}

void Server::run(AcceptHandler accept_handler, uint32_t convid) {
    auto fec = DataShard > 0 && ParityShard > 0;

    in = [this](char *buf, std::size_t len, Handler handler) {
        sess_->async_input(buf, len, handler);
    };
    if (fec) {
        auto fec_in = std::make_shared<AsyncFECInputer>(in);
        in = [this, fec_in](char *buf, std::size_t len, Handler handler) {
            fec_in->async_input(buf, len, handler);
        };
    }

    out = [this](char *buf, std::size_t len, Handler handler) {
        output(buf, len, handler);
    };
    if (fec) {
        auto fec_out = std::make_shared<AsyncFECOutputer>(out);
        out = [this, fec_out](char *buf, std::size_t len, Handler handler) {
            fec_out->async_input(buf, len, handler);
        };
    }
    sess_ = std::make_shared<Session>(service_, convid, out);
    sess_->run();

    out2 = [this](char *buf, std::size_t len, Handler handler) {
        sess_->async_write(buf, len, handler);
    };
    if (!NoComp) {
        auto snappy_writer =
            std::make_shared<snappy_stream_writer>(service_, out2);
        out2 = [this, snappy_writer](char *buf, std::size_t len,
                                     Handler handler) {
            snappy_writer->async_input(buf, len, handler);
        };
    }
    smux_ = std::make_shared<smux>(service_, out2);
    smux_->set_accept_handler(accept_handler);
    smux_->run();

    in2 = [this](char *buf, std::size_t len, Handler handler) {
        smux_->async_input(buf, len, handler);
    };
    if (!NoComp) {
        auto snappy_reader =
            std::make_shared<snappy_stream_reader>(service_, in2);
        in2 = [this, snappy_reader](char *buf, std::size_t len,
                                    Handler handler) {
            snappy_reader->async_input(buf, len, handler);
        };
    }

    do_sess_receive();
}

void Server::async_input(char *buf, std::size_t len, Handler handler) {
    in(buf, len, handler);
}

void Server::do_sess_receive() {
    auto self = shared_from_this();
    sess_->async_read_some(
        sbuf_, sizeof(sbuf_), [this, self](std::error_code ec, std::size_t sz) {
            if (ec) {
                return;
            }
            in2(sbuf_, sz, [this, self](std::error_code ec, std::size_t) {
                if (ec) {
                    return;
                }
                do_sess_receive();
            });
        });
}