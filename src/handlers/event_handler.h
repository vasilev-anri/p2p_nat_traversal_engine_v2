#pragma once

#include <functional>
#include <cstdint>

class EventHandler {
public:
    using DoneCallback = std::function<void()>;

    void set_done_callback(DoneCallback cb) {
        on_done_ = std::move(cb);
    }

    virtual ~EventHandler() = default;
    virtual void handle_event(uint32_t events) = 0;
    virtual int get_fd() = 0;
    virtual void on_tick() {};

protected:
    void done() {
        if (on_done_) on_done_();
    }

private:
    DoneCallback on_done_;
};