#pragma once

#include <cstdint>


namespace IOEvents {
    constexpr uint32_t READABLE  = 1 << 0;
    constexpr uint32_t WRITABLE  = 1 << 1;
    constexpr uint32_t CLOSED    = 1 << 2;
    constexpr uint32_t ERROR     = 1 << 3;
}
