#include "rendezvous_client.h"


RendezvousClient::RendezvousClient(UDPHandler& udp, uint16_t tcp_port, uint64_t node_id, uint32_t vps_ip, uint16_t vps_port) : udp_(udp), tcp_port_(tcp_port), node_id_(node_id) {
    vps_endpoint_.ip = vps_ip;
    vps_endpoint_.port = vps_port;
}

void RendezvousClient::send_register() {
    Header header{};
    header.node_id = node_id_;
    header.type = MessageType::REGISTER;

    Register msg{};
    msg.header = header;
    msg.private_endpoint.ip = get_private_ip();
    msg.private_endpoint.udp_port = htons(udp_.get_port_());
    msg.private_endpoint.tcp_port = htons(tcp_port_);

    udp_.send_to(vps_endpoint_.ip, vps_endpoint_.port, reinterpret_cast<const uint8_t*>(&msg), sizeof(msg));
}

void RendezvousClient::send_keep_alive() {
    Header msg{};
    msg.node_id = node_id_;
    msg.type = MessageType::KEEPALIVE;

    udp_.send_to(vps_endpoint_.ip, vps_endpoint_.port, reinterpret_cast<const uint8_t*>(&msg), sizeof(msg));
}

void RendezvousClient::send_request(uint64_t target_node) {
    Header header{};
    header.node_id = node_id_;;
    header.type = MessageType::REQUEST;

    Request msg{};
    msg.header = header;
    msg.target_node_id = target_node;
    msg.private_endpoint.ip = get_private_ip();
    msg.private_endpoint.udp_port = htons(udp_.get_port_());
    msg.private_endpoint.tcp_port = htons(tcp_port_);

    udp_.send_to(vps_endpoint_.ip, vps_endpoint_.port, reinterpret_cast<const uint8_t*>(&msg), sizeof(msg));
}

void RendezvousClient::handle_notify(Notify* msg) {
    if (notify_callback_) {
        notify_callback_(msg->public_endpoint, msg->private_endpoint);
    }
}

void RendezvousClient::set_notify_callback(NotifyCallback cb) {
    notify_callback_ = std::move(cb);
}

