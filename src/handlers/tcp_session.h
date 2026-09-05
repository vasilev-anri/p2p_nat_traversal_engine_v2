#pragma once
#include <chrono>
#include <cstdint>

#include "event_handler.h"
#include "../session/session_state.h"
#include "../utils/unique_fd.h"
#include "../wire/msg_parser.h"
#include "../peer/peer.h"
#include "../session/session_role.h"

class TCPSession : public EventHandler {
public:
    TCPSession(UniqueFD fd, SessionRole role_, uint64_t node_id, uint16_t tcp_port, uint16_t udp_port, SessionState initial = SessionState::CONNECTED);
    void handle_event(uint32_t events) override;
    int get_fd() override;

    void on_message(const Message& message);

    void send_hello();
    void send_hello_ack();
    void send_ping();
    void send_pong(uint64_t nonce);

    void on_tick() override;

private:
    UniqueFD fd_;
    MessageParser parser_;

    SessionState state_;
    Peer peer_;

    SessionRole role_;

    uint64_t node_id_;
    uint16_t self_tcp_port_;
    uint16_t self_udp_port_;

    std::chrono::steady_clock::time_point last_ping_;
};
