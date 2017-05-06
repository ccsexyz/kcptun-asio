#include "encrypt.h"
#include "kcptun_client.h"
#include "kcptun_server.h"
#include "local.h"
#include "server.h"

void print_configs() {
    std::cout << "listening on: " << LocalAddr << "\n";
    std::cout << "encryption: " << Crypt << "\n";
    std::cout << "nodelay parameters: " << NoDelay << " " << Interval << " " << Resend << " " << Nc << "\n";
    std::cout << "remote address: " << RemoteAddr << "\n";
    std::cout << "sndwnd: " << SndWnd << " rcvwnd: " << RcvWnd << "\n";
    std::cout << "compression: " << (NoComp ? "false " : "true ") << "\n";
    std::cout << "mtu: " << Mtu << "\n";
    std::cout << "datashard: " << DataShard << " parityshard: " << ParityShard << "\n"; 
    std::cout << "acknodelay: " << (AckNodelay ? "true ": "false ") << "\n";
    std::cout << "dscp: " << Dscp << "\n";
    std::cout << "sockbuf: " << SockBuf << "\n";
    std::cout << "keepalive: " << KeepAlive << "\n";
    std::cout << "conn: " << Conn << "\n";
    std::cout << "autoexpire: " << AutoExpire << "\n";
    std::cout << "scavengettl: " << ScavengeTTL << std::endl;
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
        remote_endpoint = asio::ip::udp::endpoint(*resolver.resolve({asio::ip::udp::v4(), get_host(RemoteAddr), get_port(RemoteAddr)}));
    }
    {
        asio::ip::tcp::resolver resolver(io_service);
        local_endpoint = asio::ip::tcp::endpoint(*resolver.resolve({asio::ip::tcp::v4(), get_host(LocalAddr), get_port(LocalAddr)}));
    }
    std::make_shared<kcptun_client>(io_service, local_endpoint, remote_endpoint)->run();
    io_service.run();
    return 0;
}
