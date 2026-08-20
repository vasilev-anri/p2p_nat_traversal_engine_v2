#pragma once

#include <cstdint>
#include <vector>

#include "msg_header.h"


/**
 *  Universal container - wraps every message type
 *
 *
 *  header              - identifies message type & payload size
 *
 *  payload             - type specific raw bytes
 */
struct Message {
    MessageHeader header;
    std::vector<uint8_t> payload;
};