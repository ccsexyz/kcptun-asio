#ifndef SHADOWSOCKS_ASIO_LOGGER_H
#define SHADOWSOCKS_ASIO_LOGGER_H

#include <string>

enum { VERBOSE, DEBUG, INFO, WARN, FATAL };

void setLogLevel(int level);

void setLogFile(std::string filepath);

void log(int level, const int line, const char *func, const char *fmt...);

#define verbose(fmt, ...) log(VERBOSE, __LINE__, __func__, fmt, ##__VA_ARGS__)
#define debug(fmt, ...) log(DEBUG, __LINE__, __func__, fmt, ##__VA_ARGS__)
#define info(fmt, ...) log(INFO, __LINE__, __func__, fmt, ##__VA_ARGS__)
#define warn(fmt, ...) log(WARN, __LINE__, __func__, fmt, ##__VA_ARGS__)
#define fatal(fmt, ...)                                                        \
    do {                                                                       \
        log(FATAL, __LINE__, __func__, fmt, ##__VA_ARGS__);                    \
        ::exit(1);                                                             \
    } while (0)
#define debugif(condition, ...)                                                \
    do {                                                                       \
        if ((condition))                                                       \
            debug(__VA_ARGS__);                                                \
    } while (0)
#define infoif(condition, ...)                                                 \
    do {                                                                       \
        if ((condition))                                                       \
            info(__VA_ARGS__);                                                 \
    } while (0)
#define warnif(condition, ...)                                                 \
    do {                                                                       \
        if ((condition))                                                       \
            warn(__VA_ARGS__);                                                 \
    } while (0)
#define fatalif(condition, ...)                                                \
    do {                                                                       \
        if ((condition)) {                                                     \
            fatal(__VA_ARGS__);                                                \
        }                                                                      \
    } while (0)

#endif // SHADOWSOCKS_ASIO_LOGGER_H
