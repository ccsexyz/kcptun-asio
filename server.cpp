#include "server.h"

Server::Server(asio::io_service &io_service, asio::ip::udp::endpoint ep)
    : service_(io_service),
      usocket_(std::make_shared<asio::ip::udp::socket>(io_service, ep)) {}

void Server::run(AcceptHandler handler) {
    acceptHandler_ = handler;
    do_receive();
}

void Server::do_receive() {
    auto self = shared_from_this();
    usocket_->async_receive_from(
        asio::buffer(buf_, sizeof(buf_)), ep_,
        [this, self](std::error_code ec, std::size_t len) {
            if (ec) {
                return;
            }
            auto it = sessions_.find(ep_);
            std::shared_ptr<Session> sess;
            if (it != sessions_.end()) {
                sess = it->second;
            } else {
                uint32_t convid;
                decode32u((byte *)(buf_), &convid);
                sess =
                    std::make_shared<Session>(service_, usocket_, ep_, convid);
                sessions_.emplace(ep_, sess);
                sess->run();
                if (acceptHandler_) {
                    acceptHandler_(sess);
                }
            }
            sess->input(buf_, len);
            do_receive();
        });
}
