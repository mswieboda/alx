#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include "Game.h"

struct Camera {
    float x = 0.0f;
    float y = 0.0f;
    float lerp_speed = 14.0f; // Smooth GBA-style camera follow speed
    bool initialized = false;

    // Optional target tracking (pointers to world coordinates, e.g. player center_x, center_y)
    float target_x = 0.0f;
    float target_y = 0.0f;
    bool has_target = false;

    // Optional boundary limits
    float limit_left   = 0.0f;
    float limit_top    = 0.0f;
    float limit_right  = 0.0f;
    float limit_bottom = 0.0f;
    bool  has_limits   = false;

    // Optional deadzone tracking (inner viewport box where player movement does not shift the camera)
    float deadzone_w = 0.5f * Game::WIDTH; // 30% of 320px width (6 tiles wide)
    float deadzone_h = 0.5f * Game::HEIGHT; // 30% of 240px height (4.5 tiles high)
    bool use_deadzone = true;

    void follow(float target_center_x, float target_center_y) {
        target_x = target_center_x;
        target_y = target_center_y;
        has_target = true;
    }

    void clear_target() {
        has_target = false;
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

    void set_deadzone(float width, float height) {
        deadzone_w = width;
        deadzone_h = height;
        use_deadzone = true;
    }

    void clear_deadzone() {
        use_deadzone = false;
    }

    // Optional manual pan offset (used during PanMode scouting)
    float pan_offset_x = 0.0f;
    float pan_offset_y = 0.0f;
    float pan_anchor_x = 0.0f;
    float pan_anchor_y = 0.0f;
    bool is_panning_active = false;

    void set_pan_offset(float offset_x, float offset_y) {
        pan_offset_x = offset_x;
        pan_offset_y = offset_y;
    }

    void start_panning() {
        if (!is_panning_active) {
            pan_anchor_x = x;
            pan_anchor_y = y;
            is_panning_active = true;
        }
    }

    void stop_panning() {
        is_panning_active = false;
    }

    void update(float viewport_width = static_cast<float>(Game::WIDTH),
                float viewport_height = static_cast<float>(Game::HEIGHT))
    {
        // center viewport on target or apply deadzone boundary tracking
        if (has_target) {
            float effective_target_x = target_x + pan_offset_x;
            float effective_target_y = target_y + pan_offset_y;

            if (is_panning_active) {
                x = pan_anchor_x + pan_offset_x;
                y = pan_anchor_y + pan_offset_y;
            } else if (use_deadzone) {
                float half_vw = viewport_width / 2.0f;
                float half_vh = viewport_height / 2.0f;
                float half_dw = deadzone_w / 2.0f;
                float half_dh = deadzone_h / 2.0f;

                float target_rel_x = effective_target_x - x - half_vw;
                float target_rel_y = effective_target_y - y - half_vh;

                if (target_rel_x < -half_dw) {
                    x = effective_target_x - half_vw + half_dw;
                } else if (target_rel_x > half_dw) {
                    x = effective_target_x - half_vw - half_dw;
                }

                if (target_rel_y < -half_dh) {
                    y = effective_target_y - half_vh + half_dh;
                } else if (target_rel_y > half_dh) {
                    y = effective_target_y - half_vh - half_dh;
                }
            } else {
                x = effective_target_x - (viewport_width / 2.0f);
                y = effective_target_y - (viewport_height / 2.0f);
            }
        }

        // clamp viewport position inside limits (expanded by pan_offset during camera scouting)
        if (has_limits) {
            float min_x = limit_left + std::min(0.0f, pan_offset_x);
            float max_x = std::max(limit_left, limit_right - viewport_width) + std::max(0.0f, pan_offset_x);
            float min_y = limit_top + std::min(0.0f, pan_offset_y);
            float max_y = std::max(limit_top, limit_bottom - viewport_height) + std::max(0.0f, pan_offset_y);

            x = std::clamp(x, min_x, max_x);
            y = std::clamp(y, min_y, max_y);
        }
    }

    int to_screen_x(float world_x) const {
        return static_cast<int>(std::round(world_x - x));
    }

    int to_screen_y(float world_y) const {
        return static_cast<int>(std::round(world_y - y));
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
