//
// Created by ccsexyz on 17-5-3.
//

#ifndef KCPTUN_UTILS_H
#define KCPTUN_UTILS_H

#include <algorithm>
#include "asio.hpp"
#include "asio/high_resolution_timer.hpp"
#include <assert.h>
#include <cstdio>
#include <cstdlib>
#include <string.h>
#include <deque>
#include <fstream>
#include <functional>
#include <iostream>
#include <iterator>
#include <list>
#include <memory>
#include <random>
#include <stddef.h>
#include <stdint.h>
#include <streambuf>
#include <string>
#include <system_error>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#include <chrono>
#include <map>
#include <array>
#include "gflags/gflags.h"
#include "glog/logging.h"

#include "encoding.h"

enum { nonce_size = 16, crc_size = 4 };
enum { mtu_limit = 1500 };

// support make_unique in c++ 11
template <typename T, typename... Args>
inline typename std::enable_if<!std::is_array<T>::value, std::unique_ptr<T>>::type
my_make_unique(Args &&... args) {
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

template <typename T>
inline typename std::enable_if<std::is_array<T>::value && std::extent<T>::value == 0,
std::unique_ptr<T>>::type
my_make_unique(size_t size) {
    using U = typename std::remove_extent<T>::type;
    return std::unique_ptr<T>(new U[size]());
}

template <typename T, typename... Args>
typename std::enable_if<std::extent<T>::value != 0, void>::type
my_make_unique(Args &&...) = delete;

using Handler = std::function<void(std::error_code, std::size_t)>;
using OutputHandler = std::function<void(char *, std::size_t, Handler)>;

static void itimeofday(long *sec, long *usec) {
    struct timeval time;
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count();
    if (sec != nullptr) {
        *sec = ms / 1000000;
    }
    if (usec != nullptr) {
        *usec = ms % 1000000;
    }
}

static uint64_t iclock64(void) {
    long s, u;
    uint64_t value;
    itimeofday(&s, &u);
    value = ((uint64_t)s) * 1000 + (u / 1000);
    return value;
}

static uint32_t iclock() { return (uint32_t)(iclock64() & 0xfffffffful); }

struct Task {
    Task() { reset(); }
    Task(char *buf, std::size_t len, Handler handler)
        : buf(buf), len(len), handler(handler) {}

    char *buf;
    std::size_t len;
    Handler handler;
    int c = 8;

    void reset() {
        buf = nullptr;
        len = 0;
        handler = nullptr;
    }

    bool check() { return buf != nullptr && len != 0; }
};

#define IEEEPOLY 0xedb88320
#define CastagnoliPOLY 0x82f63b78

static inline uint32_t crc32c_ieee(uint32_t crc, const unsigned char *buf,
                                   size_t len) {
    int k;

    crc = ~crc;
    while (len--) {
        crc ^= *buf++;
        for (k = 0; k < 8; k++)
            crc = crc & 1 ? (crc >> 1) ^ IEEEPOLY : crc >> 1;
    }
    return ~crc;
}

static inline uint32_t crc32c_cast(uint32_t crc, const unsigned char *buf,
                                   std::size_t len) {
    int k;

    crc = ~crc;
    while (len--) {
        crc ^= *buf++;
        for (k = 0; k < 8; k++)
            crc = crc & 1 ? (crc >> 1) ^ CastagnoliPOLY : crc >> 1;
    }
    crc = ~crc;
    return (uint32_t)(crc >> 15 | crc << 17) + 0xa282ead8;
}

static inline std::error_code errc(int c) {
    return std::error_code(c, std::generic_category());
}

#define TRACE
//    std::cout << __func__ << " " << __LINE__ << " " << __FILE__ << std::endl;

class clean_ {
public:
    clean_(const clean_ &) = delete;

    clean_(clean_ &&) = delete;

    clean_ &operator=(const clean_ &) = delete;

    clean_() = default;
};

class DeferCaller final : clean_ {
public:
    DeferCaller(std::function<void()> &&functor)
        : functor_(std::move(functor)) {}

    DeferCaller(const std::function<void()> &functor) : functor_(functor) {}

    ~DeferCaller() {
        if (functor_)
            functor_();
    }

    void cancel() { functor_ = nullptr; }

private:
    std::function<void()> functor_;
};

class ConstructCaller final : clean_ {
public:
    ConstructCaller(std::function<void()> &&functor) {
        functor();
    }
};

class Destroy : public clean_ {
public:
    ~Destroy() {
        destroy();
    }
    void call_on_destroy(const std::function<void()> &h) {
        destroy_handlers_.push_back(h);
    }
    void call_on_destroy(std::function<void()> &&h) {
        destroy_handlers_.emplace_back(h);
    }
    void destroy() {
        if (destroy_) {
            return;
        }
        destroy_ = true;
        call_this_on_destroy();
        for (auto &h : destroy_handlers_) {
            if (h) {
                h();
            }
        }
        destroy_handlers_.clear();
    }
    bool is_destroyed() const {
        return destroy_;
    }

protected:
    virtual void call_this_on_destroy() {}

private:
    bool destroy_ = false;
    std::vector<std::function<void()>> destroy_handlers_;
};

class AsyncReadWriter {
public:
    virtual ~AsyncReadWriter() = default;
    virtual void async_read_some(char *buf, std::size_t len,
                                 Handler handler) = 0;
    virtual void async_write(char *buf, std::size_t len, Handler handler) = 0;
};

class AsyncInOutputer {
public:
    AsyncInOutputer() = default;
    AsyncInOutputer(OutputHandler o) : o_(o) {}
    virtual ~AsyncInOutputer() = default;
    void set_output_handler(OutputHandler o) { o_ = o; }
    virtual void async_input(char *buf, std::size_t len, Handler handler) = 0;

protected:
    void output(char *buf, std::size_t len, Handler handler) {
        o_(buf, len, handler);
    }

private:
    OutputHandler o_;
};

class UsocketReadWriter : public AsyncReadWriter {
public:
    UsocketReadWriter(asio::ip::udp::socket &&usocket,
                      asio::ip::udp::endpoint ep)
        : usocket_(std::move(usocket)), ep_(ep) {}
    UsocketReadWriter(asio::ip::udp::socket &&usocket)
        : usocket_(std::move(usocket)), connected_(true) {}
    void async_read_some(char *buf, std::size_t len, Handler handler) override {
        usocket_.async_receive(asio::buffer(buf, len), handler);
    }
    void async_write(char *buf, std::size_t len, Handler handler) override {
        if (connected_) {
            usocket_.async_send(asio::buffer(buf, len), handler);
        } else {
            usocket_.async_send_to(asio::buffer(buf, len), ep_, handler);
        }
    }

private:
    bool connected_ = false;
    asio::ip::udp::socket usocket_;
    asio::ip::udp::endpoint ep_;
};

static inline const char *get_bool_str(bool b) {
    if (b) {
        return "true";
    }
    return "false";
}

class Buffers final {
public:
    explicit Buffers(std::size_t n = 2048) : n(n) {}
    ~Buffers() {
        for (auto &buf : all_bufs_) {
            free(buf);
        }
    }
    Buffers(const Buffers &b) {
        n = b.n;
    }
    Buffers &operator=(const Buffers &b) {
        n = b.n;
        return *this;
    }

    void reset(std::size_t n2) {
        n = n2;
    }

    void push_back(char *buf);
    char *get();

    std::size_t capacity() const {
        return all_bufs_.size();
    }
    std::size_t size() const {
        return bufs_.size();
    }

private:
    std::size_t n;
    std::unordered_set<char *> bufs_;
    std::unordered_set<char *> all_bufs_;
};

class kvar final {
public:
    explicit kvar(const std::string &name);
    ~kvar();

    void add(int i) {
        (*p) += i;
    }
    void sub(int i) {
        (*p) -= i;
    }
    int get() {
        return *p;
    }

private:
    int *p;
    std::string name_;
};

class kvar_ : public clean_ {
public:
    explicit kvar_(kvar &kv) : v_(kv) {
        v_.add(1);
    }
    ~kvar_() {
        v_.sub(1);
    }

private:
    kvar &v_;
};

void printKvars();

void run_kvar_printer(asio::io_service &service);

struct buffer final {
public:
    buffer();
    ~buffer();

    buffer(const buffer &) = delete;

    buffer &operator =(buffer &) = delete;

    buffer(buffer &&other) noexcept;

    std::size_t size() {
        return len;
    }

    std::size_t aval() {
        return cap - off - len;
    }

    char *start() {
        return buf + off;
    }

    char *end() {
        return buf + off + len;
    }

    void append(char *b, std::size_t sz) {
        memcpy(end(), b, sz);
        len += sz;
    }

    void retrieve(char *b, std::size_t sz) {
        memcpy(b, start(), sz);
        off += sz;
        len -= sz;
    }

    char *buf = nullptr;
    std::size_t off = 0;
    std::size_t len = 0;
    std::size_t cap = 0;

private:
    void init();
};

class LinearBuffer final : public clean_ {
public:
    LinearBuffer();
    ~LinearBuffer();

    std::size_t size();
    void append(char *buf, std::size_t len);
    void retrieve(char *buf, std::size_t len);

private:
    std::deque<buffer> bufs_;
};

#endif // KCPTUN_UTILS_H
