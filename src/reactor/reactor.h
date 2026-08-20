#pragma once

#include <memory>
#include <unordered_map>

#include "../handlers/event_handler.h"
#include "../utils/unique_fd.h"


class Reactor {
public:
    Reactor();
    void register_handler(std::unique_ptr<EventHandler> handler, bool want_write = false);
    void unregister_handler(int fd);
    void handle_events();
private:
    static uint32_t translate_events(uint32_t epoll_events);
private:
    UniqueFD epfd_;
    std::unordered_map<int, std::unique_ptr<EventHandler>> handlers_;
};
