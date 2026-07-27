#pragma once
#include "alx/Action.h"
#include "alx/Camera.h"
#include "alx/Tiles.h"
#include "alx/Network.h"
#include "alx/Layer.h"
#include "alx/DrawFX.h"
#include "core/Collision.h"

namespace alx {

struct FacingVector {
    float dx = 0.0f;
    float dy = 1.0f;
};

struct PlayerInputBuffer {
    static constexpr float FACING_DIAGONAL_LATCH_TIME = 0.050f; // 50ms (~3 frames at 60 FPS)
    static constexpr float FACING_START_DELAY_TIME    = 0.033f; // 33ms (~2 frames at 60 FPS)

    float diagonal_latch_timer = 0.0f;
    float start_delay_timer = 0.0f;
    float latched_facing_dx = 0.0f;
    float latched_facing_dy = 1.0f;
    float current_facing_dx = 0.0f;
    float current_facing_dy = 1.0f;
    bool was_moving = false;

    // Updates internal facing state using input latching (Technique 1) & release buffering (Technique 2) and returns facing vector
    FacingVector update_facing(float dt, float raw_dx, float raw_dy) {
        bool is_diagonal = (raw_dx != 0.0f && raw_dy != 0.0f);
        bool is_moving = (raw_dx != 0.0f || raw_dy != 0.0f);

        if (is_diagonal) {
            // Immediately lock diagonal facing vector and refresh latch timer
            constexpr float inv_sqrt2 = 0.70710678118f;
            current_facing_dx = raw_dx * inv_sqrt2;
            current_facing_dy = raw_dy * inv_sqrt2;
            latched_facing_dx = current_facing_dx;
            latched_facing_dy = current_facing_dy;
            diagonal_latch_timer = FACING_DIAGONAL_LATCH_TIME;
            start_delay_timer = 0.0f;
        } else if (is_moving) {
            // Single-axis input
            if (diagonal_latch_timer > 0.0f) {
                // Technique 2: Diagonal Release Buffer (Stop Latch)
                diagonal_latch_timer -= dt;
                current_facing_dx = latched_facing_dx;
                current_facing_dy = latched_facing_dy;
                start_delay_timer = 0.0f;
            } else if (!was_moving) {
                // Technique 1: Facing Direction Hysteresis (2-Frame Input Latch)
                // Defer updating facing_dx/dy by 2 frames when starting movement from idle
                start_delay_timer = FACING_START_DELAY_TIME;
            } else if (start_delay_timer > 0.0f) {
                start_delay_timer -= dt;
                if (start_delay_timer <= 0.0f) {
                    current_facing_dx = raw_dx;
                    current_facing_dy = raw_dy;
                }
            } else {
                current_facing_dx = raw_dx;
                current_facing_dy = raw_dy;
            }
        } else {
            // Stopped moving (raw_dx == 0 && raw_dy == 0)
            if (diagonal_latch_timer > 0.0f) {
                diagonal_latch_timer -= dt;
                current_facing_dx = latched_facing_dx;
                current_facing_dy = latched_facing_dy;
            }
            start_delay_timer = 0.0f;
        }

        was_moving = is_moving;
        return FacingVector{ current_facing_dx, current_facing_dy };
    }
};

struct Player : public Entity {
    // Movement speed in pixels per 60Hz physics tick.
    // NOTE FOR TUNING: Sticking to simple rational fractions (0.25f, 0.50f, 0.75f, 1.00f, 1.25f, 1.50f)
    // maintains a steady, harmonic multi-frame sub-pixel cadence without irregular rasterization stutter:
    //   - 1.00f = 60px/s (1px/tick: 100% 60Hz smooth, 0 pause frames)
    //   - 0.75f = 45px/s (3/4px/tick: steady 4-frame +1, +1, +0, +1 cadence)
    //   - 0.50f = 30px/s (1/2px/tick: steady 30Hz alternating +1, +0, +1, +0 cadence)
    //   - 0.25f = 15px/s (1/4px/tick: steady 15Hz 4-tick +1, +0, +0, +0 pulse)
    float pixels_per_tick = 1.0f;
    float speed = pixels_per_tick * static_cast<float>(Game::TARGET_FPS);
    int wand_radius = 96;
    bool is_panning = false;

    // Input buffer for facing hysteresis & diagonal release buffer
    PlayerInputBuffer input_buffer;

    // Facing vector (default facing down)
    float facing_dx = 0.0f;
    float facing_dy = 1.0f;

    // Attack timing and radius constants
    static constexpr float ATTACK_ACTIVE_DURATION = 0.15f;
    static constexpr float ATTACK_COOLDOWN_DURATION = 0.25f;
    static constexpr float ATTACK_HIT_RADIUS = 10.0f;
    static constexpr float ATTACK_HIT_OFFSET = 10.0f;

    // Relative collision ratio constants
    static constexpr float GROUND_RADIUS_RATIO = 0.50f;   // 50% of transform.width (6.0px)
    static constexpr float GROUND_OFFSET_Y_RATIO = 1.00f; // Bottom aligned (transform.y + transform.height - r)
    static constexpr float HURT_RADIUS_RATIO = 0.50f;     // 50% of transform.width (6.0px)
    static constexpr float HURT_OFFSET_Y_RATIO = 0.50f;   // Torso center (transform.y + transform.height * 0.5)

    // visuals
    static constexpr float SHADOW_RX_RATIO = 0.8f;
    static constexpr float SHADOW_RY_RATIO_OF_RX = 0.45f;


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
            Transform{ x, y, 12, 24, Layer::WorldObj }, // Transform (x, y, w, h, z_index)
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

    Collision::Circle ground_circle(float px, float py) const {
        float r = transform.width * GROUND_RADIUS_RATIO;
        float cy = py + (transform.height * GROUND_OFFSET_Y_RATIO) - r;
        return Collision::Circle{ px + (transform.width / 2.0f), cy, r };
    }

    Collision::Circle ground_circle() const {
        return ground_circle(transform.x, transform.y);
    }

    Collision::Circle hurt_circle(float px, float py) const {
        float r = transform.width * HURT_RADIUS_RATIO;
        float cy = py + (transform.height * HURT_OFFSET_Y_RATIO);
        return Collision::Circle{ px + (transform.width / 2.0f), cy, r };
    }

    Collision::Circle hurt_circle() const {
        return hurt_circle(transform.x, transform.y);
    }

    void sync_prev_transforms() {
        transform_prev = transform;
    }

    bool is_attacking() const {
        return attack_active_timer > 0.0f;
    }

    Collision::Circle attack_hit_circle() const {
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

    void draw(std::vector<uint32_t>& screen_buffer, float alpha) {
        if (!active) return;

        float world_draw_x = Draw::interpolate(transform_prev.x, transform.x, alpha);
        float world_draw_y = Draw::interpolate(transform_prev.y, transform.y, alpha);
        float world_draw_w = transform.width;
        float world_draw_h = transform.height;

        float world_bottom_y = world_draw_y + world_draw_h;

        // Player shadow underneath player at bottom Y edge (foreshortened oval)
        DrawFX::shadow(
            world_draw_x,
            world_draw_y,
            world_draw_w,
            world_draw_h,
            transform.z_index,
            SHADOW_RX_RATIO,
            SHADOW_RY_RATIO_OF_RX
        );

        // Player body
        if (auto* rect = std::get_if<RectangleRender>(&visual)) {
            Draw::rect(
                world_draw_x,
                world_draw_y,
                world_draw_w,
                world_draw_h,
                rect->color,
                rect->fill,
                rect->thickness,
                transform.z_index,
                static_cast<int>(world_bottom_y) // sort Y override
            );
        }

        // Ground feet collision circle outline (cyan debug)
        if (Game::DRAW_DEBUG_COLLISION_AREAS) {
            Collision::Circle ground = ground_circle(world_draw_x, world_draw_y);
            Draw::oval(
                ground.cx,
                ground.cy,
                ground.radius,
                ground.radius,
                0xFF00FFFF, // Bright Cyan debug outline
                false,      // fill = false (outline only)
                1,          // thickness = 1
                transform.z_index + 1,
                static_cast<int>(world_bottom_y)
            );
        }

        // Attack hit box/circle
        if (is_attacking()) {
            Collision::Circle hit_c = attack_hit_circle();
            Draw::rect(
                hit_c.cx - hit_c.radius,
                hit_c.cy - hit_c.radius,
                hit_c.radius * 2.0f,
                hit_c.radius * 2.0f,
                0x8000FFFF, // 50% transparent Cyan debug attack box
                true,
                1,
                transform.z_index,
                static_cast<int>(world_bottom_y) // sort Y override
            );
        }

        // Indicator where tile build/remove happens
        float size = std::round(world_draw_h / 4.0f);
        float target_center_x = world_draw_x + (world_draw_w / 2.0f);
        float target_center_y = world_draw_y + std::round(world_draw_h / 1.25f);
        float box_x = target_center_x - (size / 2.0f);
        float box_y = target_center_y - (size / 2.0f);

        Draw::rect(
            box_x,
            box_y,
            size,
            size,
            0xFF990099,
            true,
            1,
            transform.z_index,
            static_cast<int>(world_bottom_y)
        );
    }

    int cursed_alloy() const { return m_cursed_alloy; }
    void add_cursed_alloy(int amount) { m_cursed_alloy += amount; }
    FixtureType selected_fixture_type() const { return m_selected_fixture_type; }

    static int fixture_cost(FixtureType type) {
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

        FacingVector facing = input_buffer.update_facing(dt, dx, dy);
        facing_dx = facing.dx;
        facing_dy = facing.dy;

        // DIAGONAL SPEED SCALE OPTIONS:
        // 1.00f = Classic 16-bit SNES/Zelda grid-aligned (+41% speed boost, 100% 60Hz smooth)
        // 0.75f = Rational 3/4px sub-pixel step (+6% speed boost, silky 4-frame rational rhythm)
        // 0.70710678f = Euclidean normalized (+0% speed boost, irrational jitter pattern)
        static constexpr float DIAGONAL_SPEED_SCALE = 0.75f;

        if (dx != 0.0f && dy != 0.0f) {
            dx *= DIAGONAL_SPEED_SCALE;
            dy *= DIAGONAL_SPEED_SCALE;
        }

        float target_x = transform.x + dx * speed * dt;
        if (!is_solid_ground(target_x, transform.y, tiles, network)) {
            transform.x = target_x;
        }

        float target_y = transform.y + dy * speed * dt;
        if (!is_solid_ground(transform.x, target_y, tiles, network)) {
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
        int tile_size = tiles.tile_size();
        GridPos target_pos{ static_cast<int16_t>(static_cast<int>(center_x) / tile_size), static_cast<int16_t>(static_cast<int>(center_y) / tile_size) };

        if (Action::is_build_tile()) {
            int cost = fixture_cost(m_selected_fixture_type);
            if (m_cursed_alloy >= cost && network.can_place_fixture(target_pos, m_selected_fixture_type, tiles)) {
                m_cursed_alloy -= cost;
                network.place_fixture(target_pos, m_selected_fixture_type);
            }
        }

        if (Action::is_just_pressed(Action::Cancel)) {
            if (network.in_bounds(target_pos)) {
                const Fixture& fix = network.fixture(target_pos);
                if (fix.type != FixtureType::None && fix.type != FixtureType::Seep) {
                    int refund = fixture_cost(fix.type);
                    m_cursed_alloy += refund;
                    network.remove_fixture(target_pos);
                }
            }
        }
    }

    bool is_solid_ground(float test_x, float test_y, const Tiles& tiles, const Network& network) const {
        Collision::Circle ground = ground_circle(test_x, test_y);
        float tile_size = static_cast<float>(tiles.tile_size());

        int min_tx = static_cast<int>(ground.cx - ground.radius) / tiles.tile_size();
        int max_tx = static_cast<int>(ground.cx + ground.radius) / tiles.tile_size();
        int min_ty = static_cast<int>(ground.cy - ground.radius) / tiles.tile_size();
        int max_ty = static_cast<int>(ground.cy + ground.radius) / tiles.tile_size();

        for (int ty = min_ty; ty <= max_ty; ++ty) {
            for (int tx = min_tx; tx <= max_tx; ++tx) {
                if (tiles.is_wall(tx, ty)) {
                    if (Collision::circle_vs_aabb(ground, tx * tile_size, ty * tile_size, tile_size, tile_size)) {
                        return true;
                    }
                }
                if (network.in_bounds(tx, ty)) {
                    if (network.is_solid(tx, ty)) {
                        Collision::AABB fixture_aabb = fixture_ground_aabb(tx, ty, tile_size);
                        if (Collision::circle_vs_aabb(ground, fixture_aabb)) {
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
