#ifndef KCPTUN_CONFIG_H
#define KCPTUN_CONFIG_H

#include "utils.h"

struct config {
public:
    std::string local_addr;
    std::string remote_addr;
    std::string key;
    std::string crypt;
    std::string mode;
    int conn;
    int auto_expire;
    int scavengettl;
    int mtu;
    int sndwnd;
    int rcvwnd;
    int datashard;
    int parityshard;
    int dscp;
    bool nocomp;
    bool acknodelay;
    int nodelay;
    int interval;
    int resend;
    int nc;
    int sockbuf;
    int keepalive;
    std::string log;
};

extern config global_config;

#endif
