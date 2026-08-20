#pragma once

#include <cstdint>
#include <cstddef>


struct Peer {
    static constexpr size_t SIZE = 16;

    uint64_t node_id;
    uint32_t ip;
    uint16_t tcp_port;
    uint16_t udp_port;
};