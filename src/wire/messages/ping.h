#pragma once

#include <cstdint>
#include <cstddef>

struct Ping {
    static constexpr size_t MIN_SIZE = 8;

    uint64_t nonce;
};