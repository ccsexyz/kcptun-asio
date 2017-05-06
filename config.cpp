#include "config.h"
#include "json.h"
#include "utils.h"

std::string LocalAddr = ":12948";
std::string RemoteAddr = ":29900";
std::string TargetAddr = ":29900";
std::string Key = "it's a secrect";
std::string Crypt = "aes";
std::string Mode = "fast";
int Conn = 1;
int AutoExpire = 0;
int Mtu = 1350;
int ScavengeTTL = 600;
int SndWnd = 128;
int RcvWnd = 512;
int DataShard = 10;
int ParityShard = 3;
int Dscp = 0;
bool NoComp = false;
bool AckNodelay = true;
int NoDelay = true;
int Resend = true;
int Nc = 0;
int SockBuf = 4194304;
int KeepAlive = 10;
std::string LogFile;
int Interval = 40;
std::string ConfigFilePath;

using json = nlohmann::json;

void parse_command_lines(int argc, char **argv) {
    std::vector<std::string> cmdlines;
    cmdlines.reserve(argc-1);
    for(int i = 1; i < argc; i++) {
        cmdlines.push_back(argv[i]);
    }
    cmdlines.emplace_back("");
    std::string s;
    std::unordered_map<std::string, std::function<void()>> handlers = {
        {"-c", [&]{ConfigFilePath = s;}},
        {"-l", [&]{LocalAddr = s;}},
        {"--listen", [&]{LocalAddr = s;}},
        {"--localaddr", [&]{LocalAddr = s;}},
        {"--target", [&]{TargetAddr = s;}},
        {"-t", [&]{TargetAddr = s;}},
        {"-r", [&]{RemoteAddr = s;}},
        {"--remoteaddr", [&]{RemoteAddr = s;}},
        {"--key", [&]{Key = s;}},
        {"--crypt", [&]{Crypt = s;}},
        {"--mode", [&]{Mode = s;}},
        {"--mtu", [&]{Mtu = std::stoi(s);}},
        {"--sndwnd", [&]{SndWnd = std::stoi(s);}},
        {"--rcvwnd", [&]{RcvWnd = std::stoi(s);}},
        {"--datashard", [&]{DataShard = std::stoi(s);}},
        {"--ds", [&]{DataShard = std::stoi(s);}},
        {"--parityshard", [&]{ParityShard = std::stoi(s);}},
        {"--dscp", [&]{Dscp = std::stoi(s);}},
        {"--nocomp", [&]{NoComp = true;}},
        {"--log", [&]{LogFile = s;}},
        {"--autoexpire", [&]{AutoExpire = std::stoi(s);}},
        {"--scavengettl", [&]{ScavengeTTL = std::stoi(s);}},
        {"--acknodelay", [&]{AckNodelay = true;}},
        {"--nodelay", [&]{NoDelay = std::stoi(s);}},
        {"--interval", [&]{Interval = std::stoi(s);}},
        {"--resend", [&]{Resend = std::stoi(s);}},
        {"--nc", [&]{Nc = std::stoi(s);}},
        {"--sockbuf", [&]{SockBuf = std::stoi(s);}},
        {"--keepalive", [&]{KeepAlive = std::stoi(s);}},
        {"--conn", [&]{Conn = std::stoi(s);}}
    };
    std::unordered_set<std::string> ons = {
        "--nocomp", "--acknodelay"
    };
    auto size = cmdlines.size();
    for(auto i = 0; i < size; i++) {
        auto fit = handlers.find(cmdlines[i]);
        if(fit == handlers.end()) {
            continue;
        } 
        if(ons.find(cmdlines[i]) == ons.end()) {
            s = cmdlines[i + 1];
            i++;
        }
        (fit->second)();
        s = "";
    }
    if(ConfigFilePath.empty()) {
        return;
    }
    std::ifstream ifs(ConfigFilePath);
    std::string jsonstr((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
    if(jsonstr.empty()) {
        return;
    }
    auto j = json::parse(jsonstr);
    using json_type = decltype(j);
    if(j.is_null() || (!j.is_object())) {
        return;
    }
    for(auto it = j.cbegin(); it != j.cend(); ++it) {
        auto key = it.key();
        auto value = it.value();
        key = "--" + key;
        auto fit = handlers.find(key);
        if(fit == handlers.end()) {
            continue;
        }
        auto handler = fit->second;
        if(!handler) {
            continue;
        }
        if(value.is_boolean() && value.get<bool>()) {
            handler();
        } else if(value.is_string()) {
            s = value.get<std::string>();
            handler();
        } else if(value.is_number_unsigned()) {
            auto number = int(value.get<uint32_t>());
            s = std::to_string(number);
            handler();
        }
        s = "";
    }
    return;
}

std::string get_host(const std::string &addr) {
    auto pos = addr.find_last_of(':');
    if(pos == std::string::npos) {
        std::terminate();
    }
    return addr.substr(0, pos);
}

std::string get_port(const std::string &addr) {
    auto pos = addr.find_last_of(':');
    if(pos == std::string::npos) {
        std::terminate();
    }
    return addr.substr(pos+1);
}

void process_configs() {
    if(Mode == "normal") {
        NoDelay = 0;
        Interval = 40;
        Resend = 2;
        Nc = 1;
    } else if(Mode == "fast") {
        NoDelay = 0;
        Interval = 30;
        Resend = 2;
        Nc = 1;
    } else if(Mode == "fast2") {
        NoDelay = 1;
        Interval = 20;
        Resend = 2;
        Nc = 1;
    } else if(Mode == "fast3") {
        NoDelay = 1;
        Interval = 10;
        Resend = 2;
        Nc = 1;
    }
}