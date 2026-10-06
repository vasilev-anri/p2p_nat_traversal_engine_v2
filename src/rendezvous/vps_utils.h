#pragma once

#include <cstdint>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <cstdlib>
#include <stdexcept>
#include <string>

struct VPSEndpoint {
    uint32_t ip;
    uint16_t port;
};

inline std::string load_vps_ip() {
    const char* raw = std::getenv("P2P_VPS_IP");
    if (raw == nullptr) throw std::runtime_error("Environment variable P2P_VPS_IP not set");

    std::string ip(raw);

    constexpr const char* whitespace = " \t\n\r\f\v";
    ip.erase(0, ip.find_first_not_of(whitespace));
    ip.erase(ip.find_last_not_of(whitespace) + 1);

    if (ip.empty()) throw std::runtime_error("Environment variable P2P_VPS_IP can not be empty");

    in_addr temp{};
    if (inet_pton(AF_INET, ip.c_str(), &temp) != 1)
        throw std::runtime_error("Environment variable P2P_VPS_IP is not a valid IPv4 address: " + ip);

    return ip;
}

inline uint32_t get_private_ip() {
    ifaddrs* interfaces = nullptr;

    if (getifaddrs(&interfaces) == -1) return -1;

    uint32_t res = 0;
    for (ifaddrs* iface = interfaces; iface != nullptr; iface = iface->ifa_next) {
        if (!iface->ifa_addr) continue;
        if (iface->ifa_addr->sa_family != AF_INET) continue;
        if (std::string(iface->ifa_name) == "lo") continue;
        auto* addr = reinterpret_cast<sockaddr_in*>(iface->ifa_addr);
        res = ntohl(addr->sin_addr.s_addr);
        break;
    }
    freeifaddrs(interfaces);
    return res;
}

inline std::string ip_to_str(uint32_t host_order_ip) {
    uint32_t net = htonl(host_order_ip);
    char buf[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &net, buf, sizeof(buf));
    return buf;
}