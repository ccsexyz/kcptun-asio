//
// Created by ccsexyz on 17-5-3.
//

#ifndef KCPTUN_UTILS_H
#define KCPTUN_UTILS_H

#include "encoding.h"
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

#define TRACE
//    std::cout << __func__ << " " << __LINE__ << " " << __FILE__ << std::endl;

#endif // KCPTUN_UTILS_H
