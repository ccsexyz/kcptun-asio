#include "encrypt.h"
#include "kcptun_client.h"
#include "local.h"
#include "server.h"

void print_configs() {
    info("listening on: %s\n"
         "encryption: %s\n"
         "nodelay parameters: %d %d %d %d\n"
         "remote address: %s\n"
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
         RemoteAddr.c_str(), SndWnd, RcvWnd, get_bool_str(!NoComp), Mtu,
         DataShard, ParityShard, get_bool_str(AckNodelay), Dscp, SockBuf,
         KeepAlive, Conn, AutoExpire, ScavengeTTL);
}

int main(int argc, char **argv) {
    parse_command_lines(argc, argv);
    process_configs();
    print_configs();
    asio::io_service io_service;
    asio::ip::udp::endpoint remote_endpoint;
    asio::ip::tcp::endpoint local_endpoint;
    {
        asio::ip::udp::resolver resolver(io_service);
        remote_endpoint = asio::ip::udp::endpoint(*resolver.resolve(
            {asio::ip::udp::v4(), get_host(RemoteAddr), get_port(RemoteAddr)}));
    }
    {
        asio::ip::tcp::resolver resolver(io_service);
        local_endpoint = asio::ip::tcp::endpoint(*resolver.resolve(
            {asio::ip::tcp::v4(), get_host(LocalAddr), get_port(LocalAddr)}));
    }
    std::make_shared<kcptun_client>(io_service, local_endpoint, remote_endpoint)
        ->run();
    io_service.run();
    return 0;
}
