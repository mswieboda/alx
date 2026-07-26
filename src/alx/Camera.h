#pragma once
#include <algorithm>
#include <cmath>
#include "core/Camera.h"
#include "alx/Action.h"
#include "Game.h"

namespace alx {

class Camera {
public:
    core::Camera core_camera;

    // Pan speed constants
    static constexpr float PAN_SPEED = 2.0f;
    static constexpr float RETURN_SPEED = 6.0f;
    static constexpr float ZOOM_DURATION = 0.15f;

    void follow(float player_center_x, float player_center_y) {
        core_camera.follow(player_center_x, player_center_y);
    }

    void sync_render_position(float player_alpha_x, float player_alpha_y) {
        core_camera.follow(player_alpha_x, player_alpha_y);
        sync_core_camera();
    }

    void set_limits(float left, float top, float right, float bottom) {
        core_camera.set_limits(left, top, right, bottom);
    }

    void set_deadzone(float width, float height) {
        core_camera.set_deadzone(width, height);
    }

    bool is_player_movement_locked() const {
        return m_is_panning_active || m_pan_offset_x != 0.0f || m_pan_offset_y != 0.0f || m_zoom_progress > 0.0f;
    }

    void update(float dt) {
        bool is_panning_held = Action::is_pressed(Action::PanMode);

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

    int to_screen_x(float world_x) const { return core_camera.to_screen_x(world_x); }
    int to_screen_y(float world_y) const { return core_camera.to_screen_y(world_y); }
    float to_world_x(float screen_x) const { return core_camera.to_world_x(screen_x); }
    float to_world_y(float screen_y) const { return core_camera.to_world_y(screen_y); }

    float get_x() const { return core_camera.get_x(); }
    float get_y() const { return core_camera.get_y(); }
    float get_zoom() const { return core_camera.zoom; }

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
            m_pre_pan_x = core_camera.x;
            m_pre_pan_y = core_camera.y;
            m_pan_anchor_x = core_camera.x + half_vw_base;
            m_pan_anchor_y = core_camera.y + half_vh_base;
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
        core_camera.zoom = quantized_tile_size / 16.0f;
    }

    void update_anchor_blend(float dt) {
        if (m_has_ever_wasd_panned && !m_has_wasd_input) {
            m_anchor_blend += (1.0f - m_anchor_blend) * (1.0f - std::exp(-RETURN_SPEED * dt));
        }
    }

    void sync_core_camera() {
        bool is_panning_or_decaying = m_is_panning_active || m_pan_offset_x != 0.0f || m_pan_offset_y != 0.0f || m_zoom_progress > 0.0f;

        if (core_camera.has_target) {
            float half_vw = (static_cast<float>(Game::WIDTH) / 2.0f) / core_camera.zoom;
            float half_vh = (static_cast<float>(Game::HEIGHT) / 2.0f) / core_camera.zoom;

            if (is_panning_or_decaying) {
                if (!m_has_ever_wasd_panned) {
                    float center_x = m_pan_anchor_x;
                    float center_y = m_pan_anchor_y;
                    core_camera.x = center_x - half_vw;
                    core_camera.y = center_y - half_vh;
                } else {
                    float current_anchor_x = (1.0f - m_anchor_blend) * m_pan_anchor_x + m_anchor_blend * core_camera.target_x;
                    float current_anchor_y = (1.0f - m_anchor_blend) * m_pan_anchor_y + m_anchor_blend * core_camera.target_y;

                    float center_x = current_anchor_x + m_pan_offset_x;
                    float center_y = current_anchor_y + m_pan_offset_y;

                    float max_pan_x = Game::TILE_SIZE * 12.0f;
                    float max_pan_y = Game::TILE_SIZE * 9.0f;

                    center_x = std::clamp(center_x, core_camera.target_x - max_pan_x, core_camera.target_x + max_pan_x);
                    center_y = std::clamp(center_y, core_camera.target_y - max_pan_y, core_camera.target_y + max_pan_y);

                    core_camera.x = center_x - half_vw;
                    core_camera.y = center_y - half_vh;
                }
            } else {
                core_camera.update();
                return;
            }
        }

        if (core_camera.has_limits) {
            float max_x = std::max(core_camera.limit_left, core_camera.limit_right - static_cast<float>(Game::WIDTH));
            float max_y = std::max(core_camera.limit_top, core_camera.limit_bottom - static_cast<float>(Game::HEIGHT));

            core_camera.x = std::clamp(core_camera.x, core_camera.limit_left, max_x);
            core_camera.y = std::clamp(core_camera.y, core_camera.limit_top, max_y);
        }

        core_camera.x = std::round(core_camera.x);
        core_camera.y = std::round(core_camera.y);
    }
};

} // namespace alx
