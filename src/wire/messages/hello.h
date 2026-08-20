#pragma once

#include <cstdint>
#include <cstddef>

/**
 *  node_id         - unique peer identity; prevents connecting to wrong host
 *  tcp_port        - peer's TCP listening port
 *  udp_port        - peer's udp port for hole punching
 */

struct Hello {
    static constexpr size_t MIN_SIZE = 12;

    uint64_t node_id;
    uint16_t tcp_port;
    uint16_t udp_port;
};
