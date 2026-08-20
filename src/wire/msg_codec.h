#pragma once

#include <cstdint>
#include <span>
#include <stdexcept>
#include <vector>

#include "msg_header.h"
#include "messages/hello.h"
#include "messages/ping.h"
#include "messages/pong.h"


class MessageCodec {
public:
    /// Encode the 18-byte message header into network byte order.
    static std::vector<uint8_t> encode_header(const MessageHeader&);

    /// Decode 18 bytes into a MessageHeader.
    /// Throws errors on invalid MAGIC, unknown message type, invalid length.
    static MessageHeader decode_header(std::span<const uint8_t>);

    /// Encode the "Hello" message (minimum size: 12 bytes) in network byte order.
    static std::vector<uint8_t> encode_hello(const Hello&);

    /// Decode at least 12 bytes into Hello message.
    /// Throws error on invalid message size.
    static Hello decode_hello(std::span<const uint8_t>);

    static std::vector<uint8_t> encode_ping(const Ping&);

    static Ping decode_ping(std::span<const uint8_t>);

    static std::vector<uint8_t> encode_pong(const Pong&);

    static Pong decode_pong(std::span<const uint8_t>);

};
