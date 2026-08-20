#pragma once

#include <unistd.h>


class UniqueFD {
public:
    UniqueFD() = default;

    explicit UniqueFD(int fd) : fd_(fd) {}
    UniqueFD(const UniqueFD&) = delete;
    UniqueFD& operator=(const UniqueFD&) = delete;
    UniqueFD(UniqueFD&& other) noexcept : fd_(other.fd_) { other.fd_ = -1; }
    UniqueFD& operator=(UniqueFD&& other) noexcept {
        if (this != &other) {
            reset();
            fd_ = other.fd_;
            other.fd_ = -1;
        }
        return *this;
    }

    ~UniqueFD() noexcept { reset(); };

    [[nodiscard]] bool is_valid() const noexcept { return fd_ != -1; }
    explicit operator bool() const noexcept { return is_valid(); }
    [[nodiscard]] int release() noexcept {
        int fd = fd_;
        fd_ = -1;
        return fd;
    }
    void reset(int new_fd = -1) noexcept {
        if (fd_ != -1 && fd_ != new_fd) ::close(fd_);
        fd_ = new_fd;
    }
    [[nodiscard]] int get() const noexcept { return fd_; }

private:
    int fd_ = -1;
};
