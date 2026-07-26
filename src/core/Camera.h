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
    float pre_pan_x = 0.0f;
    float pre_pan_y = 0.0f;
    float anchor_blend = 0.0f;
    bool is_panning_active = false;
    bool has_ever_wasd_panned = false;

    bool is_panning_or_decaying() const {
        return is_panning_active || pan_offset_x != 0.0f || pan_offset_y != 0.0f || zoom != 1.0f;
    }

    void set_pan_offset(float offset_x, float offset_y) {
        pan_offset_x = offset_x;
        pan_offset_y = offset_y;
    }

    void start_panning(float viewport_width = static_cast<float>(Game::WIDTH),
                       float viewport_height = static_cast<float>(Game::HEIGHT)) {
        if (!is_panning_active) {
            float half_vw_base = viewport_width / 2.0f;
            float half_vh_base = viewport_height / 2.0f;
            pre_pan_x = x;
            pre_pan_y = y;
            pan_anchor_x = x + half_vw_base;
            pan_anchor_y = y + half_vh_base;
            anchor_blend = 0.0f;
            has_ever_wasd_panned = false;
            is_panning_active = true;
        }
    }

    float pan_speed = 2.0f;
    float return_speed = 6.0f;

    void update_anchor_blend(float dt, bool has_wasd_input) {
        if (has_wasd_input) {
            has_ever_wasd_panned = true;
        }
        
        // ONLY blend towards player center if WASD was actually used to scout!
        if (has_ever_wasd_panned && !has_wasd_input) {
            anchor_blend += (1.0f - anchor_blend) * (1.0f - std::exp(-return_speed * dt));
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
            float half_vw = (viewport_width / 2.0f) / zoom;
            float half_vh = (viewport_height / 2.0f) / zoom;

            float effective_target_x = target_x + pan_offset_x;
            float effective_target_y = target_y + pan_offset_y;

            if (is_panning_or_decaying()) {
                if (!has_ever_wasd_panned) {
                    // Tactical Radar Peek: Keep camera anchored to exact pre-pan resting position
                    float center_x = pan_anchor_x;
                    float center_y = pan_anchor_y;
                    x = center_x - half_vw;
                    y = center_y - half_vh;
                } else {
                    // Active Scouting: Blend anchor point from initial screen center (anchor_blend = 0) to player center (anchor_blend = 1)
                    float current_anchor_x = (1.0f - anchor_blend) * pan_anchor_x + anchor_blend * target_x;
                    float current_anchor_y = (1.0f - anchor_blend) * pan_anchor_y + anchor_blend * target_y;

                    float center_x = current_anchor_x + pan_offset_x;
                    float center_y = current_anchor_y + pan_offset_y;

                    float max_pan_x = Game::TILE_SIZE * 12.0f; // 12 tiles X
                    float max_pan_y = Game::TILE_SIZE * 9.0f;  // 9 tiles Y (4:3 aspect ratio)

                    center_x = std::clamp(center_x, target_x - max_pan_x, target_x + max_pan_x);
                    center_y = std::clamp(center_y, target_y - max_pan_y, target_y + max_pan_y);

                    x = center_x - half_vw;
                    y = center_y - half_vh;
                }
            } else if (use_deadzone) {
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
                x = effective_target_x - half_vw;
                y = effective_target_y - half_vh;
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

        // Quantize camera world position to exact integer pixels
        x = std::round(x);
        y = std::round(y);
    }

    float zoom = 1.0f;        // 1.0f = 1x normal, 0.5f = 2x zoom out
    float target_zoom = 1.0f; // Target zoom factor

    int to_screen_x(float world_x) const {
        return static_cast<int>(std::round((world_x - x) * zoom));
    }

    int to_screen_y(float world_y) const {
        return static_cast<int>(std::round((world_y - y) * zoom));
    }

    float to_world_x(float screen_x) const {
        return (screen_x / zoom) + x;
    }

    float to_world_y(float screen_y) const {
        return (screen_y / zoom) + y;
    }

    float get_x() const { return x; }
    float get_y() const { return y; }
};
