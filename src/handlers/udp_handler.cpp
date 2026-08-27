#include "udp_handler.h"

#include <cstdio>
#include <netinet/in.h>
#include <sys/socket.h>

#include "../utils/error_utils.h"
#include "../utils/io_events.h"
#include "../utils/socket_setup.h"
#include "../utils/socket_utils.h"


UDPHandler::UDPHandler(int port) : port_(port) {
    fd_ = UniqueFD(::socket(AF_INET, SOCK_DGRAM, 0));
    setup();
}

void UDPHandler::handle_event(uint32_t events) {
    if (events & IOEvents::ERROR) {
        done();
        return;
    }

    auto [status, packets] = drain_udp(get_fd());

    if (status == DrainStatus::ERROR) {
        done();
        return;
    }

    if (packets.empty()) return;

    for (const auto& packet : packets) {
        if (packet.sender.sin_addr.s_addr == vps_endpoint_.ip && packet.sender.sin_port == vps_endpoint_.port) {
            if (rendezvous_callback_) {
                auto* notify = reinterpret_cast<const Notify*>(packet.data.data());
                rendezvous_callback_(*notify);
            }
        } else {
            if (punch_callback_) {
                punch_callback_(packet.sender.sin_addr.s_addr, packet.sender.sin_port);
            }
        }
    }

    fflush(stdout);
}

int UDPHandler::get_fd() {
    return fd_.get();
}

int UDPHandler::get_port_() {
    return port_;
}

void UDPHandler::send_to(uint32_t ip, uint16_t port, const uint8_t* data, size_t len) {
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = ip;


    // debug
    printf("send_to: %s:%d len=%zu\n",
           inet_ntoa(addr.sin_addr),
           ntohs(addr.sin_port),
           len);

    ssize_t n = ::sendto(get_fd(), data, len, 0,
                         reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
    if (n == -1) perror("sendto");
}

void UDPHandler::set_vps_address(uint32_t ip, uint16_t port) {
    vps_endpoint_.ip = ip;
    vps_endpoint_.port = port;
}



void UDPHandler::setup() {
    setup_bound_socket(get_fd(), port_);
}

void UDPHandler::set_rendezvous_callback(RendezvousCallback cb) {
    rendezvous_callback_ = std::move(cb);
}

void UDPHandler::set_punch_callback(PunchCallback cb) {
    punch_callback_ = std::move(cb);
}
