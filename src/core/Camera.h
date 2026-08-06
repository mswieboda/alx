#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include "Game.h"

namespace core {

struct Camera {
    float x = 0.0f;
    float y = 0.0f;
    float lerp_speed = 16.0f; // Smooth GBA-style camera follow speed
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
    float deadzone_w = 0.5f * Game::WIDTH; // 50% of 320px width
    float deadzone_h = 0.5f * Game::HEIGHT; // 50% of 240px height
    bool use_deadzone = true;

    float zoom = 1.0f; // 1.0f = 1x normal, 0.75f = 3/4 scale

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

    void update(float dt = 1.0f / 60.0f,
                float viewport_width = static_cast<float>(Game::WIDTH),
                float viewport_height = static_cast<float>(Game::HEIGHT))
    {
        if (!has_target) return;

        float half_vw = (viewport_width / 2.0f) / zoom;
        float half_vh = (viewport_height / 2.0f) / zoom;

        float desired_x = x;
        float desired_y = y;

        if (use_deadzone) {
            float half_dw = deadzone_w / 2.0f;
            float half_dh = deadzone_h / 2.0f;

            float target_rel_x = target_x - x - half_vw;
            float target_rel_y = target_y - y - half_vh;

            desired_x = x;
            if (target_rel_x < -half_dw) {
                desired_x = target_x - half_vw + half_dw;
            } else if (target_rel_x > half_dw) {
                desired_x = target_x - half_vw - half_dw;
            }

            desired_y = y;
            if (target_rel_y < -half_dh) {
                desired_y = target_y - half_vh + half_dh;
            } else if (target_rel_y > half_dh) {
                desired_y = target_y - half_vh - half_dh;
            }
        } else {
            desired_x = target_x - half_vw;
            desired_y = target_y - half_vh;
        }

        // Clamp desired destination to map limits BEFORE lerp to ensure smooth deceleration at borders
        if (has_limits) {
            float max_x = std::max(limit_left, limit_right - viewport_width);
            float max_y = std::max(limit_top, limit_bottom - viewport_height);

            desired_x = std::clamp(desired_x, limit_left, max_x);
            desired_y = std::clamp(desired_y, limit_top, max_y);
        }

        // Smoothly lerp camera position towards clamped desired destination
        if (!initialized) {
            x = desired_x;
            y = desired_y;
            initialized = true;
        } else {
            float t = 1.0f - std::exp(-lerp_speed * dt);
            x += (desired_x - x) * t;
            y += (desired_y - y) * t;
        }
    }

    // Converts world-space coordinates to integer screen pixels using arithmetic half-up rounding
    // (std::floor(val + 0.5f)). This provides two key properties:
    // 1. Sub-Pixel Smoothness: Floating-point entity transforms (Player, Enemies, FX) round smoothly
    //    to the nearest screen pixel as they move in fractional sub-pixel increments.
    // 2. Translation Invariance: Satisfies f(v + K) == f(v) + K for all integer tile widths K,
    //    preventing negative-coordinate origin tearing on tile maps.
    int to_screen_x(float world_x) const {
        return static_cast<int>(std::floor((world_x - x) * zoom + 0.5f));
    }

    int to_screen_y(float world_y) const {
        return static_cast<int>(std::floor((world_y - y) * zoom + 0.5f));
    }

    float to_world_x(float screen_x) const {
        return (screen_x / zoom) + x;
    }

    static constexpr float DEFAULT_CULL_MARGIN_PX = 32.0f;
    static constexpr int DEFAULT_CULL_TILE_MARGIN = 2;

    [[nodiscard]] bool is_aabb_visible(
        float obj_x, float obj_y, float obj_w, float obj_h,
        float margin_px = DEFAULT_CULL_MARGIN_PX,
        float viewport_width = static_cast<float>(Game::WIDTH),
        float viewport_height = static_cast<float>(Game::HEIGHT)
    ) const noexcept {
        float view_w = viewport_width / zoom;
        float view_h = viewport_height / zoom;

        float min_cam_x = x - margin_px;
        float max_cam_x = x + view_w + margin_px;
        float min_cam_y = y - margin_px;
        float max_cam_y = y + view_h + margin_px;

        return (obj_x < max_cam_x) && (obj_x + obj_w > min_cam_x) &&
               (obj_y < max_cam_y) && (obj_y + obj_h > min_cam_y);
    }

    void get_tile_bounds(
        int map_w, int map_h, int tile_size,
        int& out_min_tx, int& out_max_tx, int& out_min_ty, int& out_max_ty,
        int tile_margin = DEFAULT_CULL_TILE_MARGIN,
        float viewport_width = static_cast<float>(Game::WIDTH),
        float viewport_height = static_cast<float>(Game::HEIGHT)
    ) const noexcept {
        if (tile_size <= 0) return;
        float view_w = viewport_width / zoom;
        float view_h = viewport_height / zoom;

        float margin_px = static_cast<float>(tile_margin * tile_size);

        out_min_tx = std::max(0, static_cast<int>(std::floor((x - margin_px) / static_cast<float>(tile_size))));
        out_max_tx = std::min(map_w - 1, static_cast<int>(std::ceil((x + view_w + margin_px) / static_cast<float>(tile_size))));
        out_min_ty = std::max(0, static_cast<int>(std::floor((y - margin_px) / static_cast<float>(tile_size))));
        out_max_ty = std::min(map_h - 1, static_cast<int>(std::ceil((y + view_h + margin_px) / static_cast<float>(tile_size))));
    }

    float to_world_y(float screen_y) const {
        return (screen_y / zoom) + y;
    }
};

} // namespace core

using Camera = core::Camera;
