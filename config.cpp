#include "config.h"
#include "rapidjson.h"
#include "document.h"
#include "utils.h"

DEFINE_string(localaddr, ":12948", "local listen address");
DEFINE_string(remoteaddr, ":29900", "kcp server address");
DEFINE_string(targetaddr, ":29900", "target server address");
DEFINE_string(l, "", "alias for localaddr");
DEFINE_string(r, "", "alias for remoteaddr");
DEFINE_string(t, "", "alias for targetaddr");
DEFINE_string(c, "", "config from json file, which will override the command from shell");
DEFINE_string(key, "it's a secret", "pre-shared secret between client and server");
DEFINE_string(crypt, "aes", "aes, aes-128, aes-192, salsa20, blowfish, twofish, cast5, 3des, tea, xtea, xor, none");
DEFINE_string(mode, "fast", "profiles: fast3, fast2, fast, normal");
DEFINE_string(logfile, "", "specify a log file to output, default goes to stdout");

DEFINE_int32(conn, 1, "set num of UDP connections to server");
DEFINE_int32(autoexpire, 0, "set auto expiration time(in seconds) for a single UDP connection, 0 to disable");
DEFINE_int32(mtu, 1350, "set maximum transmission unit for UDP packets");
DEFINE_int32(scavengettl, 600, "set how long an expired connection can live(in sec), -1 to disable");
DEFINE_int32(sndwnd, 128, "set send window size(num of packets)");
DEFINE_int32(rcvwnd, 512, "set receive window size(num of packets)");
DEFINE_int32(datashard, 10, "set reed-solomon erasure coding - datashard");
DEFINE_int32(parityshard, 3, "set reed-solomon erasure coding - parityshard");
DEFINE_int32(ds, -1, "alias for datashard");
DEFINE_int32(ps, -1, "alias for parityshard");
DEFINE_int32(dscp, 0, "set dscp(6bit)");
DEFINE_int32(nodelay, 1, "");
DEFINE_int32(resend, 1, "");
DEFINE_int32(nc, 0, "");
DEFINE_int32(interval, 40, "");
DEFINE_int32(sockbuf, 4194304, "socket buffer size");
DEFINE_int32(keepalive, 10, "keepalive interval in seconds");

DEFINE_bool(nocomp, false, "disable compression");
DEFINE_bool(acknodelay, true, "flush ack immediately when a packet is received");
DEFINE_bool(kvar, false, "run default kvar printer");

using namespace rapidjson;

void print_configs() {
    char buffer[1024];
    snprintf(buffer, 1024, "listening on: %s\n"
                 "encryption: %s\n"
                 "nodelay parameters: %d %d %d %d\n"
                 "remote address: %s\n"
                 "target address: %s\n"
                 "sndwnd: %d rcvwnd: %d\n"
                 "compression: %s\n"
                 "mtu: %d\n"
                 "datashard: %d parityshard: %d\n"
                 "acknodelay: %s\n"
                 "dscp: %d\n"
                 "sockbuf: %d\n"
                 "keepalive: %d\n"
                 "conn: %d\n"
                 "autoexpire: %d\n"
                 "scavengettl: %d\n",
         FLAGS_localaddr.c_str(),
         FLAGS_crypt.c_str(),
         FLAGS_nodelay, FLAGS_interval, FLAGS_resend, FLAGS_nc,
         FLAGS_remoteaddr.c_str(),
         FLAGS_targetaddr.c_str(),
         FLAGS_sndwnd, FLAGS_rcvwnd, get_bool_str(!FLAGS_nocomp), FLAGS_mtu,
         FLAGS_datashard, FLAGS_parityshard, get_bool_str(FLAGS_acknodelay), FLAGS_dscp, FLAGS_sockbuf,
         FLAGS_keepalive, FLAGS_conn, FLAGS_autoexpire, FLAGS_scavengettl);
    LOG(INFO) << buffer;
}

static bool
check_zero_cstr(const char *cstr)
{
    if (cstr == NULL || strlen(cstr) == 0) {
        return true;
    }
    return false;
}

static void
env_assign_bool(char *src, void *dst)
{
    *(bool *)dst = true;
}

static void
env_assign_int32(char *src, void *dst)
{
    *(google::int32*)dst = std::atoi(src);
}

static void
env_assign_string(char *src, void *dst)
{
    *(std::string *)dst = src;
}

static std::unordered_map<std::string, std::tuple<void *, std::function<void (char *, void *)>>> env_assigners = {
    {"localaddr", std::make_tuple(&FLAGS_localaddr, env_assign_bool)},
    {"remoteaddr", std::make_tuple(&FLAGS_remoteaddr, env_assign_string)},
    {"targetaddr", std::make_tuple(&FLAGS_targetaddr, env_assign_string)},
    {"key", std::make_tuple(&FLAGS_key, env_assign_string)},
    {"crypt", std::make_tuple(&FLAGS_crypt, env_assign_string)},
    {"logfile", std::make_tuple(&FLAGS_logfile, env_assign_string)},
    {"mode", std::make_tuple(&FLAGS_mode, env_assign_string)},

    {"conn", std::make_tuple(&FLAGS_conn, env_assign_int32)},
    {"autoexpire", std::make_tuple(&FLAGS_autoexpire, env_assign_int32)},
    {"mtu", std::make_tuple(&FLAGS_mtu, env_assign_int32)},
    {"sndwnd", std::make_tuple(&FLAGS_sndwnd, env_assign_int32)},
    {"rcvwnd", std::make_tuple(&FLAGS_rcvwnd, env_assign_int32)},
    {"scavengettl", std::make_tuple(&FLAGS_scavengettl, env_assign_int32)},
    {"datashard", std::make_tuple(&FLAGS_datashard, env_assign_int32)},
    {"parityshard", std::make_tuple(&FLAGS_parityshard, env_assign_int32)},
    {"nodelay", std::make_tuple(&FLAGS_nodelay, env_assign_int32)},
    {"resend", std::make_tuple(&FLAGS_resend, env_assign_int32)},
    {"nc", std::make_tuple(&FLAGS_nc, env_assign_int32)},
    {"interval", std::make_tuple(&FLAGS_interval, env_assign_int32)},
    {"sockbuf", std::make_tuple(&FLAGS_sockbuf, env_assign_int32)},
    {"keepalive", std::make_tuple(&FLAGS_keepalive, env_assign_int32)},

    {"nocomp", std::make_tuple(&FLAGS_nocomp, env_assign_bool)},
    {"acknodelay", std::make_tuple(&FLAGS_acknodelay, env_assign_bool)},
    {"kvar", std::make_tuple(&FLAGS_kvar, env_assign_bool)},
};

static void
parse_plugin_option(char *plugin_option)
{
    char *key = strtok(plugin_option, "=");
    if (check_zero_cstr(key)) {
        return;
    }
    char *value = strtok(NULL, "=");

    auto it = env_assigners.find(key);
    if (it == env_assigners.end()) {
        return;
    }

    void *dst = std::get<0>(it->second);
    auto &caller = std::get<1>(it->second);
    caller(value, dst);
}

static void
parse_plugin_options(char *plugin_options)
{
    char *str = NULL;
    std::vector<char *> option_strs;
    // note: strtok isn't thread-safe, this function shouldn't
    //       be used in multi-threaded environment
    while ((str = strtok(plugin_options, ";")) != NULL) {
        plugin_options = NULL;
        option_strs.emplace_back(str);
    }

    for (auto str : option_strs) {
        parse_plugin_option(str);
    }
}

// for SIP003
static void
parse_config_from_env()
{
    const char *remote_host = std::getenv("SS_REMOTE_HOST");
    const char *remote_port = std::getenv("SS_REMOTE_PORT");
    const char *local_host = std::getenv("SS_LOCAL_HOST");
    const char *local_port = std::getenv("SS_LOCAL_PORT");
    char *plugin_options = std::getenv("SS_PLUGIN_OPTIONS");

    bool has_zero_str = false;
    has_zero_str |= check_zero_cstr(remote_host);
    has_zero_str |= check_zero_cstr(remote_port);
    has_zero_str |= check_zero_cstr(local_host);
    has_zero_str |= check_zero_cstr(local_port);

    if (has_zero_str) {
        return;
    }

    FLAGS_remoteaddr = std::string(remote_host) + ":" + std::string(remote_port);
    FLAGS_localaddr = std::string(local_host) + ":" + std::string(local_port);

    if (check_zero_cstr(plugin_options)) {
        return;
    }

    plugin_options = strdup(plugin_options);
    parse_plugin_options(plugin_options);
    free(plugin_options);
}

void parse_command_lines(int argc, char **argv) {
    google::LogToStderr();
    DeferCaller defer([] {
        process_configs();
        google::SetLogDestination(0, FLAGS_logfile.c_str());
        print_configs();
    });

    gflags::ParseCommandLineFlags(&argc, &argv, true);
    FLAGS_colorlogtostderr = true;
    google::InitGoogleLogging(argv[0]);

    auto string_alias_check = [](std::string &var, const std::string &alias) {
        if (alias.empty()) {
            return;
        }
        var = alias;
    };
    auto integer_alias_check = [](int &var, int alias) {
        if (alias >= 0) {
            var = alias;
        }
    };

    string_alias_check(FLAGS_localaddr, FLAGS_l);
    string_alias_check(FLAGS_remoteaddr, FLAGS_r);
    string_alias_check(FLAGS_targetaddr, FLAGS_t);

    integer_alias_check(FLAGS_parityshard, FLAGS_ps);
    integer_alias_check(FLAGS_datashard, FLAGS_ds);

    parse_config_from_env();

    if (FLAGS_c.empty()) {
        return;
    }

    std::ifstream ifs(FLAGS_c);
    std::string jsonstr((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
    if (jsonstr.empty()) {
        return;
    }

    Document d;
    d.Parse(jsonstr.data());

    if (d.IsNull() || !d.IsObject()) {
        return;
    }

    std::unordered_map<std::string, std::function<void(bool)>> boolean_handlers;
    std::unordered_map<std::string, std::function<void(int)>> integer_handlers;
    std::unordered_map<std::string, std::function<void(const std::string &)>> string_handlers;

    auto get_int_assigner = [&](const std::string &name, int *pi) {
        integer_handlers.insert(std::make_pair(name, [&, pi](int i) {
            *pi = i;
        }));
    };
    auto get_bool_assigner = [&](const std::string &name, bool *pb) {
        boolean_handlers.insert(std::make_pair(name, [&, pb](bool b) {
            *pb = b;
        }));
    };
    auto get_string_assigner = [&](const std::string &name, std::string *ps) {
        string_handlers.insert(std::make_pair(name, [&, ps](const std::string &s) {
            *ps = s;
        }));
    };

    get_string_assigner("localaddr", &FLAGS_localaddr);
    get_string_assigner("remoteaddr", &FLAGS_remoteaddr);
    get_string_assigner("targetaddr", &FLAGS_targetaddr);
    get_string_assigner("listen", &FLAGS_localaddr);
    get_string_assigner("target", &FLAGS_targetaddr);
    get_string_assigner("key", &FLAGS_key);
    get_string_assigner("crypt", &FLAGS_crypt);
    get_string_assigner("mode", &FLAGS_mode);
    get_string_assigner("logfile", &FLAGS_logfile);

    get_int_assigner("conn", &FLAGS_conn);
    get_int_assigner("autoexpire", &FLAGS_autoexpire);
    get_int_assigner("mtu", &FLAGS_mtu);
    get_int_assigner("scavengettl", &FLAGS_scavengettl);
    get_int_assigner("sndwnd", &FLAGS_sndwnd);
    get_int_assigner("rcvwnd", &FLAGS_rcvwnd);
    get_int_assigner("datashard", &FLAGS_datashard);
    get_int_assigner("parityshard", &FLAGS_parityshard);
    get_int_assigner("dscp", &FLAGS_dscp);
    get_int_assigner("nodelay", &FLAGS_nodelay);
    get_int_assigner("resend", &FLAGS_resend);
    get_int_assigner("nc", &FLAGS_nc);
    get_int_assigner("sockbuf", &FLAGS_sockbuf);
    get_int_assigner("keepalive", &FLAGS_keepalive);
    get_int_assigner("interval", &FLAGS_interval);

    get_bool_assigner("kvar", &FLAGS_kvar);
    get_bool_assigner("nocomp", &FLAGS_nocomp);
    get_bool_assigner("acknodelay", &FLAGS_acknodelay);

    for (auto &m : d.GetObject()) {
        if (!m.name.IsString()) {
            continue;
        }
        std::string key = m.name.GetString();
        auto &value = m.value;
        if (value.IsBool()) {
            auto it = boolean_handlers.find(key);
            if (it != boolean_handlers.end()) {
                (it->second)(value.GetBool());
            }
        } else if (value.IsString()) {
            auto it = string_handlers.find(key);
            if (it != string_handlers.end()) {
                (it->second)(value.GetString());
            }
        } else if (value.IsNumber()) {
            auto it = integer_handlers.find(key);
            if (it != integer_handlers.end()) {
                (it->second)(value.GetInt());
            }
        }
    }
}

std::string get_host(const std::string &addr) {
    auto pos = addr.find_last_of(':');
    if (pos == std::string::npos) {
        std::terminate();
    }
    auto host = addr.substr(0, pos);
    if (host == "") {
        host = "0.0.0.0";
    }
    return host;
}

std::string get_port(const std::string &addr) {
    auto pos = addr.find_last_of(':');
    if (pos == std::string::npos) {
        std::terminate();
    }
    return addr.substr(pos + 1);
}

void process_configs() {
    auto assigner = [](int nodelay, int interval, int resend, int nc) -> std::function<void()> {
        return [nodelay, interval, resend, nc]() {
            FLAGS_nodelay = nodelay;
            FLAGS_interval = interval;
            FLAGS_resend = resend;
            FLAGS_nc = nc;
        };
    };
    std::unordered_map<std::string, std::function<void()>> handlers = {
            {"normal", assigner(0, 40, 2, 1)},
            {"fast",   assigner(0, 30, 2, 1)},
            {"fast2",  assigner(1, 20, 2, 1)},
            {"fast3",  assigner(1, 10, 2, 1)},
    };
    auto it = handlers.find(FLAGS_mode);
    if (it == handlers.end()) {
        return;
    }
    (it->second)();
}
