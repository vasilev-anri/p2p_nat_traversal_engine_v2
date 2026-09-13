#pragma once

#include <stdexcept>
#include <string>
#include <cstdlib>

inline std::string load_room_key() {
    const char* room_key = std::getenv("P2P_ROOM_KEY");

    if (room_key == nullptr) throw std::runtime_error("Environment variable P2P_ROOM_KEY not set");

    std::string key(room_key);

    constexpr const char* whitespace = " \t\n\r\f\v";

    key.erase(0, key.find_first_not_of(whitespace));
    key.erase(key.find_last_not_of(whitespace) + 1);

    if (key.empty()) throw std::runtime_error("Environment variable P2P_ROOM_KEY can not be empty");

    return key;
}