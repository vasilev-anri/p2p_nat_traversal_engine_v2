#pragma once

#include <cstring>
#include <string>
#include <stdexcept>


struct SyscallError {
    int code;               // errno
    std::string what;       // context
    [[nodiscard]] std::string message() const {
        return what + " failed: " + strerror(code);
    }
    static SyscallError from_errno(std::string_view what) {
        return {errno, std::string(what)};
    }
};

template <typename T>
void throw_on_error(T&& result) {
    if (!result) throw std::runtime_error(result.error().message());
}


