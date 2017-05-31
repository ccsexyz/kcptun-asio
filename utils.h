//
// Created by ccsexyz on 17-5-3.
//

#ifndef KCPTUN_UTILS_H
#define KCPTUN_UTILS_H

#include <algorithm>
#include <arpa/inet.h>
#include <asio.hpp>
#include <assert.h>
#include <cstdio>
#include <cstdlib>
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
#include <sys/time.h>
#include <system_error>
#include <tuple>
#include <type_traits>
#include <unistd.h>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "encoding.h"
#include "logger.h"

enum { nonce_size = 16, crc_size = 4 };
enum { mtu_limit = 1500 };

#if __cplusplus < 201402L
// support make_unique in c++ 11
namespace std {
template <typename T, typename... Args>
inline typename enable_if<!is_array<T>::value, unique_ptr<T>>::type
make_unique(Args &&... args) {
    return unique_ptr<T>(new T(std::forward<Args>(args)...));
}

template <typename T>
inline typename enable_if<is_array<T>::value && extent<T>::value == 0,
                          unique_ptr<T>>::type
make_unique(size_t size) {
    using U = typename remove_extent<T>::type;
    return unique_ptr<T>(new U[size]());
}

template <typename T, typename... Args>
typename enable_if<extent<T>::value != 0, void>::type
make_unique(Args &&...) = delete;
}

#endif

using Handler = std::function<void(std::error_code, std::size_t)>;
using OutputHandler = std::function<void(char *, std::size_t, Handler)>;

static void itimeofday(long *sec, long *usec) {
    struct timeval time;
    gettimeofday(&time, NULL);
    if (sec)
        *sec = time.tv_sec;
    if (usec)
        *usec = time.tv_usec;
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
    void async_read_some(char *buf, std::size_t len, Handler handler) override {
        usocket_.async_receive(asio::buffer(buf, len), handler);
    }
    void async_write(char *buf, std::size_t len, Handler handler) override {
        usocket_.async_send_to(asio::buffer(buf, len), ep_, handler);
    }

private:
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
    Buffers(std::size_t n = 2048) : n(n) {}
    ~Buffers() {
        for (auto &buf : all_bufs_) {
            free(buf);
        }
    }
    void push_back(char *buf) {
        bufs_.insert(buf);
        auto s1 = bufs_.size();
        auto s2 = all_bufs_.size();
        if(s1 * 4 > s2 * 3 && s2 > 16) {
            std::vector<char *> v;
            auto it = 0;
            for(auto buf : bufs_) {
                if(++it > s1 / 2) {
                    break;
                }
                v.push_back(buf);
            }
            for(auto buf : v) {
                bufs_.erase(buf);
                all_bufs_.erase(buf);
                free(buf);
            }
        }
    }
    char *get() {
        for(auto buf : bufs_) {
            bufs_.erase(buf);
            return buf;
        }
        char *buf = static_cast<char *>(malloc(n));
        all_bufs_.insert(buf);
        return buf;
    }
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

#endif // KCPTUN_UTILS_H
