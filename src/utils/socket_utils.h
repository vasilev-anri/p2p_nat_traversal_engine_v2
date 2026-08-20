#pragma once

#include <expected>
#include <fcntl.h>
#include <vector>
#include <netinet/in.h>
#include <sys/epoll.h>

#include "error_utils.h"
#include "../wire/msg_codec.h"
#include "../wire/msg.h"
#include "../session/session_state.h"


enum class DrainStatus {
    OK,
    CLOSED,
    ERROR
};

struct TCPDrainResult {
    DrainStatus status;
    std::vector<uint8_t> data;
};

struct UDPPacket {
    std::vector<uint8_t> data;
    sockaddr_in sender;
};

struct UDPDrainResult {
    DrainStatus status;
    std::vector<UDPPacket> packets;
};


/**
 *
 * @param port
 * @return
 */
inline sockaddr_in create_address(const int port) {
    sockaddr_in addr{};

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    return addr;
}

inline std::expected<void, SyscallError> set_reuse_address(const int fd) {
    const int yes = 1;
    if (::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
        return std::unexpected(SyscallError::from_errno("setsockopt(SO_REUSEADDR)"));
    return {};
}

inline TCPDrainResult drain_tcp(int fd) {
    std::vector<uint8_t> out;
    while (true) {
        uint8_t buf[4096];
        ssize_t n = ::recv(fd, buf, sizeof(buf), 0);

        if (n > 0) {
            out.insert(out.end(), buf, buf + n);
            continue;
        }

        if (n == 0) return {DrainStatus::CLOSED, std::move(out)};

        if (errno == EINTR) continue;

        if (errno == EAGAIN || errno == EWOULDBLOCK) return {DrainStatus::OK, std::move(out)};

        return {DrainStatus::ERROR, {}};
    }
}

inline void send_all(int fd, Message& message) {
    auto header_bytes = MessageCodec::encode_header(message.header);

    std::vector<uint8_t> out;
    out.reserve(header_bytes.size() + message.payload.size());

    out.insert(out.end(), header_bytes.begin(), header_bytes.end());
    out.insert(out.end(), message.payload.begin(), message.payload.end());

    size_t total = out.size();
    size_t sent = 0;

    while (sent < total) {
        ssize_t n = ::send(fd, out.data() + sent, total - sent, 0);
        if (n == -1) {
            if (errno == EINTR) continue;
            if (errno == EAGAIN || errno == EWOULDBLOCK) continue;
            return;
        }
        sent += n;
    }
}

inline UDPDrainResult drain_udp(int fd) {
    UDPDrainResult result{};

    while (true) {
        UDPPacket packet{};
        socklen_t addr_len = sizeof(packet.sender);
        uint8_t buf[4095];

        ssize_t n = ::recvfrom(fd, buf, sizeof(buf), 0, reinterpret_cast<sockaddr*>(&packet.sender), &addr_len);

        if (n > 0) {
            packet.data.assign(buf, buf + n);
            result.packets.push_back(std::move(packet));
            continue;
        }

        if (n == 0) continue;

        if (errno == EINTR) continue;

        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            result.status = DrainStatus::OK;
            return result;
        }

        result.status = DrainStatus::ERROR;
        return result;
    }
}

inline std::expected<void, SyscallError> set_nonblocking(int fd) {
    const int flags = ::fcntl(fd, F_GETFL, 0);
    if (flags == -1) return std::unexpected(SyscallError::from_errno("fcntl(F_GETFL)"));
    if (::fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
        return std::unexpected(SyscallError::from_errno("fcntl(F_SETFL)"));
    return {};
}

inline std::expected<void, SyscallError> bind_socket(int fd, sockaddr_in& addr) {
    if (::bind(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == -1)
        return std::unexpected(SyscallError::from_errno("bind"));
    return {};
}

inline std::expected<void, SyscallError> listen_socket(int fd, int backlog) {
    if (::listen(fd, backlog) == -1)
        return std::unexpected(SyscallError::from_errno("listen"));
    return {};
}

inline std::expected<SessionState, SyscallError> connect_on_socket(int fd, sockaddr_in& addr) {
    if (::connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == 0) return SessionState::CONNECTED;
    if (errno == EINPROGRESS) return SessionState::CONNECTING;
    return std::unexpected(SyscallError::from_errno("connect"));
}


/**
 * Creates an epoll instance
 * @return UniqueFD owning the epoll file descriptor or SyscallError is epoll_create1() fails
 */
inline std::expected<UniqueFD, SyscallError> create_epoll_fd() {
    int fd = ::epoll_create1(0);
    if (fd == -1)
        return std::unexpected(SyscallError::from_errno("epoll_create1"));
    return UniqueFD(fd);
}

inline std::expected<void, SyscallError> epoll_ctl_add(int epfd, int fd, epoll_event* ev) {
    if (::epoll_ctl(epfd, EPOLL_CTL_ADD, fd, ev) == -1)
        return std::unexpected(SyscallError::from_errno("epoll_ctl(ADD)"));
    return {};
}

inline std::expected<void, SyscallError> epoll_ctl_del(int epfd, int fd) {
    if (::epoll_ctl(epfd, EPOLL_CTL_DEL, fd, nullptr) == -1)
        return std::unexpected(SyscallError::from_errno("epoll_ctl(DEL)"));
    return {};
}

inline std::expected<int, SyscallError> epoll_wait_interruptible(int epfd, epoll_event* events, int maxevents, int timeout) {
    while (true) {
        int nfds = epoll_wait(epfd, events, maxevents, timeout);
        if (nfds >= 0) return nfds;
        if (errno == EINTR) continue;
        return std::unexpected(SyscallError::from_errno("epoll_wait"));
    }
}