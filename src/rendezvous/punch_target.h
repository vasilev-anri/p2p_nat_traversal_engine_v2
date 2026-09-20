#pragma once

#include <chrono>
#include <cstdint>
#include <tuple>


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

// for reverse lookups
// endpoint ==> node
struct EndpointKey {
    uint32_t ip;
    uint16_t port;

    bool operator==(const EndpointKey& o) const {
        return ip == o.ip && port == o.port;
    }

    bool operator<(const EndpointKey& o) const {
        return std::tie(ip, port) < std::tie(o.ip, o.port);
    }
};