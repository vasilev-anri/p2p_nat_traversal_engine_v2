#include "reactor.h"

#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>

#include "../utils/io_events.h"
#include "../utils/socket_utils.h"

Reactor::Reactor() {
    auto result = create_epoll_fd();
    if (!result) throw std::runtime_error(result.error().message());
    epfd_ = std::move(*result);
}

void Reactor::register_handler(std::unique_ptr<EventHandler> handler, bool want_write) {
    int fd = handler->get_fd();

    handler->set_done_callback([this, fd]() { unregister_handler(fd); });

    epoll_event ev{};
    ev.data.ptr= handler.get();
    ev.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
    if (want_write) ev.events |= EPOLLOUT;

    if (auto res = epoll_ctl_add(epfd_.get(), fd, &ev); !res)
        throw std::runtime_error(res.error().message());

    handlers_[fd] = std::move(handler);
}

void Reactor::unregister_handler(int fd) {
    if (auto res = epoll_ctl_del(epfd_.get(), fd); !res)
        fprintf(stderr, "epoll_ctl_del failed: %s\n", res.error().message().c_str());
    handlers_.erase(fd);
}

void Reactor::handle_events() {

    epoll_event events[64];
    auto result = epoll_wait_interruptible(epfd_.get(), events, 64, 5000);
    if (!result) return;
    if (result.value() == 0) {
        for (auto& [fd, handler] : handlers_) {
            handler->on_tick();
        }
        return;
    }

    for (int i = 0; i < result.value(); ++i) {
        auto* handler = static_cast<EventHandler*>(events[i].data.ptr);
        handler->handle_event(translate_events(events[i].events));
    }
}

uint32_t Reactor::translate_events(uint32_t epoll_events) {
    uint32_t result = 0;

    if (epoll_events & EPOLLIN) result |= IOEvents::READABLE;
    if (epoll_events & EPOLLOUT) result |= IOEvents::WRITABLE;
    if (epoll_events & (EPOLLRDHUP | EPOLLHUP)) result |= IOEvents::CLOSED;
    if (epoll_events & EPOLLERR) result |= IOEvents::ERROR;

    return result;
}
