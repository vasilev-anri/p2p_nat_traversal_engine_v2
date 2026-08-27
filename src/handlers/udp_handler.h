#pragma once

#include "event_handler.h"
#include "../utils/unique_fd.h"
#include "../protocol/messages.h"
#include "../rendezvous/vps_utils.h"


class UDPHandler : public EventHandler {
public:
    using RendezvousCallback = std::function<void(const Notify&)>;              /* NOTIFY arrives from VPS */
    using PunchCallback = std::function<void(uint32_t ip, uint16_t port)>;      /* punch packet arrives from peer */

    UDPHandler(int port);
    void handle_event(uint32_t events) override;
    int get_fd() override;
    int get_port_();

    void send_to(uint32_t ip, uint16_t port, const uint8_t* data, size_t len);
    void set_vps_address(uint32_t ip, uint16_t port);
    void set_rendezvous_callback(RendezvousCallback cb);
    void set_punch_callback(PunchCallback cb);
private:
    UniqueFD fd_;
    int port_;
    VPSEndpoint vps_endpoint_;
    RendezvousCallback rendezvous_callback_;
    PunchCallback punch_callback_;

private:
    void setup();
};
