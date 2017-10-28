#ifndef KCPTUN_CONFIG_H
#define KCPTUN_CONFIG_H

#include "utils.h"

extern std::string LocalAddr;
extern std::string RemoteAddr;
extern std::string TargetAddr;
extern std::string Key;
extern std::string Crypt;
extern std::string Mode;
extern int Conn;
extern int AutoExpire;
extern int Mtu;
extern int ScavengeTTL;
extern int SndWnd;
extern int RcvWnd;
extern int DataShard;
extern int ParityShard;
extern int Dscp;
extern bool NoComp;
extern bool AckNodelay;
extern int NoDelay;
extern int Resend;
extern int Nc;
extern int SockBuf;
extern int KeepAlive;
extern std::string LogFile;
extern int Interval;
extern bool Kvar;

void parse_command_lines(int argc, char **argv);

std::string get_host(const std::string &addr);

std::string get_port(const std::string &addr);

void process_configs();

#endif
