#pragma once
#include <algorithm>
#include <cmath>
#include "core/Camera.h"
#include "alx/Action.h"
#include "Game.h"

namespace alx {

struct Camera : public core::Camera {
    // Pan speed constants
    static constexpr float PAN_SPEED = 2.0f;
    static constexpr float RETURN_SPEED = 6.0f;
    static constexpr float ZOOM_DURATION = 0.15f;

    void sync_render_position(float player_alpha_x, float player_alpha_y, float dt = 1.0f / 60.0f) {
        follow(player_alpha_x, player_alpha_y);
        sync_core_camera(dt);
    }

    bool is_player_movement_locked() const {
        return m_is_panning_active;
    }

    bool is_scouting_or_decaying() const {
        return m_is_panning_active || m_pan_offset_x != 0.0f || m_pan_offset_y != 0.0f || m_zoom_progress > 0.0f;
    }

    void update(float dt) {
        bool is_panning_held = Action::is_pan_mode_active();

        if (is_panning_held) {
            start_panning();
        } else {
            stop_panning();
        }

        process_pan_input(is_panning_held);
        update_pan_offset(dt, is_panning_held);
        update_zoom_transition(dt, is_panning_held);
        update_anchor_blend(dt);
        sync_core_camera();
    }

private:
    float m_target_pan_x = 0.0f;
    float m_target_pan_y = 0.0f;
    float m_pan_offset_x = 0.0f;
    float m_pan_offset_y = 0.0f;
    float m_pan_anchor_x = 0.0f;
    float m_pan_anchor_y = 0.0f;
    float m_pre_pan_x = 0.0f;
    float m_pre_pan_y = 0.0f;
    float m_anchor_blend = 0.0f;
    float m_zoom_progress = 0.0f;

    bool m_is_panning_active = false;
    bool m_has_ever_wasd_panned = false;
    bool m_has_wasd_input = false;

    void start_panning() {
        if (!m_is_panning_active) {
            float half_vw_base = static_cast<float>(Game::WIDTH) / 2.0f;
            float half_vh_base = static_cast<float>(Game::HEIGHT) / 2.0f;
            m_pre_pan_x = x;
            m_pre_pan_y = y;
            m_pan_anchor_x = x + half_vw_base;
            m_pan_anchor_y = y + half_vh_base;
            m_anchor_blend = 0.0f;
            m_has_ever_wasd_panned = false;
            m_is_panning_active = true;
        }
    }

    void stop_panning() {
        m_is_panning_active = false;
    }

    void process_pan_input(bool is_panning_held) {
        m_target_pan_x = 0.0f;
        m_target_pan_y = 0.0f;
        m_has_wasd_input = false;

        if (is_panning_held) {
            float pdx = 0.0f;
            float pdy = 0.0f;
            if (Action::is_pressed(Action::MoveUp))    pdy -= 1.0f;
            if (Action::is_pressed(Action::MoveDown))  pdy += 1.0f;
            if (Action::is_pressed(Action::MoveLeft))  pdx -= 1.0f;
            if (Action::is_pressed(Action::MoveRight)) pdx += 1.0f;

            if (pdx != 0.0f || pdy != 0.0f) {
                m_has_wasd_input = true;
                m_has_ever_wasd_panned = true;
            }

            if (pdx != 0.0f && pdy != 0.0f) {
                constexpr float inv_sqrt2 = 0.70710678118f;
                pdx *= inv_sqrt2;
                pdy *= inv_sqrt2;
            }

            constexpr float MAX_PAN_DIST_X = Game::TILE_SIZE * 12.0f;
            constexpr float MAX_PAN_DIST_Y = MAX_PAN_DIST_X * (static_cast<float>(Game::HEIGHT) / static_cast<float>(Game::WIDTH));
            m_target_pan_x = pdx * MAX_PAN_DIST_X;
            m_target_pan_y = pdy * MAX_PAN_DIST_Y;
        }
    }

    void update_pan_offset(float dt, bool is_panning_held) {
        float active_speed = (is_panning_held && m_has_wasd_input) ? PAN_SPEED : RETURN_SPEED;
        float pan_t = 1.0f - std::exp(-active_speed * dt);
        m_pan_offset_x += (m_target_pan_x - m_pan_offset_x) * pan_t;
        m_pan_offset_y += (m_target_pan_y - m_pan_offset_y) * pan_t;

        if (!is_panning_held) {
            if (std::abs(m_pan_offset_x) < 0.5f) m_pan_offset_x = 0.0f;
            if (std::abs(m_pan_offset_y) < 0.5f) m_pan_offset_y = 0.0f;
        }
    }

    void update_zoom_transition(float dt, bool is_panning_held) {
        if (is_panning_held) {
            m_zoom_progress += dt / ZOOM_DURATION;
        } else {
            m_zoom_progress -= dt / ZOOM_DURATION;
        }
        m_zoom_progress = std::clamp(m_zoom_progress, 0.0f, 1.0f);

        float eased_t = m_zoom_progress * m_zoom_progress * (3.0f - 2.0f * m_zoom_progress);
        float min_zoom = 0.75f;
        float max_zoom = 1.00f;
        float raw_zoom = max_zoom + (min_zoom - max_zoom) * eased_t;

        float raw_tile_size = 16.0f * raw_zoom;
        float quantized_tile_size = std::round(raw_tile_size);
        zoom = quantized_tile_size / 16.0f;
    }

    void update_anchor_blend(float dt) {
        if (m_has_ever_wasd_panned && !m_has_wasd_input) {
            m_anchor_blend += (1.0f - m_anchor_blend) * (1.0f - std::exp(-RETURN_SPEED * dt));
        }
    }

    void sync_core_camera(float dt = 1.0f / 60.0f) {
        update_shake(dt);
        if (has_target) {
            float half_vw = (static_cast<float>(Game::WIDTH) / 2.0f) / zoom;
            float half_vh = (static_cast<float>(Game::HEIGHT) / 2.0f) / zoom;

            if (is_scouting_or_decaying()) {
                if (!m_has_ever_wasd_panned) {
                    float center_x = m_pan_anchor_x;
                    float center_y = m_pan_anchor_y;
                    x = center_x - half_vw;
                    y = center_y - half_vh;
                } else {
                    float current_anchor_x = (1.0f - m_anchor_blend) * m_pan_anchor_x + m_anchor_blend * target_x;
                    float current_anchor_y = (1.0f - m_anchor_blend) * m_pan_anchor_y + m_anchor_blend * target_y;

                    float center_x = current_anchor_x + m_pan_offset_x;
                    float center_y = current_anchor_y + m_pan_offset_y;

                    float max_pan_x = Game::TILE_SIZE * 12.0f;
                    float max_pan_y = Game::TILE_SIZE * 9.0f;

                    center_x = std::clamp(center_x, target_x - max_pan_x, target_x + max_pan_x);
                    center_y = std::clamp(center_y, target_y - max_pan_y, target_y + max_pan_y);

                    x = center_x - half_vw;
                    y = center_y - half_vh;
                }
            } else {
                core::Camera::update(dt);
                return;
            }
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
