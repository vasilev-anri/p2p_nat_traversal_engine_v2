#include "msg_codec.h"
#include "msg_types.h"
#include "../protocol/codec_utils.h"

std::vector<uint8_t> MessageCodec::encode_header(const MessageHeader& header) {
    std::vector<uint8_t> res;
    res.reserve(MessageHeader::HEADER_SIZE);

    write_u32(MessageHeader::MAGIC, res);
    write_u8(header.version, res);
    write_u8(header.type, res);
    write_u32(header.length, res);
    write_u32(header.session_id, res);
    write_u32(header.request_id, res);

    return res;
}

MessageHeader MessageCodec::decode_header(std::span<const uint8_t> buf) {
    if (buf.size() != MessageHeader::HEADER_SIZE) throw std::runtime_error("Message header size mismatch");

    size_t offset = 0;
    MessageHeader header{};

    const auto magic = read_u32(buf, offset);
    // MAGIC mismatch - not our protocol; drop immediately
    if (magic != MessageHeader::MAGIC) throw std::runtime_error("Invalid message header");

    const auto version = read_u8(buf, offset);
    const auto type = read_u8(buf, offset);

    switch (static_cast<MessageType>(type)) {
        case MessageType::hello:
        case MessageType::hello_ack:
        case MessageType::bye:
        case MessageType::ping:
        case MessageType::pong:
        case MessageType::offer:
        case MessageType::answer:
        case MessageType::candidate:
        case MessageType::error:
        case MessageType::ext:
            break;
        default:
            throw std::runtime_error("Invalid Header type");
    }

    const auto length = read_u32(buf, offset);
    // length guard - prevent memory exhaustion from malformed messages
    if (length > MessageHeader::MAX_PAYLOAD) throw std::runtime_error("Invalid header length");

    const auto session_id = read_u32(buf, offset);
    const auto request_id = read_u32(buf, offset);

    header.magic = magic;
    header.version = version;
    header.type = type;
    header.length = length;
    header.session_id = session_id;
    header.request_id = request_id;

    return header;
}

std::vector<uint8_t> MessageCodec::encode_hello(const Hello& hello) {
    std::vector<uint8_t> res;
    res.reserve(Hello::MIN_SIZE);

    write_u64(hello.node_id, res);
    write_u16(hello.tcp_port, res);
    write_u16(hello.udp_port, res);

    return res;
}

Hello MessageCodec::decode_hello(std::span<const uint8_t> buf) {
    // size guard - process valid hello message only
    if (buf.size() < Hello::MIN_SIZE) throw std::runtime_error("Invalid Hello message size");

    Hello hello{};
    size_t offset = 0;

    const auto node_id = read_u64(buf, offset);
    const auto tcp_port = read_u16(buf, offset);
    const auto udp_port = read_u16(buf, offset);

    hello.node_id = node_id;
    hello.tcp_port = tcp_port;
    hello.udp_port = udp_port;

    return hello;
}

std::vector<uint8_t> MessageCodec::encode_ping(const Ping& ping) {
    std::vector<uint8_t> res;
    res.reserve(Ping::MIN_SIZE);

    write_u64(ping.nonce, res);

    return res;
}

Ping MessageCodec::decode_ping(std::span<const uint8_t> buf) {
    if (buf.size() < Ping::MIN_SIZE) throw std::runtime_error("Invalid Ping Message size");
    Ping ping{};
    size_t offset = 0;
    const auto nonce = read_u64(buf, offset);
    ping.nonce = nonce;
    return ping;
}

std::vector<uint8_t> MessageCodec::encode_pong(const Pong& pong) {
    std::vector<uint8_t> res;
    res.reserve(Ping::MIN_SIZE);

    write_u64(pong.nonce, res);

    return res;
}

Pong MessageCodec::decode_pong(std::span<const uint8_t> buf) {
    if (buf.size() < Pong::MIN_SIZE) throw std::runtime_error("Invalid Pong Message size");
    Pong pong{};
    size_t offset = 0;
    const auto nonce = read_u64(buf, offset);
    pong.nonce = nonce;
    return pong;
}

