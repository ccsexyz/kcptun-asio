#include "encrypt.h"
#include "kcptun_client.h"
#include "kcptun_server.h"
#include "local.h"
#include "server.h"

config global_config;

int main(void) {
    global_config.crypt = "none";
    global_config.key = "hello";
    global_config.nocomp = false;
    global_config.key = pbkdf2(global_config.key);
    global_config.keepalive = 10;
    asio::io_service io_service;
    asio::ip::udp::resolver uresolver(io_service);

    //    auto tgep = *resolver.resolve({asio::ip::tcp::v4(), "www.baidu.com",
    //    "80"});
    // std::make_shared<kcptun_server>(io_service,
    // asio::ip::udp::endpoint(asio::ip::udp::v4(), 6666), tgep)->run();
    std::make_shared<kcptun_client>(
        io_service, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), 7778),
        asio::ip::udp::endpoint(
            *uresolver.resolve({asio::ip::udp::v4(), "0.0.0.0", "6666"})))
        ->run();
    io_service.run();
    return 0;
}
