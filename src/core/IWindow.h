#pragma once
#include "core/KeyCodes.h"
#include <cstdint>
#include <functional>
#include <vector>

class IWindow {
public:
    using KeyCallback = std::function<void(KeyCode key, KeyModifier mod, bool is_pressed)>;
    using ActiveCallback = std::function<void(bool is_active)>;

    virtual ~IWindow() = default;

    virtual bool is_running() const = 0;
    virtual void close() = 0;
    virtual bool is_active() const = 0;
    virtual void poll_events() = 0;
    virtual int width() const = 0;
    virtual int height() const = 0;
    virtual void present(const std::vector<uint32_t>& buffer, int target_w, int target_h) = 0;

    virtual void set_key_callback(KeyCallback cb) = 0;
    virtual void set_active_callback(ActiveCallback cb) = 0;
    virtual void move_to_left_edge() = 0;
};
