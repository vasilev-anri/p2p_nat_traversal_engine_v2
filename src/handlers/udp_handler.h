#pragma once

#include "event_handler.h"
#include "../utils/unique_fd.h"


class UDPHandler : public EventHandler {
public:
    UDPHandler(int port);
    void handle_event(uint32_t events) override;
    int get_fd() override;
private:
    UniqueFD fd_;
    int port_;

private:
    void setup();
};
