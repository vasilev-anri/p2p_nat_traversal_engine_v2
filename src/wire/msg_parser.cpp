#include "msg_parser.h"

#include "msg_codec.h"


/**
 * Appends raw received bytes to the local buffer
 *
 * @param data  Raw data acquired from draining the socket
 */
void MessageParser::feed(std::span<const uint8_t> data) {
    buf_.insert(buf_.end(), data.begin(), data.end());
}

/**
 * Attempts to extract the next complete message from the local buffer.
 * Fills the Message structure using the raw bytes extracted from the local buffer.
 *
 * @param out  Message to be filled, constructed by the caller
 * @return  false, if more bytes are needed, true if a complete message was extracted
 */
bool MessageParser::next(Message& out) {
    if (buf_.size() < MessageHeader::HEADER_SIZE) return false;

    std::span<const uint8_t> header_span(buf_.data(), MessageHeader::HEADER_SIZE);
    MessageHeader header = MessageCodec::decode_header(header_span);

    size_t total = MessageHeader::HEADER_SIZE + header.length;
    if (buf_.size() < total) return false;

    out.header = header;
    out.payload.assign(buf_.begin() + MessageHeader::HEADER_SIZE, buf_.begin() + total);

    buf_.erase(buf_.begin(), buf_.begin() + total);

    return true;
}