#include "udp_handler.h"

#include <chrono>
#include <cstdio>
#include <netinet/in.h>
#include <sys/socket.h>

#include "../protocol/rendezvous_codec.h"
#include "../protocol/security/hmac_utils.h"
#include "../utils/error_utils.h"
#include "../utils/io_events.h"
#include "../utils/socket_setup.h"
#include "../utils/socket_utils.h"


UDPHandler::UDPHandler(int port, std::vector<uint8_t> secret) : port_(port), secret_(std::move(secret)) {
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
            auto verified = HMACAuth::verify_and_strip(secret_, packet.data);
            if (!verified) {
                fprintf(stderr, "[udp] dropped unauthenticated packet from VPS endpoint\n");
                continue;
            }
            if (rendezvous_callback_) {
                auto notify = RendezvousCodec::decode_notify(*verified);
                rendezvous_callback_(notify);
            }
        }
        else {
            if (punch_callback_) {
                uint32_t ip = packet.sender.sin_addr.s_addr;
                uint16_t port = ntohs(packet.sender.sin_port);

                auto it = endpoint_to_node_.find({ip, port});

                if (it != endpoint_to_node_.end()) punch_callback_(it->second, ip, port);
            }
        }
    }
}

void UDPHandler::on_tick() {
    auto time_point = std::chrono::steady_clock::now();

    for (auto& target : punch_targets_) {
        if (target.remaining_sends <= 0) continue;
        if (time_point < target.next_send) continue;
        if (target.success) continue;

        send_to(target.public_endpoint.ip, target.public_endpoint.port, reinterpret_cast<const uint8_t*>(punch_msg_.data()), punch_msg_.size());
        send_to(target.private_endpoint.ip, target.private_endpoint.port, reinterpret_cast<const uint8_t*>(punch_msg_.data()), punch_msg_.size());
        target.remaining_sends--;
        target.next_send = time_point + std::chrono::milliseconds(30);
    }

    std::erase_if(punch_targets_, [&](const PunchTarget& t) {
        if (t.remaining_sends <= 0 || t.success) {
            endpoint_to_node_.erase({t.public_endpoint.ip, t.public_endpoint.port});
            endpoint_to_node_.erase({t.private_endpoint.ip, t.private_endpoint.port});
            return true;
        }
        return false;
    });
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

    if (auto result = udp_send_to(get_fd(), data, len, addr); !result)
        fprintf(stderr, "[udp] send failed: %s\n", result.error().message().c_str());
}

void UDPHandler::set_vps_address(uint32_t ip, uint16_t port) {
    vps_endpoint_.ip = ip;
    vps_endpoint_.port = htons(port);
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

void UDPHandler::setup_punch(uint64_t node_id, uint32_t public_ip, uint16_t public_port, uint32_t private_ip, uint16_t private_port) {

    if (punch_targets_.size() >= MAX_PUNCH_TARGETS) {
        fprintf(stderr, "[punch] target limit reached, ignoring new punch\n");
        return;
    }

    PunchTarget punch_target {
        .node_id = node_id,
        .public_endpoint = {
            .ip = public_ip,
            .port = public_port
        },
        .private_endpoint = {
            .ip = private_ip,
            .port = private_port
        },
        .remaining_sends = 100,
        .next_send = std::chrono::steady_clock::now()
    };

    endpoint_to_node_.try_emplace({public_ip, public_port}, node_id);
    endpoint_to_node_.try_emplace({private_ip, private_port}, node_id);

    punch_targets_.push_back(punch_target);
}

void UDPHandler::mark_punch_success(uint64_t node_id, uint32_t ip, uint16_t port) {
    if (punched_peers_.contains(node_id)) return;
    punched_peers_.insert(node_id);
    printf("[punch] hole opened for node %lu <-- %s:%d\n", node_id, ip_to_str(ntohl(ip)).c_str(), port);

    for (auto& target : punch_targets_) {
        if (target.node_id == node_id) {
            target.success = true;
        }
    }
}

