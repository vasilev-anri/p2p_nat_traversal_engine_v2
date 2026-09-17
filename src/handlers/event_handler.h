#pragma once

#include <functional>
#include <cstdint>

class EventHandler {
public:
    using DoneCallback = std::function<void()>;
    using WantWriteCallback = std::function<void(bool)>;

    void set_done_callback(DoneCallback cb) {
        on_done_ = std::move(cb);
    }

    void set_want_write_callback(WantWriteCallback cb) {
        on_want_write_ = std::move(cb);
    }

    virtual ~EventHandler() = default;
    virtual void handle_event(uint32_t events) = 0;
    virtual int get_fd() = 0;
    virtual void on_tick() {};

protected:
    void done() {
        if (on_done_) on_done_();
    }

    void want_write(bool wants_write) {
        if (on_want_write_) on_want_write_(wants_write);
    }

private:
    DoneCallback on_done_;
    WantWriteCallback on_want_write_;
};