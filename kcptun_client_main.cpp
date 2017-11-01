#include "encrypt.h"
#include "kcptun_client.h"
#include "local.h"
#include "server.h"

int main(int argc, char **argv) {
    gflags::SetUsageMessage("usage: kcptun_client");
    parse_command_lines(argc, argv);
    asio::io_service io_service;
    asio::ip::udp::endpoint remote_endpoint;
    asio::ip::tcp::endpoint local_endpoint;
    {
        asio::ip::udp::resolver resolver(io_service);
        remote_endpoint = asio::ip::udp::endpoint(*resolver.resolve(
            {asio::ip::udp::v4(), get_host(FLAGS_remoteaddr), get_port(FLAGS_remoteaddr)}));
    }
    {
        asio::ip::tcp::resolver resolver(io_service);
        local_endpoint = asio::ip::tcp::endpoint(*resolver.resolve(
            {asio::ip::tcp::v4(), get_host(FLAGS_localaddr), get_port(FLAGS_localaddr)}));
    }
    std::make_shared<kcptun_client>(io_service, local_endpoint, remote_endpoint)
        ->run();
    if (FLAGS_kvar) {
        run_kvar_printer(io_service);
    }
    io_service.run();
    gflags::ShutDownCommandLineFlags();
    google::ShutdownGoogleLogging();
    return 0;
}
