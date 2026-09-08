#include "rendezvous_client.h"

#include "../protocol/RendezvousCodec.h"


RendezvousClient::RendezvousClient(UDPHandler& udp, uint16_t tcp_port, uint64_t node_id, uint32_t vps_ip, uint16_t vps_port) : udp_(udp), tcp_port_(tcp_port), node_id_(node_id) {
    vps_endpoint_.ip = vps_ip;
    vps_endpoint_.port = vps_port;
}

void RendezvousClient::send_register() {
    Register msg{};
    msg.header.node_id = node_id_;
    msg.header.type = RendezvousMessageType::REGISTER;
    msg.private_endpoint.ip = get_private_ip();
    msg.private_endpoint.udp_port = udp_.get_port_();
    msg.private_endpoint.tcp_port = tcp_port_;

    const auto data = RendezvousCodec::encode_register(msg);

    printf("[rendezvous] registering with VPS - node_id: %lu\n", node_id_);

    udp_.send_to(vps_endpoint_.ip, vps_endpoint_.port, data.data(), data.size());
}

void RendezvousClient::send_keep_alive() {
    Header msg{};
    msg.node_id = node_id_;
    msg.type = RendezvousMessageType::KEEPALIVE;

    const auto data = RendezvousCodec::encode_header(msg);

    udp_.send_to(vps_endpoint_.ip, vps_endpoint_.port, data.data(), data.size());
}

void RendezvousClient::send_request(uint64_t target_node) {
    Request msg{};
    msg.header.node_id = node_id_;
    msg.header.type = RendezvousMessageType::REQUEST;
    msg.target_node_id = target_node;
    msg.private_endpoint.ip = get_private_ip();
    msg.private_endpoint.udp_port = udp_.get_port_();
    msg.private_endpoint.tcp_port = tcp_port_;

    const auto data = RendezvousCodec::encode_request(msg);

    udp_.send_to(vps_endpoint_.ip, vps_endpoint_.port, data.data(), data.size());
}

void RendezvousClient::handle_notify(Notify* msg) {
    if (notified_peers_.contains(msg->header.node_id)) return;
    notified_peers_.insert(msg->header.node_id);

    printf("[rendezvous] NOTIFY received from node: %lu\n", msg->header.node_id);
    if (notify_callback_) {
        notify_callback_(msg->header.node_id, msg->public_endpoint, msg->private_endpoint);
    }
}

void RendezvousClient::set_notify_callback(NotifyCallback cb) {
    notify_callback_ = std::move(cb);
}

