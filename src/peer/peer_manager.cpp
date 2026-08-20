#include "peer_manager.h"

#include "../utils/codec_utils.h"


std::vector<uint8_t> serialize(Peer& peer) {
    std::vector<uint8_t> res;
    res.reserve(Peer::SIZE);

    write_u64(peer.node_id, res);
    write_u32(peer.ip, res);
    write_u16(peer.tcp_port, res);
    write_u16(peer.udp_port, res);

    return res;
}

Peer deserialize(std::vector<uint8_t>& data) {
    size_t offset = 0;
    Peer peer{};

    const auto node_id = read_u64(data, offset);
    const auto ip = read_u32(data, offset);
    const auto tcp_port = read_u16(data, offset);
    const auto udp_port = read_u16(data, offset);

    peer.node_id = node_id;
    peer.ip = ip;
    peer.tcp_port = tcp_port;
    peer.udp_port = udp_port;

    return peer;
}
