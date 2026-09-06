#pragma once

#include "event_handler.h"
#include "../utils/unique_fd.h"
#include "../protocol/messages.h"
#include "../rendezvous/punch_target.h"
#include "../rendezvous/vps_utils.h"


class UDPHandler : public EventHandler {
public:
    using RendezvousCallback = std::function<void(const Notify&)>;              /* NOTIFY arrives from VPS */
    using PunchCallback = std::function<void(uint32_t ip, uint16_t port)>;      /* punch packet arrives from peer */

    UDPHandler(int port);
    void handle_event(uint32_t events) override;
    void on_tick() override;
    int get_fd() override;
    int get_port_();

    void send_to(uint32_t ip, uint16_t port, const uint8_t* data, size_t len);
    void set_vps_address(uint32_t ip, uint16_t port);
    void set_rendezvous_callback(RendezvousCallback cb);
    void set_punch_callback(PunchCallback cb);

    void setup_punch(uint64_t node_id, uint32_t public_ip, uint16_t public_port, uint32_t private_ip, uint16_t private_port);

    void mark_punch_success(uint32_t ip, uint16_t port);

private:
    UniqueFD fd_;
    int port_;
    VPSEndpoint vps_endpoint_;
    RendezvousCallback rendezvous_callback_;
    PunchCallback punch_callback_;
    std::vector<PunchTarget> punch_targets_;

    static constexpr std::string_view punch_msg_ = "PUNCH";

private:
    void setup();
};
