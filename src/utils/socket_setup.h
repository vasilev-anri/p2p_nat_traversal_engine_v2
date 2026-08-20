#pragma once

#include "socket_utils.h"
#include "error_utils.h"

inline void setup_bound_socket(int fd, int port) {
    auto addr = create_address(port);

    throw_on_error(set_reuse_address(fd));
    throw_on_error(bind_socket(fd, addr));
    throw_on_error(set_nonblocking(fd));
}
