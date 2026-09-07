#include "dht_node.h"

#include "../peer/peer_manager.h"


DHTNode::DHTNode(uint64_t node_id, uint16_t tcp_port, uint16_t udp_port) {
    self_.node_id = node_id;
    self_.tcp_port = tcp_port;
    self_.udp_port = udp_port;
    self_.ip = 0;
}

void DHTNode::start(uint16_t dht_port) {
    node_.run(dht_port, dht::crypto::generateIdentity(), true);
    node_.bootstrap("bootstrap.jami.net", "4222");
}

void DHTNode::announce() {
    auto data = serialize(self_);
    node_.put(ROOM_KEY, data);
}

void DHTNode::discover(PeerDiscoveryCallback cb) {
    auto key = dht::InfoHash::get(ROOM_KEY);
    auto token = node_.listen(key, [this, cb](const std::vector<std::shared_ptr<dht::Value>>& values, bool expired) {
        for (const auto& value : values) {
            Peer peer = deserialize(value->data);
            if (peer.node_id == self_.node_id) continue;
            cb(peer);
        }
        return true;
    });
}

void DHTNode::shutdown() {
    node_.join();
}

void DHTNode::try_set_public_ip() {
    if (self_.ip != 0) return;

    for (const auto& addr : node_.getPublicAddress(AF_INET)) {
        if (!addr.isPrivate() && !addr.isLoopback()) {
            self_.ip = addr.getIPv4().sin_addr.s_addr;
            printf("[info] public ip: %s\n", inet_ntoa(*reinterpret_cast<in_addr*>(&self_.ip)));

            // Re-announce() with the real IP. The first announce() at startup publishes ip = 0 (almost always)
            // this call updates it
            announce();
            break;
        }
    }
}

uint32_t DHTNode::get_self_ip() const {
    return self_.ip;
}
