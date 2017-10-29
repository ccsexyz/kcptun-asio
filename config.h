#ifndef KCPTUN_CONFIG_H
#define KCPTUN_CONFIG_H

#include "utils.h"

DECLARE_string(localaddr);
DECLARE_string(remoteaddr);
DECLARE_string(targetaddr);
DECLARE_string(key);
DECLARE_string(crypt);
DECLARE_string(mode);
DECLARE_string(logfile);

DECLARE_int32(conn);
DECLARE_int32(autoexpire);
DECLARE_int32(mtu);
DECLARE_int32(scavengettl);
DECLARE_int32(sndwnd);
DECLARE_int32(rcvwnd);
DECLARE_int32(datashard);
DECLARE_int32(parityshard);
DECLARE_int32(dscp);
DECLARE_int32(nodelay);
DECLARE_int32(resend);
DECLARE_int32(nc);
DECLARE_int32(sockbuf);
DECLARE_int32(keepalive);
DECLARE_int32(interval);

DECLARE_bool(kvar);
DECLARE_bool(nocomp);
DECLARE_bool(acknodelay);

void parse_command_lines(int argc, char **argv);

std::string get_host(const std::string &addr);

std::string get_port(const std::string &addr);

void process_configs();

#endif
