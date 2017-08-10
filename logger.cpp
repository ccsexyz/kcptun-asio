#include "logger.h"
#include <chrono>
#include <fstream>
#include <iostream>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <asio/high_resolution_timer.hpp>

static int loglevel = VERBOSE;

static std::fstream fs;

void setLogLevel(int level) { loglevel = level; }

void setLogFile(std::string filepath) {
    if (filepath.length() == 0) {
        return;
    }
    fs.open(filepath, fs.binary | fs.out | fs.app);
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

void log(int level, const int line, const char *func, const char *fmt...) {
    if (level < loglevel) {
        return;
    }
    std::size_t offset = 0;
    const std::size_t buffersize = 4096;
    static char buffer[4096];
    char *buf = buffer;

    offset += snprintf(buf + offset, buffersize - offset, "[%s] %d %s ",
                       getLevelString(level), line, func);

    va_list args;
    va_start(args, fmt);
    offset += vsnprintf(buf + offset, buffersize - offset, fmt, args);
    va_end(args);

    while (buf[offset] == '\n' || buf[offset] == '\0')
        offset--;
    buf[++offset] = '\n';
    buf[++offset] = '\0';

    if (fs.is_open()) {
        fs.write(buf, offset);
        fs.flush();
    } else {
        std::cout.write(buf, offset);
        std::cout.flush();
    }
}