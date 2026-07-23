#pragma once
#include <algorithm>
#include <cstdint>
#include "Game.h"

struct Camera {
    float x = 0.0f;
    float y = 0.0f;

    // Optional target tracking (pointers to world coordinates, e.g. player center_x, center_y)
    const float* target_x = nullptr;
    const float* target_y = nullptr;

    // Optional boundary limits
    float limit_left   = 0.0f;
    float limit_top    = 0.0f;
    float limit_right  = 0.0f;
    float limit_bottom = 0.0f;
    bool  has_limits   = false;

    void follow(const float* tx, const float* ty) {
        target_x = tx;
        target_y = ty;
    }

    void clear_target() {
        target_x = nullptr;
        target_y = nullptr;
    }

    void set_limits(float left, float top, float right, float bottom) {
        limit_left   = left;
        limit_top    = top;
        limit_right  = right;
        limit_bottom = bottom;
        has_limits   = true;
    }

    void clear_limits() {
        has_limits = false;
    }

    void update(float viewport_width = static_cast<float>(Game::WIDTH),
                float viewport_height = static_cast<float>(Game::HEIGHT))
    {
        // 1. If target pointers are set, center viewport on target
        if (target_x && target_y) {
            x = (*target_x) - (viewport_width / 2.0f);
            y = (*target_y) - (viewport_height / 2.0f);
        }

        // 2. If limits are enabled, clamp viewport position inside limits
        if (has_limits) {
            float max_x = std::max(limit_left, limit_right - viewport_width);
            float max_y = std::max(limit_top, limit_bottom - viewport_height);
            x = std::clamp(x, limit_left, max_x);
            y = std::clamp(y, limit_top, max_y);
        }
    }

    int to_screen_x(float world_x) const {
        return static_cast<int>(world_x - x);
    }

    int to_screen_y(float world_y) const {
        return static_cast<int>(world_y - y);
    }

    float to_world_x(float screen_x) const {
        return screen_x + x;
    }

    float to_world_y(float screen_y) const {
        return screen_y + y;
    }

    float get_x() const { return x; }
    float get_y() const { return y; }
};
