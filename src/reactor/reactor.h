#pragma once

#include <memory>
#include <unordered_map>

#include "../handlers/event_handler.h"
#include "../utils/unique_fd.h"


class Reactor {
public:
    static constexpr size_t MAX_TRACKED_SESSIONS = 256;

    Reactor();
    void register_handler(std::unique_ptr<EventHandler> handler, bool want_write = false);
    void unregister_handler(int fd);
    void modify_handler(int fd, bool want_write);
    void handle_events();
    bool has_capacity() const;
private:
    static uint32_t translate_events(uint32_t epoll_events);
private:
    UniqueFD epfd_;
    std::unordered_map<int, std::unique_ptr<EventHandler>> handlers_;
    size_t session_count_ = 0;
};
