#pragma once

#include <functional>
#include <cstdint>
#include <memory>

#include "event_handler.h"
#include "../utils/unique_fd.h"

class TCPListener : public EventHandler {
public:
    using SessionCallback = std::function<void(std::unique_ptr<EventHandler>)>;

    void set_event_callback(SessionCallback cb);

    TCPListener(int port, uint64_t node_id, uint16_t udp_port);
    void handle_event(uint32_t events) override;
    int get_fd() override;

private:
    UniqueFD listener_;
    int port_;

    uint16_t udp_port_;

    uint64_t node_id_;

    SessionCallback on_accept_;
private:
    void setup();
};
