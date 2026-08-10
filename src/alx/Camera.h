#pragma once
#include <algorithm>
#include <cmath>
#include "core/Camera.h"
#include "alx/Action.h"
#include "Game.h"

namespace alx {

struct Camera : public core::Camera {
    // Peek-ahead constants
    static constexpr float PEEK_SPEED = 8.0f; // Smooth lerp speed for peek offset transitions
    static constexpr float PEEK_DISTANCE_PX = Game::TILE_SIZE * 5.5f; // Peek 5.5 tiles in player facing direction

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

    void update(float dt, float facing_dx = 0.0f, float facing_dy = 1.0f) {
        bool is_peek_held = Action::is_pan_mode_active();

        float target_offset_x = 0.0f;
        float target_offset_y = 0.0f;

        if (is_peek_held) {
            float len = std::sqrt(facing_dx * facing_dx + facing_dy * facing_dy);
            if (len > 0.001f) {
                target_offset_x = (facing_dx / len) * PEEK_DISTANCE_PX;
                target_offset_y = (facing_dy / len) * PEEK_DISTANCE_PX;
            }
        }

        float t = 1.0f - std::exp(-PEEK_SPEED * dt);
        m_pan_offset_x += (target_offset_x - m_pan_offset_x) * t;
        m_pan_offset_y += (target_offset_y - m_pan_offset_y) * t;

        if (!is_peek_held) {
            if (std::abs(m_pan_offset_x) < 0.2f) m_pan_offset_x = 0.0f;
            if (std::abs(m_pan_offset_y) < 0.2f) m_pan_offset_y = 0.0f;
        }

        sync_core_camera();
    }

private:
    float m_pan_offset_x = 0.0f;
    float m_pan_offset_y = 0.0f;

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
