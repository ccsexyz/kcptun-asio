#include "sess.h"
#include "encrypt.h"
#include "fec.h"

static kvar sess_kvar("Session");

Session::Session(asio::io_service &service, uint32_t convid, OutputHandler o)
    : AsyncInOutputer(o), service_(service), convid_(convid), kvar_(sess_kvar) {
}

Session::~Session() {
    TRACE
    if (kcp_ != nullptr) {
        ikcp_release(kcp_);
    }
}

void Session::run() {
    kcp_ = ikcp_create(convid_, static_cast<void *>(this));
    kcp_->output = Session::output_wrapper;
    kcp_->stream = 1;
    ikcp_nodelay(kcp_, FLAGS_nodelay, FLAGS_interval, FLAGS_resend, FLAGS_nc);
    ikcp_wndsize(kcp_, FLAGS_sndwnd, FLAGS_rcvwnd);
    ikcp_setmtu(kcp_, FLAGS_mtu);
    timer_ = std::make_shared<asio::high_resolution_timer>(service_);
    run_timer(std::chrono::high_resolution_clock::now() + std::chrono::milliseconds(FLAGS_interval));
    // run_peeksize_checker();
}

void Session::run_timer(std::chrono::high_resolution_clock::time_point pt) {
    if(timer_->expires_at() <= pt && timer_->expires_from_now() >= std::chrono::milliseconds(0)) {
        return;
    } else {
        timer_->expires_at(pt);
    }
    std::weak_ptr<Session> ws = shared_from_this();
    timer_->async_wait([this, ws](const std::error_code &) {
        auto s = ws.lock();
        if (!s) {
            return;
        }
        update();
    });
}

void Session::run_peeksize_checker() {
    auto self = shared_from_this();
    auto timer = std::make_shared<asio::high_resolution_timer>(
        service_, std::chrono::seconds(1));
    timer->async_wait([this, self, timer](const std::error_code &) {
        std::cout << ikcp_peeksize(kcp_) << std::endl;
        run_peeksize_checker();
    });
}

void Session::async_input(char *buffer, std::size_t len, Handler handler) {
    input(buffer, len);
    if (handler) {
        handler(errc(0), len);
    }
}

void Session::input(char *buffer, std::size_t len) {
    auto n = ikcp_input(kcp_, buffer, int(len));
    TRACE
    if (rtask_.check()) {
        update();
    } else {
        run_timer(std::chrono::high_resolution_clock::now() + std::chrono::milliseconds(FLAGS_interval));
    }
    return;
}

void Session::async_read_some(char *buffer, std::size_t len, Handler handler) {
    if (streambufsiz_ > 0) {
        auto n = streambufsiz_;
        if (n > len) {
            n = len;
        }
        memcpy(buffer, stream_buf_, n);
        streambufsiz_ -= n;
        if (streambufsiz_ != 0) {
            memmove(stream_buf_, stream_buf_ + n, streambufsiz_);
        }
        if (handler) {
            handler(std::error_code(0, std::generic_category()),
                    static_cast<std::size_t>(n));
        }
        return;
    }
    auto psz = ikcp_peeksize(kcp_);
    if (psz <= 0) {
        rtask_.buf = buffer;
        rtask_.len = len;
        rtask_.handler = handler;
        return;
    }
    auto n = ikcp_recv(kcp_, buffer, int(len));
    if (handler) {
        handler(std::error_code(0, std::generic_category()),
                static_cast<std::size_t>(n));
    }
    if (psz > len) {
        n = ikcp_recv(kcp_, stream_buf_, sizeof(stream_buf_));
        streambufsiz_ = n;
    }
    return;
}

void Session::async_write(char *buffer, std::size_t len, Handler handler) {
    auto n = ikcp_send(kcp_, buffer, int(len));
    if (handler) {
        handler(std::error_code(0, std::generic_category()),
                static_cast<std::size_t>(n));
    }
    run_timer(std::chrono::high_resolution_clock::now() + std::chrono::milliseconds(FLAGS_interval));
}

int Session::output_wrapper(const char *buffer, int len, struct IKCPCB *kcp,
                            void *user) {
    assert(user != nullptr);
    Session *sess = static_cast<Session *>(user);
    sess->output((char *)(buffer), static_cast<std::size_t>(len), nullptr);
    return 0;
}

void Session::update() {
    DeferCaller defer([this]{
        auto current = iclock();
        auto next = ikcp_check(kcp_, current);
        next -= current;
        if(next == 0) {
            next = 1;
        }
        // info("next = %u\n", next);
        run_timer(std::chrono::high_resolution_clock::now()+std::chrono::milliseconds(next));
    });
    ikcp_update(kcp_, iclock());
    if (!rtask_.check()) {
        return;
    }
    auto psz = ikcp_peeksize(kcp_);
    if (psz <= 0) {
        return;
    }
    auto n = ikcp_recv(kcp_, rtask_.buf, int(rtask_.len));
    auto rtask_handler = rtask_.handler;
    rtask_.reset();
    if (rtask_handler) {
        rtask_handler(std::error_code(0, std::generic_category()),
                      static_cast<std::size_t>(n));
    }
}

void Session::call_this_on_destroy() {
    auto self = shared_from_this();

    Destroy::call_this_on_destroy();

    if (timer_) {
        timer_->cancel();
        timer_ = nullptr;
    }

    if (rtask_.check()) {
        auto rtask_handler = rtask_.handler;
        rtask_.reset();
        rtask_handler(std::error_code(1, std::generic_category()), 0);
    }

    if (wtask_.check()) {
        auto wtask_handler = wtask_.handler;
        wtask_.reset();
        wtask_handler(std::error_code(1, std::generic_category()), 0);
    }
}
