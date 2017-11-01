#include "encrypt.h"
#include "kcptun_server.h"
#include "local.h"
#include "server.h"

int main(int argc, char **argv) {
    gflags::SetUsageMessage("usage: kcptun_server");
    parse_command_lines(argc, argv);
    asio::io_service io_service;
    asio::ip::udp::endpoint local_endpoint;
    asio::ip::tcp::endpoint target_endpoint;
    {
        asio::ip::udp::resolver resolver(io_service);
        local_endpoint = asio::ip::udp::endpoint(*resolver.resolve(
            {asio::ip::udp::v4(), get_host(FLAGS_localaddr), get_port(FLAGS_localaddr)}));
    }
    {
        asio::ip::tcp::resolver resolver(io_service);
        target_endpoint = asio::ip::tcp::endpoint(*resolver.resolve(
            {asio::ip::tcp::v4(), get_host(FLAGS_targetaddr), get_port(FLAGS_targetaddr)}));
    }
    std::make_shared<kcptun_server>(io_service, local_endpoint, target_endpoint)
        ->run();
    if (FLAGS_kvar) {
        run_kvar_printer(io_service);
    }
    io_service.run();
    gflags::ShutDownCommandLineFlags();
    google::ShutdownGoogleLogging();
    return 0;
}