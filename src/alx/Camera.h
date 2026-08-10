#pragma once
#include <algorithm>
#include <cmath>
#include "core/Camera.h"
#include "alx/Action.h"
#include "Game.h"

namespace alx {

struct Camera : public core::Camera {
    // Peek-ahead constants & speed tuning
    static constexpr float DEEP_PEEK_SPEED = 2.5f; // Snappy lerp speed for manual Q hold deep peek
    static constexpr float AUTO_PEEK_SPEED = 1.1f; // Gentle, smooth lerp speed for automatic running look-ahead
    static constexpr float RETURN_PEEK_SPEED = 1.9f; // Smooth decay speed back to center when stopping

    static constexpr float DEEP_PEEK_DISTANCE_PX = Game::TILE_SIZE * 7.0f; // Manual Q hold deep peek distance (tiles)
    static constexpr float AUTO_LOOKAHEAD_DISTANCE_PX = Game::TILE_SIZE * 3.0f; // Subtle auto look-ahead lead distance (tiles)

    static constexpr float RAMP_START_SEC = 0.15f; // Ignore micro-taps under N s
    static constexpr float RAMP_FULL_SEC = 0.50f;  // Continuous ramp-in window up to N s

    static constexpr float EPSILON_DIR_LEN = 0.001f;
    static constexpr float MIN_OFFSET_SNAP_PX = 0.2f;

    void sync_render_position(float player_alpha_x, float player_alpha_y, float dt = 1.0f / 60.0f) {
        follow(player_alpha_x, player_alpha_y);
        (void)dt;
        sync_core_camera();
    }

    bool is_player_movement_locked() const {
        return false;
    }

    bool is_scouting_or_decaying() const {
        return m_pan_offset_x != 0.0f || m_pan_offset_y != 0.0f;
    }

    void update(float dt, float facing_dx = 0.0f, float facing_dy = 1.0f, bool is_moving = false) {
        bool is_peek_held = Action::is_pan_mode_active();

        if (is_moving && !is_peek_held) {
            m_move_timer += dt;
        } else {
            m_move_timer = 0.0f;
        }

        // Calculate continuous auto-lead weight (0.0 to 1.0) using cubic ease-in
        float raw_t = (m_move_timer - RAMP_START_SEC) / (RAMP_FULL_SEC - RAMP_START_SEC);
        float clamped_t = std::clamp(raw_t, 0.0f, 1.0f);
        float auto_weight = clamped_t * clamped_t * (3.0f - 2.0f * clamped_t);

        float target_offset_x = 0.0f;
        float target_offset_y = 0.0f;

        float len = std::sqrt(facing_dx * facing_dx + facing_dy * facing_dy);
        if (len > EPSILON_DIR_LEN) {
            float norm_dx = facing_dx / len;
            float norm_dy = facing_dy / len;

            if (is_peek_held) {
                // Priority 1: Manual Q Hold Deep Peek (5.0 tiles)
                target_offset_x = norm_dx * DEEP_PEEK_DISTANCE_PX;
                target_offset_y = norm_dy * DEEP_PEEK_DISTANCE_PX;
            } else {
                // Priority 2: Automatic Look-Ahead weighted by continuous movement ramp-in (up to 2.0 tiles)
                target_offset_x = norm_dx * (AUTO_LOOKAHEAD_DISTANCE_PX * auto_weight);
                target_offset_y = norm_dy * (AUTO_LOOKAHEAD_DISTANCE_PX * auto_weight);
            }
        }

        // Dynamic lerp speed based on context
        float active_speed = is_peek_held ? DEEP_PEEK_SPEED : (is_moving ? AUTO_PEEK_SPEED : RETURN_PEEK_SPEED);
        float t = 1.0f - std::exp(-active_speed * dt);

        m_pan_offset_x += (target_offset_x - m_pan_offset_x) * t;
        m_pan_offset_y += (target_offset_y - m_pan_offset_y) * t;

        if (!is_peek_held && !is_moving) {
            if (std::abs(m_pan_offset_x) < MIN_OFFSET_SNAP_PX) m_pan_offset_x = 0.0f;
            if (std::abs(m_pan_offset_y) < MIN_OFFSET_SNAP_PX) m_pan_offset_y = 0.0f;
        }

        sync_core_camera();
    }

private:
    float m_pan_offset_x = 0.0f;
    float m_pan_offset_y = 0.0f;
    float m_move_timer = 0.0f;

    void sync_core_camera() {
        if (has_target) {
            float half_vw = static_cast<float>(Game::WIDTH) / 2.0f;
            float half_vh = static_cast<float>(Game::HEIGHT) / 2.0f;

            float center_x = target_x + m_pan_offset_x;
            float center_y = target_y + m_pan_offset_y;

            x = center_x - half_vw;
            y = center_y - half_vh;
        }

        if (has_limits) {
            float max_x = std::max(limit_left, limit_right - static_cast<float>(Game::WIDTH));
            float max_y = std::max(limit_top, limit_bottom - static_cast<float>(Game::HEIGHT));

            x = std::clamp(x, limit_left, max_x);
            y = std::clamp(y, limit_top, max_y);
        }
    }
};

} // namespace alx
