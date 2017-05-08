#include "encrypt.h"
#include "kcptun_server.h"
#include "local.h"
#include "server.h"

void print_configs() {
    info("listening on: %s\n"
         "encryption: %s\n"
         "nodelay parameters: %d %d %d %d\n"
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
         LocalAddr.c_str(), Crypt.c_str(), NoDelay, Interval, Resend, Nc,
         TargetAddr.c_str(), SndWnd, RcvWnd, get_bool_str(!NoComp), Mtu,
         DataShard, ParityShard, get_bool_str(AckNodelay), Dscp, SockBuf,
         KeepAlive, Conn, AutoExpire, ScavengeTTL);
}

int main(int argc, char **argv) {
    parse_command_lines(argc, argv);
    process_configs();
    print_configs();
    asio::io_service io_service;
    asio::ip::udp::endpoint local_endpoint;
    asio::ip::tcp::endpoint target_endpoint;
    {
        asio::ip::udp::resolver resolver(io_service);
        local_endpoint = asio::ip::udp::endpoint(*resolver.resolve(
            {asio::ip::udp::v4(), get_host(LocalAddr), get_port(LocalAddr)}));
    }
    {
        asio::ip::tcp::resolver resolver(io_service);
        target_endpoint = asio::ip::tcp::endpoint(*resolver.resolve(
            {asio::ip::tcp::v4(), get_host(TargetAddr), get_port(TargetAddr)}));
    }
    std::make_shared<kcptun_server>(io_service, local_endpoint, target_endpoint)
        ->run();
    io_service.run();
    return 0;
}