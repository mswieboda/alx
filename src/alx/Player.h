#pragma once
#include "alx/Action.h"
#include "alx/Camera.h"
#include "alx/Tiles.h"
#include "alx/Network.h"
#include "core/Collision.h"

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

    // Attack timing and radius constants
    static constexpr float ATTACK_ACTIVE_DURATION = 0.15f;
    static constexpr float ATTACK_COOLDOWN_DURATION = 0.25f;
    static constexpr float ATTACK_HIT_RADIUS = 10.0f;
    static constexpr float ATTACK_HIT_OFFSET = 10.0f;

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

    Collision::Circle get_feet_collision_circle(float px, float py) const {
        return Collision::Circle{ px + (transform.width / 2.0f), py + transform.height, 6.0f };
    }

    Collision::Circle get_feet_collision_circle() const {
        return get_feet_collision_circle(transform.x, transform.y);
    }

    Collision::Circle get_hurt_circle(float px, float py) const {
        return Collision::Circle{ px + (transform.width / 2.0f), py + (transform.height / 2.0f), 6.0f };
    }

    Collision::Circle get_hurt_circle() const {
        return get_hurt_circle(transform.x, transform.y);
    }

    void sync_prev_transforms() {
        transform_prev = transform;
    }

    bool is_attacking() const {
        return attack_active_timer > 0.0f;
    }

    Collision::Circle get_attack_hit_circle() const {
        float cx = center_x(1.0f) + facing_dx * ATTACK_HIT_OFFSET;
        float cy = center_y(1.0f) + facing_dy * ATTACK_HIT_OFFSET;
        return Collision::Circle{ cx, cy, ATTACK_HIT_RADIUS };
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
            Collision::Circle hit_c = get_attack_hit_circle();
            int hx = camera.to_screen_x(hit_c.cx - hit_c.radius);
            int hy = camera.to_screen_y(hit_c.cy - hit_c.radius);
            int hw = std::max(1, static_cast<int>(std::round(hit_c.radius * 2.0f * camera.get_zoom())));
            int hh = std::max(1, static_cast<int>(std::round(hit_c.radius * 2.0f * camera.get_zoom())));

            Draw::rect(
                hx, hy, hw, hh,
                0x8000FFFF, // 50% transparent Cyan debug attack box
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
            if (dx != 0.0f && dy != 0.0f) {
                constexpr float inv_sqrt2 = 0.70710678118f;
                facing_dx = dx * inv_sqrt2;
                facing_dy = dy * inv_sqrt2;
            } else {
                facing_dx = dx;
                facing_dy = dy;
            }
        }

        if (dx != 0.0f && dy != 0.0f) {
            constexpr float inv_sqrt2 = 0.70710678118f;
            dx *= inv_sqrt2;
            dy *= inv_sqrt2;
        }


        float target_x = transform.x + dx * speed * dt;
        if (!is_solid_feet(target_x, transform.y, tiles, network)) {
            transform.x = target_x;
        }

        float target_y = transform.y + dy * speed * dt;
        if (!is_solid_feet(transform.x, target_y, tiles, network)) {
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

    bool is_solid_feet(float test_x, float test_y, const Tiles& tiles, const Network& network) const {
        Collision::Circle feet = get_feet_collision_circle(test_x, test_y);
        float tile_size = static_cast<float>(tiles.get_tile_size());

        int min_tx = static_cast<int>(feet.cx - feet.radius) / tiles.get_tile_size();
        int max_tx = static_cast<int>(feet.cx + feet.radius) / tiles.get_tile_size();
        int min_ty = static_cast<int>(feet.cy - feet.radius) / tiles.get_tile_size();
        int max_ty = static_cast<int>(feet.cy + feet.radius) / tiles.get_tile_size();

        for (int ty = min_ty; ty <= max_ty; ++ty) {
            for (int tx = min_tx; tx <= max_tx; ++tx) {
                if (tiles.is_wall(tx, ty)) {
                    if (Collision::circle_vs_aabb(feet, tx * tile_size, ty * tile_size, tile_size, tile_size)) {
                        return true;
                    }
                }
                if (network.in_bounds(tx, ty)) {
                    FixtureType ft = network.get_fixture(tx, ty).type;
                    if (ft == FixtureType::Refiner || ft == FixtureType::Spire || ft == FixtureType::Seep) {
                        Collision::Circle fixture_c{ tx * tile_size + tile_size / 2.0f, ty * tile_size + tile_size / 2.0f, 7.5f };
                        if (Collision::circle_vs_circle(feet, fixture_c)) {
                            return true;
                        }
                    }
                }
            }
        }
        return false;
    }
};

} // namespace alx

