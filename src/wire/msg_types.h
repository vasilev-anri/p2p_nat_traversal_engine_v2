#pragma once

#include <cstdint>


/**
 *  hello, hello_ack - hello handshake for identity exchange
 *  bye              - shutdown
 *
 *  ping, pong       - testing connection status
 *
 *  offer, answer    - SDP negotiation for P2P connection details
 *  candidate        - ICE network paths (IP, Port) for NAT traversal
 *
 *  error            - error occurred;  carries error code in the payload
 *  ext              - extension point; carries subtype in the payload
 *
 */
enum class MessageType : uint8_t {
    hello       = 0x01,
    hello_ack   = 0x02,
    bye         = 0x03,

    ping        = 0x10,
    pong        = 0x11,

    offer       = 0x20,
    answer      = 0x21,
    candidate   = 0x22,

    error       = 0x7E,
    ext         = 0x7F,
};
