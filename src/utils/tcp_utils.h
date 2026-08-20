#pragma once

#include <cerrno>
#include <netinet/in.h>

#include "socket_utils.h"
#include "unique_fd.h"
#include "../reactor/reactor.h"


#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "../handlers/tcp_session.h"




/*  handle the loop and fd management; hand each accepted fd to the caller  */
template <typename Callback>
void accept_all(int fd, Callback cb) {
    while (true) {
        sockaddr_in addr{};
        socklen_t addr_len = sizeof(addr);
        auto cfd = UniqueFD(::accept(fd, reinterpret_cast<sockaddr*>(&addr), &addr_len));

        if (!cfd.is_valid()) {
            if (errno == EINTR) continue;
            if (errno == EAGAIN || errno == EWOULDBLOCK) break;
            break;
        }

        if (auto res = set_nonblocking(cfd.get()); !res) continue;

        cb(std::move(cfd));
    }
}

// socket() -> set_nonblocking() -> connect() -> CONNECTED || CONNECTING -> register via epoll
inline void connect_to_peer(Reactor& reactor, const std::string& ip, uint16_t port, uint64_t node_id, uint16_t self_tcp_port, uint16_t self_udp_port) {
    auto fd = UniqueFD(::socket(AF_INET, SOCK_STREAM, 0));
    if (!fd.is_valid()) return;

    if (auto res = set_nonblocking(fd.get()); !res) return;

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);

    auto state = connect_on_socket(fd.get(), addr);
    if (!state) return;

    bool want_write = (*state == SessionState::CONNECTING);

    auto session = std::make_unique<TCPSession>(std::move(fd), SessionRole::OUTBOUND, node_id, self_tcp_port, self_udp_port, *state);

    // if need_write == true -> epollout
    reactor.register_handler(std::move(session), want_write);

}