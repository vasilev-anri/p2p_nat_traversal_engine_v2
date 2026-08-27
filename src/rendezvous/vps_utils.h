#pragma once
#include <cstdint>
#include <ifaddrs.h>
#include <iostream>
#include <arpa/inet.h>
#include <netinet/in.h>

struct VPSEndpoint {
    uint32_t ip;
    uint16_t port;
};


inline uint32_t get_private_ip() {
    ifaddrs* interfaces = nullptr;

    if (getifaddrs(&interfaces) == -1) return -1;

    uint32_t res = 0;
    for (ifaddrs* iface = interfaces; iface != nullptr; iface = iface->ifa_next) {
        if (!iface->ifa_addr) continue;
        if (iface->ifa_addr->sa_family != AF_INET) continue;
        if (std::string(iface->ifa_name) == "lo") continue;
        auto* addr = reinterpret_cast<sockaddr_in*>(iface->ifa_addr);
        res = addr->sin_addr.s_addr;
        break;
    }
    freeifaddrs(interfaces);
    return res;
}

inline std::string ip_to_str(uint32_t ip) {
    char buf[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &ip, buf, sizeof(buf));
    return buf;
}