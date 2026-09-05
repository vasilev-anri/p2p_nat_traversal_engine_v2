#include "tcp_session.h"

#include <cstdio>
#include <random>
#include <sys/socket.h>

#include "../utils/io_events.h"
#include "../utils/socket_utils.h"
#include "../wire/msg_codec.h"
#include "../wire/msg_types.h"
#include "../wire/messages/hello.h"


TCPSession::TCPSession(UniqueFD fd, SessionRole role, uint64_t node_id, uint16_t tcp_port, uint16_t udp_port, SessionState initial)
    : fd_(std::move(fd)), role_(role), node_id_(node_id), self_tcp_port_(tcp_port), self_udp_port_(udp_port), state_(initial) {
    if (role_ == SessionRole::OUTBOUND && state_ == SessionState::CONNECTED) {
        send_hello();
    }
}

void TCPSession::handle_event(uint32_t events) {
    if (events & IOEvents::ERROR) {
        done();
        return;
    }

    if (state_ == SessionState::CONNECTING) {
        if (!(events & IOEvents::WRITABLE)) return; // not ready yet

        int err = 0;
        socklen_t len = sizeof(err);
        if (getsockopt(get_fd(), SOL_SOCKET, SO_ERROR, &err, &len) < 0 || err != 0) {
            done(); // connect() failed
            return;
        }

        state_ = SessionState::CONNECTED;
        send_hello();
        return;
    }

    auto [status, data] = drain_tcp(get_fd());
    if (status == DrainStatus::CLOSED || status == DrainStatus::ERROR) {
        done();
        return;
    }
    //if (data.empty()) return;

    parser_.feed(data);
    Message message{};
    while (parser_.next(message)) {
        on_message(message);
    }

    if (events & IOEvents::CLOSED) {
        done();
        return;
    }

}

int TCPSession::get_fd() {
    return fd_.get();
}

void TCPSession::on_message(const Message& message) {
    switch (static_cast<MessageType>(message.header.type)) {
        case MessageType::hello: {
            if (state_ != SessionState::CONNECTED) break;
            Hello hello = MessageCodec::decode_hello(message.payload);
            peer_.node_id = hello.node_id;
            peer_.tcp_port = hello.tcp_port;
            peer_.udp_port = hello.udp_port;
            printf("Peer connected - node_id: %lu\n", peer_.node_id);
            send_hello_ack();
            state_ = SessionState::READY;
            break;
        }
        case MessageType::hello_ack: {
            if (state_ != SessionState::HANDSHAKING) break;
            printf("handshake complete - session ready\n");
            state_ = SessionState::READY;
            send_ping();
            break;
        }
        case MessageType::ping: {
            if (state_ != SessionState::READY) break;
            Ping ping = MessageCodec::decode_ping(message.payload);
            send_pong(ping.nonce);
            printf("ping received - sending pong (nonce: %lu)\n", ping.nonce);
            break;
        }
        case MessageType::pong: {
            if (state_ != SessionState::READY) break;
            Pong pong = MessageCodec::decode_pong(message.payload);
            printf("pong received (nonce: %lu)\n", pong.nonce);
            break;
        }
        case MessageType::bye:
            done();
            break;
        default:
            break;
    }
}

void TCPSession::send_hello() {
    Hello hello{};

    hello.node_id = node_id_;
    hello.tcp_port = self_tcp_port_;
    hello.udp_port = self_udp_port_;
    auto payload = MessageCodec::encode_hello(hello);

    MessageHeader header{};
    header.magic = MessageHeader::MAGIC;
    header.version = 1;
    header.type = static_cast<uint8_t>(MessageType::hello);
    header.length = static_cast<uint32_t>(payload.size());
    header.session_id = 0;
    header.request_id = 0;

    Message message{};
    message.header = header;
    message.payload = payload;

    send_all(get_fd(), message);

    state_ = SessionState::HANDSHAKING;
}

void TCPSession::send_hello_ack() {
    MessageHeader header{};
    header.magic = MessageHeader::MAGIC;
    header.version = 1;
    header.type = static_cast<uint8_t>(MessageType::hello_ack);
    header.length = 0;
    header.session_id = 0;
    header.request_id = 0;

    Message message{};
    message.header = header;

    send_all(get_fd(), message);
}

void TCPSession::send_ping() {
    Ping ping{};

    static std::mt19937_64 rng(std::random_device{}());
    ping.nonce = rng();
    auto payload = MessageCodec::encode_ping(ping);

    MessageHeader header{};
    header.magic = MessageHeader::MAGIC;
    header.version = 1;
    header.type = static_cast<uint8_t>(MessageType::ping);
    header.length = static_cast<uint32_t>(payload.size());
    header.session_id = 0;
    header.request_id = 0;

    Message message{};
    message.header = header;
    message.payload = payload;

    send_all(get_fd(), message);
}

void TCPSession::send_pong(uint64_t nonce) {
    Pong pong{};
    pong.nonce = nonce;
    auto payload = MessageCodec::encode_pong(pong);

    MessageHeader header{};
    header.magic = MessageHeader::MAGIC;
    header.version = 1;
    header.type = static_cast<uint8_t>(MessageType::pong);
    header.length = static_cast<uint32_t>(payload.size());
    header.session_id = 0;
    header.request_id = 0;

    Message message{};
    message.header = header;
    message.payload = payload;

    send_all(get_fd(), message);
}

void TCPSession::on_tick() {
    if (state_ != SessionState::READY) return;
    auto now = std::chrono::steady_clock::now();
    if (now - last_ping_ < std::chrono::seconds(5)) return;
    last_ping_ = now;

    send_ping();
}

