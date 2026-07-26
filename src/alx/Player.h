#pragma once
#include "alx/Action.h"
#include "alx/Camera.h"
#include "alx/Tiles.h"
#include "alx/Network.h"

namespace alx {

struct Player : public Entity {
    // Integer pixels moved per physics tick
    int pixels_per_tick = 1;
    float speed = static_cast<float>(pixels_per_tick * Game::TARGET_FPS);
    int wand_radius = 96;
    bool is_panning = false;

    // Facing vector (default facing down)
    float facing_dx = 0.0f;
    float facing_dy = 1.0f;

    // Attack timing constants
    static constexpr float ATTACK_ACTIVE_DURATION = 0.15f;
    static constexpr float ATTACK_COOLDOWN_DURATION = 0.25f;

    float attack_active_timer = 0.0f;
    float attack_cooldown_timer = 0.0f;

    struct AttackHitbox {
        float x = 0.0f;
        float y = 0.0f;
        float width = 0.0f;
        float height = 0.0f;
    };

    Player(float x = 128.0f, float y = 128.0f)
        : Entity(
            Transform{ x, y, 12, 24, 10 }, // Transform (x, y, w, h, z_index)
            RectangleRender{ 0xFFFF00FF, true, 1 },         // Visual (Magenta box representation)
            true,                                           // Active
            "player"                                        // Tag for easy lookups
          )
    {
    }

    float center_x(float alpha = 1.0f) const {
        float draw_x = Draw::interpolate(transform_prev.x, transform.x, alpha);
        return draw_x + (transform.width / 2.0f);
    }

    float center_y(float alpha = 1.0f) const {
        float draw_y = Draw::interpolate(transform_prev.y, transform.y, alpha);
        return draw_y + (transform.height / 2.0f);
    }

    void sync_prev_transforms() {
        transform_prev = transform;
    }

    bool is_attacking() const {
        return attack_active_timer > 0.0f;
    }

    AttackHitbox get_attack_hitbox() const {
        float cx = transform.x + (transform.width / 2.0f);
        float cy = transform.y + (transform.height / 2.0f);

        AttackHitbox hb;
        if (std::abs(facing_dy) >= std::abs(facing_dx)) {
            hb.width = 16.0f;
            hb.height = 8.0f;
            float offset_y = (facing_dy >= 0.0f) ? (transform.height / 2.0f + 4.0f) : -(transform.height / 2.0f + 4.0f);
            hb.x = cx - 8.0f;
            hb.y = cy + offset_y - 4.0f;
        } else {
            hb.width = 8.0f;
            hb.height = 16.0f;
            float offset_x = (facing_dx >= 0.0f) ? (transform.width / 2.0f + 4.0f) : -(transform.width / 2.0f + 4.0f);
            hb.x = cx + offset_x - 4.0f;
            hb.y = cy - 8.0f;
        }
        return hb;
    }

    void update(float dt, const Tiles& tiles, Network& network, const alx::Camera& camera) {
        sync_prev_transforms();

        if (attack_active_timer > 0.0f) {
            attack_active_timer -= dt;
        }
        if (attack_cooldown_timer > 0.0f) {
            attack_cooldown_timer -= dt;
        }

        update_movement(dt, tiles, network, camera);
        update_actions(dt, tiles, network);
    }

    void draw(std::vector<uint32_t>& screen_buffer, float alpha, const alx::Camera& camera) {
        if (!active) return;

        float world_draw_x = Draw::interpolate(transform_prev.x, transform.x, alpha);
        float world_draw_y = Draw::interpolate(transform_prev.y, transform.y, alpha);

        int draw_x = camera.to_screen_x(world_draw_x);
        int draw_y = camera.to_screen_y(world_draw_y);
        int draw_w = std::max(1, static_cast<int>(std::round(transform.width * camera.get_zoom())));
        int draw_h = std::max(1, static_cast<int>(std::round(transform.height * camera.get_zoom())));

        if (auto* rect = std::get_if<RectangleRender>(&visual)) {
            Draw::rect(
                draw_x,
                draw_y,
                draw_w,
                draw_h,
                rect->color,
                rect->fill,
                rect->thickness,
                transform.z_index
            );
        }

        if (is_attacking()) {
            AttackHitbox hb = get_attack_hitbox();
            int hx = camera.to_screen_x(hb.x);
            int hy = camera.to_screen_y(hb.y);
            int hw = std::max(1, static_cast<int>(std::round(hb.width * camera.get_zoom())));
            int hh = std::max(1, static_cast<int>(std::round(hb.height * camera.get_zoom())));

            Draw::rect(
                hx, hy, hw, hh,
                0xFF00FFFF, // Cyan attack arc flash
                true,
                1,
                transform.z_index + 1
            );
        }

        float size = (transform.height / 4.0f) * camera.get_zoom();

        float target_center_x = draw_x + (draw_w / 2.0f);
        float target_center_y = draw_y + (draw_h / 1.25f);

        float box_x = target_center_x - (size / 2.0f);
        float box_y = target_center_y - (size / 2.0f);

        Draw::rect(
            static_cast<int>(std::round(box_x)),
            static_cast<int>(std::round(box_y)),
            std::max(1, static_cast<int>(std::round(size))),
            std::max(1, static_cast<int>(std::round(size))),
            0xFF990099,
            true,
            1,
            transform.z_index
        );
    }

    int get_cursed_alloy() const { return m_cursed_alloy; }
    void add_cursed_alloy(int amount) { m_cursed_alloy += amount; }
    FixtureType get_selected_fixture_type() const { return m_selected_fixture_type; }

    static int get_fixture_cost(FixtureType type) {
        switch (type) {
            case FixtureType::Pipe: return 1;
            case FixtureType::Refiner: return 5;
            case FixtureType::Spire: return 10;
            default: return 0;
        }
    }

private:
    int m_cursed_alloy = 5;
    FixtureType m_selected_fixture_type = FixtureType::Pipe;

    void update_movement(float dt, const Tiles& tiles, const Network& network, const alx::Camera& camera) {
        is_panning = Action::is_pressed(Action::PanMode);

        if (camera.is_player_movement_locked()) {
            return;
        }

        if (Action::is_pressed(Action::Build)) {
            return;
        }

        float dx = 0.0f;
        float dy = 0.0f;

        if (Action::is_pressed(Action::MoveUp))    dy -= 1.0f;
        if (Action::is_pressed(Action::MoveDown))  dy += 1.0f;
        if (Action::is_pressed(Action::MoveLeft))  dx -= 1.0f;
        if (Action::is_pressed(Action::MoveRight)) dx += 1.0f;

        if (dx != 0.0f || dy != 0.0f) {
            facing_dx = dx;
            facing_dy = dy;
        }

        if (dx != 0.0f && dy != 0.0f) {
            constexpr float inv_sqrt2 = 0.70710678118f;
            dx *= inv_sqrt2;
            dy *= inv_sqrt2;
        }

        float target_x = transform.x + dx * speed * dt;
        if (!is_solid_box(target_x, transform.y, transform.width, transform.height, tiles, network)) {
            transform.x = target_x;
        }

        float target_y = transform.y + dy * speed * dt;
        if (!is_solid_box(transform.x, target_y, transform.width, transform.height, tiles, network)) {
            transform.y = target_y;
        }
    }

    void update_actions(float dt, const Tiles& tiles, Network& network) {
        if (Action::is_attack() && attack_cooldown_timer <= 0.0f) {
            attack_active_timer = ATTACK_ACTIVE_DURATION;
            attack_cooldown_timer = ATTACK_COOLDOWN_DURATION;
        }

        if (Action::is_cycle_right()) {
            if (m_selected_fixture_type == FixtureType::Pipe) {
                m_selected_fixture_type = FixtureType::Refiner;
            } else if (m_selected_fixture_type == FixtureType::Refiner) {
                m_selected_fixture_type = FixtureType::Spire;
            } else {
                m_selected_fixture_type = FixtureType::Pipe;
            }
        } else if (Action::is_cycle_left()) {
            if (m_selected_fixture_type == FixtureType::Pipe) {
                m_selected_fixture_type = FixtureType::Spire;
            } else if (m_selected_fixture_type == FixtureType::Spire) {
                m_selected_fixture_type = FixtureType::Refiner;
            } else {
                m_selected_fixture_type = FixtureType::Pipe;
            }
        }

        if (Action::is_just_pressed(Action::DebugResource)) {
            m_cursed_alloy += 10;
        }

        float center_x = transform.x + (transform.width / 2.0f);
        float center_y = transform.y + (transform.height / 1.25f);
        int tile_size = tiles.get_tile_size();
        GridPos target_pos{ static_cast<int16_t>(static_cast<int>(center_x) / tile_size), static_cast<int16_t>(static_cast<int>(center_y) / tile_size) };

        if (Action::is_build_tile()) {
            int cost = get_fixture_cost(m_selected_fixture_type);
            if (m_cursed_alloy >= cost && network.can_place_fixture(target_pos, m_selected_fixture_type, tiles)) {
                m_cursed_alloy -= cost;
                network.place_fixture(target_pos, m_selected_fixture_type);
            }
        }

        if (Action::is_just_pressed(Action::Cancel)) {
            if (network.in_bounds(target_pos)) {
                const Fixture& fix = network.get_fixture(target_pos);
                if (fix.type != FixtureType::None && fix.type != FixtureType::Seep) {
                    int refund = get_fixture_cost(fix.type);
                    m_cursed_alloy += refund;
                    network.remove_fixture(target_pos);
                }
            }
        }
    }

    // TODO: Refactor player movement collision to use a dedicated feet collision box/circle radius and radius-based fixture collisions in the future
    bool is_solid_box(float x, float y, float width, float height, const Tiles& tiles, const Network& network) const {
        int tile_size = tiles.get_tile_size();

        int left   = static_cast<int>(x) / tile_size;
        int right  = static_cast<int>(x + width - 0.01f) / tile_size;
        int top    = static_cast<int>(y) / tile_size;
        int bottom = static_cast<int>(y + height - 0.01f) / tile_size;

        return tiles.is_wall(left, top)     ||
               tiles.is_wall(right, top)    ||
               tiles.is_wall(left, bottom)  ||
               tiles.is_wall(right, bottom);
    }
};

} // namespace alx
