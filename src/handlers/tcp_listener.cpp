#include "tcp_listener.h"

#include <sys/socket.h>
#include <netinet/in.h>

#include "tcp_session.h"
#include "../utils/error_utils.h"
#include "../utils/io_events.h"
#include "../utils/socket_setup.h"
#include "../utils/tcp_utils.h"


TCPListener::TCPListener(int port, uint64_t node_id, uint16_t udp_port) : port_(port), node_id_(node_id), udp_port_(udp_port) {
    listener_ = UniqueFD(::socket(AF_INET, SOCK_STREAM, 0));
    setup();
}

void TCPListener::handle_event(uint32_t events) {
    if (events & IOEvents::ERROR) {
        done();
        return;
    }

    if (events & IOEvents::READABLE) {
        accept_all(get_fd(), [&](UniqueFD cfd) {
            auto conn = std::make_unique<TCPSession>(std::move(cfd), SessionRole::INBOUND, node_id_, static_cast<uint16_t>(port_), udp_port_);
            if (on_accept_) on_accept_(std::move(conn));
        });
    }
}

int TCPListener::get_fd() {
    return listener_.get();
}

void TCPListener::setup() {
    setup_bound_socket(get_fd(), port_);
    throw_on_error(listen_socket(get_fd(), 10));
}

void TCPListener::set_event_callback(SessionCallback cb) {
    on_accept_ = std::move(cb);
}

