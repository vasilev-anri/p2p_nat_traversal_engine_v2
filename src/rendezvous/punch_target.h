#pragma once

#include <chrono>
#include <cstdint>


struct PunchEndpoint {
    uint32_t ip;
    uint16_t port;
};

struct PunchTarget {
    uint64_t node_id;

    PunchEndpoint public_endpoint;
    PunchEndpoint private_endpoint;

    int remaining_sends;
    std::chrono::steady_clock::time_point next_send;

    bool success = false;
};