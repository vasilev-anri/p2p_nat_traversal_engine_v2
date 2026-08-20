#pragma once

#include <opendht.h>
#include <functional>
#include "../peer/peer.h"


class DHTNode {
public:
    using PeerDiscoveryCallback = std::function<void(const Peer&)>;

    DHTNode(uint64_t node_id, uint16_t tcp_port, uint16_t udp_port);

    void start(uint16_t dht_port);
    void announce();
    void discover(PeerDiscoveryCallback cb);
    void shutdown();

    void try_set_public_ip();

    uint32_t get_self_ip() const;

private:
    dht::DhtRunner node_;
    Peer self_{};

    static constexpr auto ROOM_KEY = "p2p_nat_traversal_ax308_f90mbr";
};
