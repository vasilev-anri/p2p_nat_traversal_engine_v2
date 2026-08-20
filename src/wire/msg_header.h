#pragma once

#include <cstdint>
#include <cstddef>


/**
 *  Fixed 18-byte frame header preceding each message
 *
 *  MAGIC 0x50325050     ("P2PP"). Different values mean wrong protocol and are dropped immediately
 *  MAX_PAYLOAD          1 MiB cap - prevents memory exhaustion from malformed length fields
 *  version              currently set to 1
 *  type                 MessageType enum value; Determines how to interpret the payload
 *  length               payload byte count
 *  session_id           identifies the logical session this message belongs to
 *  request_id           correlates requests with responses (e.g. ping -> pong)
 */

struct MessageHeader {
    static constexpr uint32_t MAGIC = 0x50325050;
    static constexpr uint32_t MAX_PAYLOAD = 1 << 20;
    static constexpr size_t HEADER_SIZE = 18;

    uint32_t magic;
    uint8_t version;
    uint8_t type;

    uint32_t length;
    uint32_t session_id;
    uint32_t request_id;
};
