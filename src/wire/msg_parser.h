#pragma once

#include <cstdint>
#include <span>
#include <vector>

#include "msg.h"


class MessageParser {
public:
    /// Append raw bytes to internal buffer
    void feed(std::span<const uint8_t> data);

    /// Extract next complete message if available.
    /// Returns true and fills out or returns false if more bytes are needed
    bool next(Message& out);
private:
    std::vector<uint8_t> buf_;
};
