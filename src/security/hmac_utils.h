#pragma once

#include <cstdint>
#include <cstddef>
#include <optional>
#include <span>
#include <vector>

namespace HMACAuth {
    inline constexpr size_t TAG_SIZE = 32;
    inline constexpr size_t TIMESTAMP_SIZE = 8;
    inline constexpr int64_t MAX_CLOCK_SKEW_SECONDS = 30;

    std::vector<uint8_t> load_secret();
    std::vector<uint8_t> sign(const std::vector<uint8_t>& key, std::vector<uint8_t> data);
    std::optional<std::vector<uint8_t>> verify_and_strip(const std::vector<uint8_t>& key, std::span<const uint8_t> data);
}
