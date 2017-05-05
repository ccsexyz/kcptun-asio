#include "kcptun_client.h"
#include "kcptun_server.h"
#include "local.h"
#include "server.h"
#include "encrypt.h"

config global_config;

int main(void) {
    global_config.crypt = "aes-256-cfb";
    global_config.key = "password";
    global_config.key = pbkdf2(global_config.key);
    global_config.keepalive = 3; 
    asio::io_service io_service;
    asio::ip::udp::resolver uresolver(io_service);

//    auto tgep = *resolver.resolve({asio::ip::tcp::v4(), "www.baidu.com", "80"});
    // std::make_shared<kcptun_server>(io_service,
    // asio::ip::udp::endpoint(asio::ip::udp::v4(), 6666), tgep)->run();
    std::make_shared<kcptun_client>(
        io_service, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), 7777),
        asio::ip::udp::endpoint(*uresolver.resolve({asio::ip::udp::v4(), "45.66.6.66", "12580"})))
        ->run();
    io_service.run();
    return 0;
}
