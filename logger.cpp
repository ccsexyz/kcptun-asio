#include "logger.h"
#include <chrono>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

static int loglevel = VERBOSE;

class holder {
public:
    void setfd(int fd) {
        if (fd_ >= 0) {
            close(fd_);
        }
        fd_ = fd;
    }
    const int getfd() const { return fd_; }
    ~holder() {
        if (fd_ >= 0) {
            close(fd_);
        }
    }

private:
    int fd_ = -1;
};

static holder hdr;

void setLogLevel(int level) { loglevel = level; }

void setLogFile(std::string filepath) {
    if (filepath.length() == 0) {
        return;
    }
    int fd = open(filepath.c_str(), O_APPEND | O_WRONLY | O_CREAT, 0);
    if (fd < 0) {
        return;
    }
    hdr.setfd(fd);
}

static const char *getLevelString(int level) {
    switch (level) {
    case VERBOSE:
        return "VERBOSE";
    case DEBUG:
        return "DEBUG";
    case INFO:
        return "INFO";
    case WARN:
        return "WARN";
    case ERROR:
        return "ERROR";
    default:
        return "FATAL";
    }
}

__thread time_t savedSec;
__thread char savedSecStr[128];
__thread char *usecStr;

const char *getCurrTimeStr() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    if (tv.tv_sec != savedSec) {
        struct tm t;
        localtime_r(&(tv.tv_sec), &t);
        savedSec = tv.tv_sec;
        sprintf(savedSecStr, "%04d.%02d.%02d-%02d:%02d:%02d-", t.tm_year + 1900,
                t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec);
        usecStr = savedSecStr + strlen(savedSecStr);
    }
    sprintf(usecStr, "%06d", tv.tv_usec);
    return savedSecStr;
}

void log(int level, const int line, const char *func, const char *fmt...) {
    if (level < loglevel) {
        return;
    }
    std::size_t offset = 0;
    const std::size_t buffersize = 4096;
    static char buffer[4096];
    char *buf = buffer;

    offset += snprintf(buf + offset, buffersize - offset, "[%s][%s] %d %s ",
                       getLevelString(level), getCurrTimeStr(), line, func);

    va_list args;
    va_start(args, fmt);
    offset += vsnprintf(buf + offset, buffersize - offset, fmt, args);
    va_end(args);

    while (buf[offset] == '\n' || buf[offset] == '\0')
        offset--;
    buf[++offset] = '\n';
    buf[++offset] = '\0';

    int fd = STDOUT_FILENO;
    if (hdr.getfd() >= 0) {
        fd = hdr.getfd();
    } else if (level > INFO) {
        fd = STDERR_FILENO;
    }

    auto ret = write(fd, buf, offset);
    if (ret < 0) {
        if (hdr.getfd() >= 0) {
            hdr.setfd(-1);
        }
        if (level > INFO) {
            fd = STDERR_FILENO;
        } else {
            fd = STDOUT_FILENO;
        }
        write(fd, buf, offset);
    }
}