#pragma once

#include <memory>

#include "vps_utils.h"
#include "../handlers/udp_handler.h"
#include "../protocol/messages.h"



class RendezvousClient {
public:
    using NotifyCallback = std::function<void(Endpoint public_endpoint, Endpoint private_endpoint)>;

    RendezvousClient(UDPHandler& udp, uint64_t node_id, uint32_t vps_ip, uint16_t vps_port);

    void send_register();
    void send_keep_alive();
    void send_request(uint64_t target_node);
    void handle_notify(Notify*);

    void set_notify_callback(NotifyCallback cb);

private:
    UDPHandler& udp_;
    uint64_t node_id_;
    VPSEndpoint vps_endpoint_{};
    NotifyCallback notify_callback_;
};
