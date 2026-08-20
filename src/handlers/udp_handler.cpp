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
        fwrite(packet.data.data(), 1, packet.data.size(), stdout);
    }

    fflush(stdout);
}

int UDPHandler::get_fd() {
    return fd_.get();
}

void UDPHandler::setup() {
    setup_bound_socket(get_fd(), port_);
}
