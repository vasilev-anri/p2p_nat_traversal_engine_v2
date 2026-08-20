#pragma once

#include <cstdint>
#include <span>
#include <vector>
#include <stdexcept>


/**
 * Write an integer in big-endian network byte order - most significant byte first.
 *
 * Takes an unsigned integer and appends its bytes to the output vector.
 *
 * @tparam T  uint8_t, uint16_t, uint32_t, or uint64_t - enforced at compile time
 * @param val  value to serialize
 * @param out  output buffer to append bytes to
 */
template <class T>
static void write_integral_be(T val, std::vector<uint8_t>& out) {
    static_assert(std::is_unsigned_v<T>, "Unsigned type required");
    static_assert(
        sizeof(T) == 1 || sizeof(T) == 2 || sizeof(T) == 4 || sizeof(T) == 8,
        "Only fixed-width in size (1, 2, 4, 8) bytes supported"
    );
    const size_t n = sizeof(T);
    const size_t start = out.size();
    out.resize(start + n);
    for (size_t i = 0; i < n; ++i) {
        out[start + i] = static_cast<uint8_t>(val >> (n - i - 1) * 8);
    }
}

/**
 * Read an integer in big-endian network byte order - most significant byte first.
 *
 * Reconstructs an integer from n bytes in buf starting at offset.
 *
 * @tparam T  uint8_t, uint16_t, uint32_t, or uint64_t - enforced at compile time
 * @param buf  source byte buffer in network byte order
 * @param offset  read cursor - advanced by sizeof(T) after reading
 * @return  reconstructed integer value
 */
template <class T>
static T read_integral_be(std::span<const uint8_t> buf, size_t& offset) {
    static_assert(std::is_unsigned_v<T>, "Unsigned type required");
    static_assert(
        sizeof(T) == 1 || sizeof(T) == 2 || sizeof(T) == 4 || sizeof(T) == 8,
        "Only fixed-width in size (1, 2, 4, 8) bytes supported"
    );
    const size_t n = sizeof(T);
    if (offset + n > buf.size()) throw std::out_of_range("Buffer underflow");

    T res = 0;
    for (size_t i = 0; i < n; ++i) {
        res <<= 8;
        res |= static_cast<T>(buf[offset + i]);
    }
    offset += n;
    return res;
}


inline void write_u8(uint8_t val, std::vector<uint8_t>& out) { write_integral_be(val, out); }
inline void write_u16(uint16_t val, std::vector<uint8_t>& out) { write_integral_be(val, out); }
inline void write_u32(uint32_t val, std::vector<uint8_t>& out) { write_integral_be(val, out); }
inline void write_u64(uint64_t val, std::vector<uint8_t>& out) { write_integral_be(val, out); }

inline uint8_t read_u8(std::span<const uint8_t> buf, size_t& offset) { return read_integral_be<uint8_t>(buf, offset); }
inline uint16_t read_u16(std::span<const uint8_t> buf, size_t& offset) { return read_integral_be<uint16_t>(buf, offset); }
inline uint32_t read_u32(std::span<const uint8_t> buf, size_t& offset) { return read_integral_be<uint32_t>(buf, offset); }
inline uint64_t read_u64(std::span<const uint8_t> buf, size_t& offset) { return read_integral_be<uint64_t>(buf, offset); }

