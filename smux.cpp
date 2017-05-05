#include "smux.h"
#include "config.h"

void smux::run() {
    do_receive_frame();
    do_keepalive_checker();
    do_keepalive_sender();
}

void smux::do_keepalive_checker() {
    std::weak_ptr<smux> weaksmux = shared_from_this();
    if (!keepalive_check_timer_) {
        keepalive_check_timer_ = std::make_shared<asio::deadline_timer>(
            service_, boost::posix_time::seconds(global_config.keepalive * 3));
    } else {
        keepalive_check_timer_->expires_at(
            keepalive_check_timer_->expires_at() +
            boost::posix_time::seconds(global_config.keepalive * 3));
    }
    keepalive_check_timer_->async_wait(
        [this, weaksmux](const std::error_code &) {
            auto s = weaksmux.lock();
            if (!s || destroy_) {
                return;
            }
            if (data_ready_) {
                data_ready_ = false;
                do_keepalive_checker();
            } else {
                s->destroy();
            }
        });
}

void smux::do_keepalive_sender() {
    std::weak_ptr<smux> weaksmux = shared_from_this();
    if (!keepalive_sender_timer_) {
        keepalive_sender_timer_ = std::make_shared<asio::deadline_timer>(
            service_, boost::posix_time::seconds(global_config.keepalive));
    } else {
        keepalive_sender_timer_->expires_at(
            keepalive_sender_timer_->expires_at() +
            boost::posix_time::seconds(global_config.keepalive));
    }
    keepalive_sender_timer_->async_wait(
        [this, weaksmux](const std::error_code &) {
            auto s = weaksmux.lock();
            if (!s || destroy_) {
                return;
            }
            async_write_frame(frame{VERSION, cmdNop, 0, 0}, nullptr);
            do_keepalive_sender();
        });
}

void smux::do_stat_checker() {
    auto self = shared_from_this();
    auto stat_timer = std::make_shared<asio::deadline_timer>(
        service_, boost::posix_time::seconds(1));
    stat_timer->async_wait([this, self, stat_timer](const std::error_code &) {
        if (destroy_) {
            return;
        }
        do_stat_checker();
    });
}

void smux::input(char *buf, std::size_t len, Handler handler) {
    data_ready_ = true;
    if (destroy_) {
        if (handler) {
            handler(std::error_code(1, std::generic_category()), 0);
        }
        return;
    }
    input_task_.reset();
    if (len < read_task_.len) {
        memcpy(read_task_.buf, buf, len);
        read_task_.buf += len;
        read_task_.len -= len;
        if (handler) {
            handler(std::error_code(0, std::generic_category()), 0);
        }
    } else {
        TRACE
        memcpy(read_task_.buf, buf, read_task_.len);
        len -= read_task_.len;
        buf += read_task_.len;
        auto read_handler = read_task_.handler;
        read_task_.reset();
        if (len == 0) {
            TRACE
            if (handler) {
                handler(std::error_code(0, std::generic_category()), 0);
            }
        } else {
            TRACE
            input_task_.buf = buf;
            input_task_.len = len;
            input_task_.handler = handler;
        }
        if (read_handler) {
            read_handler(std::error_code(0, std::generic_category()), 0);
        }
    }
    return;
}

void smux::async_read_full(char *buf, std::size_t len, Handler handler) {
    read_task_.reset();
    if (destroy_) {
        if (handler) {
            handler(std::error_code(1, std::generic_category()), 0);
        }
        return;
    }
    TRACE
    if (len < input_task_.len) {
        memcpy(buf, input_task_.buf, len);
        input_task_.len -= len;
        input_task_.buf += len;
        TRACE
        if (handler) {
            TRACE
            handler(std::error_code(0, std::generic_category()), 0);
        }
    } else if (len == input_task_.len) {
        memcpy(buf, input_task_.buf, len);
        auto input_handler = input_task_.handler;
        input_task_.reset();
        TRACE
        if (input_handler) {
            TRACE
            input_handler(std::error_code(0, std::generic_category()), 0);
        }
        if (handler) {
            handler(std::error_code(0, std::generic_category()), 0);
        }
    } else {
        TRACE
        memcpy(buf, input_task_.buf, input_task_.len);
        read_task_.len = len - input_task_.len;
        read_task_.buf = buf + input_task_.len;
        read_task_.handler = handler;
        auto input_handler = input_task_.handler;
        input_task_.reset();
        if (input_handler) {
            TRACE
            input_handler(std::error_code(0, std::generic_category()), 0);
        }
    }
    return;
}

void smux::do_receive_frame() {
    auto self = shared_from_this();
    frame_flag = true;
    async_read_full(frame_header_, headerSize,
                    [this, self](std::error_code ec, std::size_t) {
                        if (ec) {
                            TRACE
                            return;
                        }
                        frame f = frame::unmarshal(frame_header_);
                        async_read_full(frame_data_,
                                        static_cast<std::size_t>(f.length),
                                        [f, this, self](std::error_code ec,
                                                        std::size_t) mutable {
                                            if (ec) {
                                                return;
                                            }
                                            f.data = frame_data_;
                                            frame_flag = false;
                                            handle_frame(f);
                                        });
                    });
}

void smux::handle_frame(frame f) {
    auto it = sessions_.find(f.id);
    auto id = f.id;
    switch (f.cmd) {
    case cmdSyn:
        if (it == sessions_.end() && acceptHandler_) {
            auto s = std::make_shared<smux_sess>(
                service_, id, VERSION, std::weak_ptr<smux>(shared_from_this()));
            acceptHandler_(s);
            sessions_.emplace(std::make_pair(id, std::weak_ptr<smux_sess>(s)));
        }
        do_receive_frame();
        break;
    case cmdFin:
        if (it != sessions_.end()) {
            auto s = it->second.lock();
            if (s) {
                sessions_.erase(f.id);
                s->destroy();
            }
        }
        do_receive_frame();
        break;
    case cmdPsh:
        if (it != sessions_.end()) {
            auto s = it->second.lock();
            if (s) {
                auto self = shared_from_this();
                s->input(f.data, static_cast<std::size_t>(f.length),
                         [this, self](std::error_code ec, std::size_t) {
                             TRACE
                             do_receive_frame();
                         });
            } else {
                do_receive_frame();
            }
        }
        break;
    case cmdNop:
        do_receive_frame();
        break;
    default:
        TRACE
        destroy();
        break;
    }
    return;
}

void smux::async_write(char *buf, std::size_t len, Handler handler) {
    if (destroy_) {
        if (handler) {
            handler(std::error_code(1, std::generic_category()), 0);
        }
        return;
    }
    TRACE
    out_handler_(buf, len, handler);
}

void smux::destroy() {
    destroy_ = true;
    auto read_handler = read_task_.handler;
    auto input_handler = input_task_.handler;
    read_task_.reset();
    input_task_.reset();
    if (read_handler) {
        read_handler(std::error_code(1, std::generic_category()), 0);
    }
    if (input_handler) {
        input_handler(std::error_code(1, std::generic_category()), 0);
    }
}

smux_sess::smux_sess(asio::io_service &io_service, uint32_t id, uint8_t version,
                     std::weak_ptr<smux> sm)
    : service_(io_service), id_(id), version_(version), sm_(sm) {}

void smux_sess::destroy() {
    destroy_ = true;
    auto read_handler = read_task_.handler;
    auto input_handler = input_task_.handler;
    read_task_.reset();
    input_task_.reset();
    if (read_handler) {
        read_handler(std::error_code(1, std::generic_category()), 0);
    }
    if (input_handler) {
        input_handler(std::error_code(1, std::generic_category()), 0);
    }
}

void smux_sess::input(char *buf, std::size_t len, Handler handler) {
    if (destroy_) {
        if (handler) {
            handler(std::error_code(1, std::generic_category()), 0);
        }
        return;
    }
    input_task_.reset();
    if (read_task_.check()) {
        if (len <= read_task_.len) {
            memcpy(read_task_.buf, buf, len);
            auto read_handler = read_task_.handler;
            read_task_.reset();
            if (read_handler) {
                read_handler(std::error_code(0, std::generic_category()), len);
            }
            if (handler) {
                handler(std::error_code(0, std::generic_category()), 0);
            }
        } else {
            memcpy(read_task_.buf, buf, read_task_.len);
            auto read_handler = read_task_.handler;
            auto read_len = read_task_.len;
            read_task_.reset();
            len -= read_len;
            buf += read_len;
            input_task_.buf = buf;
            input_task_.len = len;
            input_task_.handler = handler;
            if (read_handler) {
                read_handler(std::error_code(0, std::generic_category()),
                             read_len);
            }
        }
    } else {
        input_task_.buf = buf;
        input_task_.len = len;
        input_task_.handler = handler;
    }
    return;
}

void smux_sess::async_read_some(char *buf, std::size_t len, Handler handler) {
    if (destroy_) {
        if (handler) {
            handler(std::error_code(1, std::generic_category()), 0);
        }
        return;
    }
    if (input_task_.len == 0) {
        read_task_.buf = buf;
        read_task_.len = len;
        read_task_.handler = handler;
    } else if (len < input_task_.len) {
        memcpy(buf, input_task_.buf, len);
        input_task_.len -= len;
        input_task_.buf += len;
        if (handler) {
            handler(std::error_code(0, std::generic_category()), len);
        }
    } else {
        memcpy(buf, input_task_.buf, input_task_.len);
        auto input_len = input_task_.len;
        auto input_handler = input_task_.handler;
        input_task_.reset();
        if (handler) {
            handler(std::error_code(0, std::generic_category()), input_len);
        }
        if (input_handler) {
            input_handler(std::error_code(0, std::generic_category()), 0);
        }
    }
    return;
}

void smux_sess::async_write(char *buf, std::size_t len, Handler handler) {
    if (destroy_) {
        if (handler) {
            handler(std::error_code(1, std::generic_category()), 0);
        }
        return;
    }
    auto s = sm_.lock();
    if (!s) {
        if (handler) {
            handler(std::error_code(1, std::generic_category()), 0);
        }
        return;
    }
    auto f = frame{version_, cmdPsh, static_cast<uint16_t>(len), id_};
    f.marshal(data);
    memcpy(data + headerSize, buf, len);
    s->async_write(data, len + headerSize, handler);
}

smux_sess::~smux_sess() {
    auto s = sm_.lock();
    if (s) {
        s->async_write_frame(frame{version_, cmdFin, 0, id_}, nullptr);
    }
}

void smux::async_write_frame(frame f, Handler handler) {
    TRACE
    if (destroy_) {
        TRACE
        if (handler) {
            handler(std::error_code(1, std::generic_category()), 0);
        }
        return;
    }
    char *header = new char[headerSize];
    f.marshal(header);
    async_write(header, headerSize,
                [handler, header](std::error_code ec, std::size_t) {
                    if (ec) {
                        TRACE
                    }
                    delete[] header;
                    if (handler) {
                        handler(ec, 0);
                    }
                });
}

void smux::async_connect(
    std::function<void(std::shared_ptr<smux_sess>)> connectHandler) {
    TRACE
    auto self = shared_from_this();
    nextStreamID_ += 2;
    auto sid = nextStreamID_;
    auto ss = std::make_shared<smux_sess>(service_, sid, VERSION,
                                          std::weak_ptr<smux>(self));
    async_write_frame(
        frame{VERSION, cmdSyn, 0, sid},
        [this, self, ss, connectHandler, sid](std::error_code ec, std::size_t) {
            TRACE
            if (ec) {
                TRACE
                return;
            }
            if (connectHandler) {
                sessions_.emplace(sid, std::weak_ptr<smux_sess>(ss));
                connectHandler(ss);
            }
        });
}
