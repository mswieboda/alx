#pragma once
#include "alx/Action.h"
#include "alx/Camera.h"
#include "alx/Tiles.h"
#include "alx/Network.h"
#include "alx/Layer.h"
#include "alx/DrawFX.h"
#include "alx/WorldCollision.h"
#include "alx/TrigLUT.h"
#include "core/Collision.h"
#include "core/Entity.h"
#include "Debug.h"
#include "assets/Images.h"


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
    // visuals
    static constexpr int SPRITE_WIDTH = 19;
    static constexpr int SPRITE_HEIGHT = 29;
    static constexpr float SHADOW_RX_RATIO = 0.45f;
    static constexpr float SHADOW_RY_RATIO_OF_RX = 0.45f;

    // Movement speed in pixels per 60Hz physics tick.
    // NOTE FOR TUNING: Sticking to simple rational fractions (0.25f, 0.50f, 0.75f, 1.00f, 1.25f, 1.50f)
    // maintains a steady, harmonic multi-frame sub-pixel cadence without irregular rasterization stutter:
    //   - 1.00f = 60px/s (1px/tick: 100% 60Hz smooth, 0 pause frames)
    //   - 0.75f = 45px/s (3/4px/tick: steady 4-frame +1, +1, +0, +1 cadence)
    //   - 0.50f = 30px/s (1/2px/tick: steady 30Hz alternating +1, +0, +1, +0 cadence)
    //   - 0.25f = 15px/s (1/4px/tick: steady 15Hz 4-tick +1, +0, +0, +0 pulse)
    static constexpr float SPEED_PX_PER_TICK = 1.00f; // 1.00 px/tick at 60 FPS
    static constexpr float SPEED = SPEED_PX_PER_TICK * Game::TARGET_FPS; // 60.0 px/s

    float speed = SPEED;
    int wand_radius = 96;
    bool is_panning = false;

    // Input buffer for facing hysteresis & diagonal release buffer
    PlayerInputBuffer input_buffer;

    // Facing vector (default facing down)
    float facing_dx = 0.0f;
    float facing_dy = 1.0f;

    enum class AttackPhase { Idle, ActiveSweep, Recovery };

    // Relative collision ratio constants
    static constexpr float GROUND_RADIUS_RATIO = 0.25f;   // % of transform.width
    static constexpr float GROUND_OFFSET_Y_RATIO = 1.00f; // Bottom aligned (transform.y + transform.height - r)
    static constexpr float HURT_RADIUS_RATIO = 0.25f;     // % of transform.width
    static constexpr float HURT_OFFSET_Y_RATIO = 0.50f;   // Torso center (transform.y + transform.height * %)


    // Attack timing and radius constants
    static constexpr float ATTACK_SWEEP_DURATION    = 0.15f; // 0.15s active arc sweep
    static constexpr float ATTACK_RECOVERY_DURATION = 0.15f; // 0.10s recovery delay
    static constexpr float ATTACK_REACH_RADIUS      = 12.0f; // Reach distance from player center
    static constexpr float ATTACK_HIT_RADIUS        = 8.0f;  // Radius of hit circle
    static constexpr int ATTACK_ARC_SWEEP_START_DEG = -60;   // in degrees (where swing starts)
    static constexpr int ATTACK_ARC_SWEEP_SWING_DEG = 125;    // in degrees (total swing motion)
    static constexpr float ATTACK_KNOCKBACK_SPEED   = 115.0f;


    AttackPhase attack_phase = AttackPhase::Idle;
    float attack_timer = 0.0f;
    uint32_t current_swing_id = 0;
    float swing_progress_prev = 0.0f;
    float swing_progress_curr = 0.0f;

    Player(float x = 128.0f, float y = 128.0f)
        : Entity(
            Transform{ // Transform
                x,
                y,
                SPRITE_WIDTH, // width
                SPRITE_HEIGHT, // height
                Layer::WorldObj // z-index
            },
            SpriteRender{
                Assets::Images::mystic_19x29, // pixels
                Assets::Images::mystic_19x29_len, // pixels size
                SPRITE_WIDTH, // width
                SPRITE_HEIGHT // height
            },
            true, // active
            "player" // tag
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

    struct PlacementPoint {
        float cx = 0.0f;
        float cy = 0.0f;
    };

    PlacementPoint placement_fixture_center(float px, float py, float tile_size = 16.0f) const {
        Collision::Circle g = ground_circle(px, py);
        float fallback_cx = g.cx + facing_dx * tile_size;
        float fallback_cy = g.cy + facing_dy * tile_size;

        for (float d = tile_size * 0.5f; d <= tile_size * 2.5f; d += tile_size * 0.25f) {
            float test_cx = g.cx + facing_dx * d;
            float test_cy = g.cy + facing_dy * d;
            int tx = static_cast<int>(std::floor(test_cx / tile_size));
            int ty = static_cast<int>(std::floor(test_cy / tile_size));

            Collision::AABB proposed_aabb = fixture_ground_aabb(tx, ty, tile_size, m_selected_fixture_type);
            if (!Collision::circle_vs_aabb(g, proposed_aabb)) {
                return PlacementPoint{ test_cx, test_cy };
            }
        }

        return PlacementPoint{ fallback_cx, fallback_cy };
    }

    PlacementPoint placement_fixture_center(float tile_size = 16.0f) const {
        return placement_fixture_center(transform.x, transform.y, tile_size);
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

    int base_facing_angle_deg() const {
        if (facing_dx > 0.5f && facing_dy > 0.5f) return 45;
        if (facing_dx > 0.5f && facing_dy < -0.5f) return 315;
        if (facing_dx < -0.5f && facing_dy > 0.5f) return 135;
        if (facing_dx < -0.5f && facing_dy < -0.5f) return 225;
        if (facing_dx > 0.5f) return 0;
        if (facing_dx < -0.5f) return 180;
        if (facing_dy > 0.5f) return 90;
        if (facing_dy < -0.5f) return 270;
        return 90; // Default down
    }

    Collision::Circle calculate_attack_circle_at(float progress, float px, float py) const {
        int base_deg = base_facing_angle_deg();
        int offset_deg = ATTACK_ARC_SWEEP_START_DEG + static_cast<int>(progress * ATTACK_ARC_SWEEP_SWING_DEG);
        int angle_deg = base_deg + offset_deg;

        float pcx = px + (transform.width / 2.0f);
        float pcy = py + (transform.height / 2.0f);
        int reach = static_cast<int>(ATTACK_REACH_RADIUS);

        int cx = static_cast<int>(pcx) + ((reach * TrigLUT::cos(angle_deg)) >> TrigLUT::SHIFT);
        int cy = static_cast<int>(pcy) + ((reach * TrigLUT::sin(angle_deg)) >> TrigLUT::SHIFT);

        return Collision::Circle{ static_cast<float>(cx), static_cast<float>(cy), ATTACK_HIT_RADIUS };
    }

    bool is_attacking() const {
        return attack_phase == AttackPhase::ActiveSweep;
    }

    Collision::Circle attack_hit_circle(float alpha = 1.0f) const {
        float px = Draw::interpolate(transform_prev.x, transform.x, alpha);
        float py = Draw::interpolate(transform_prev.y, transform.y, alpha);
        float p = Draw::interpolate(swing_progress_prev, swing_progress_curr, alpha);
        return calculate_attack_circle_at(p, px, py);
    }

    bool try_build_tile(const Tiles& tiles, Network& network) {
        int cost = fixture_cost(m_selected_fixture_type);
        if (m_cursed_alloy < cost) return false;

        float tile_sz = static_cast<float>(tiles.tile_size());
        PlacementPoint pt = placement_fixture_center(tile_sz);
        GridPos target_pos{
            static_cast<int16_t>(static_cast<int>(std::floor(pt.cx / tile_sz))),
            static_cast<int16_t>(static_cast<int>(std::floor(pt.cy / tile_sz)))
        };

        // Self-overlap guard: reject placement if fixture's solid ground box intersects player's ground circle
        Collision::AABB proposed_aabb = fixture_ground_aabb(target_pos.x, target_pos.y, tile_sz, m_selected_fixture_type);
        if (Collision::circle_vs_aabb(ground_circle(), proposed_aabb)) {
            return false;
        }

        if (network.can_place_fixture(target_pos, m_selected_fixture_type, tiles)) {
            m_cursed_alloy -= cost;
            network.place_fixture(target_pos, m_selected_fixture_type);
            return true;
        }
        return false;
    }

    bool try_remove_tile(const Tiles& tiles, Network& network) {
        float tile_sz = static_cast<float>(tiles.tile_size());
        PlacementPoint pt = placement_fixture_center(tile_sz);
        GridPos target_pos{
            static_cast<int16_t>(static_cast<int>(std::floor(pt.cx / tile_sz))),
            static_cast<int16_t>(static_cast<int>(std::floor(pt.cy / tile_sz)))
        };

        if (network.in_bounds(target_pos)) {
            const Fixture& fix = network.fixture(target_pos);
            if (fix.type != FixtureType::None && fix.type != FixtureType::Seep) {
                int refund = fixture_cost(fix.type);
                m_cursed_alloy += refund;
                network.remove_fixture(target_pos);
                return true;
            }
        }
        return false;
    }

    void update(float dt, const Tiles& tiles, Network& network, const alx::Camera& camera) {
        sync_prev_transforms();

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
        if (auto* sprite = std::get_if<SpriteRender>(&visual)) {
            Draw::sprite(
                world_draw_x,
                world_draw_y,
                sprite->pixels,
                sprite->pixels_size,
                sprite->width,
                sprite->height,
                transform.z_index,
                static_cast<int>(world_bottom_y) // sort Y override
            );
        }

        // Ground ground collision circle outline
        if (Debug::DRAW_GROUND_AREAS) {
            Collision::Circle ground = ground_circle(world_draw_x, world_draw_y);
            Draw::circle(
                ground.cx,
                ground.cy,
                ground.radius,
                0xFF00FFFF, // Bright Cyan debug outline
                false,      // fill = false (outline only)
                1,          // thickness = 1
                transform.z_index + 1,
                static_cast<int>(world_bottom_y)
            );
        }

        // Ground hurt collision circle outline
        if (Debug::DRAW_HURT_AREAS) {
            Collision::Circle hurt = hurt_circle();
            Draw::circle(
                hurt.cx,
                hurt.cy,
                hurt.radius,
                0xFFFFFF00, // Bright Yellow debug outline
                false,      // fill = false (outline only)
                1,          // thickness = 1
                transform.z_index + 1,
                static_cast<int>(world_bottom_y)
            );
        }

        // Attack hit circle (2px inner-outlined circle)
        if (is_attacking() && (Debug::DRAW_MELEE_ARCS)) {
            Collision::Circle hit_c = attack_hit_circle(alpha);
            Draw::circle(
                hit_c.cx,
                hit_c.cy,
                hit_c.radius,
                0xFF00FFFF, // Bright Cyan debug circle
                false,      // fill = false (outline only)
                1,          // thickness = 2 (2px inner-outlined circle)
                transform.z_index + 2,
                static_cast<int>(world_bottom_y) // sort Y override
            );
        }

        if (Action::is_pressed(Action::Build)) {
            PlacementPoint pt = placement_fixture_center(world_draw_x, world_draw_y, 16.0f);

            float tile_sz = 16.0f;
            float tile_x = std::floor(pt.cx / tile_sz) * tile_sz;
            float tile_y = std::floor(pt.cy / tile_sz) * tile_sz;
            Draw::rect(
                tile_x,
                tile_y,
                tile_sz,
                tile_sz,
                0xFF00FFFF, // 1px Cyan border outline
                false,      // fill = false
                1,          // thickness = 1
                transform.z_index + 1,
                static_cast<int>(world_bottom_y)
            );
        }
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
        is_panning = Action::is_pan_mode_active();

        if (camera.is_player_movement_locked()) {
            return;
        }

        if (is_attacking()) {
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

        // NOTE: WorldCollision::try_move() prevents geometry penetration during normal gameplay.
        // Enable ejection safety net if adding heavy knockback, teleports, or phase-dashes.
        // WorldCollision::enforce_solid_ground_ejection(transform.x, transform.y, ground_circle(), tiles, network, 2.0f, tag);

        WorldCollision::try_move(transform.x, transform.y, dx * speed * dt, dy * speed * dt, ground_circle(), tiles, network);
    }

    void update_actions(float dt, const Tiles& tiles, Network& network) {
        if (attack_phase == AttackPhase::ActiveSweep) {
            swing_progress_prev = swing_progress_curr;
            attack_timer += dt;
            if (attack_timer >= ATTACK_SWEEP_DURATION) {
                swing_progress_curr = 1.0f;
                attack_phase = AttackPhase::Recovery;
                attack_timer = 0.0f;
            } else {
                swing_progress_curr = attack_timer / ATTACK_SWEEP_DURATION;
            }
        } else if (attack_phase == AttackPhase::Recovery) {
            attack_timer += dt;
            if (attack_timer >= ATTACK_RECOVERY_DURATION) {
                attack_phase = AttackPhase::Idle;
                attack_timer = 0.0f;
                swing_progress_prev = 0.0f;
                swing_progress_curr = 0.0f;
            }
        }

        if (Action::is_attack() && attack_phase == AttackPhase::Idle) {
            sync_prev_transforms();
            attack_phase = AttackPhase::ActiveSweep;
            attack_timer = 0.0f;
            swing_progress_prev = 0.0f;
            swing_progress_curr = 0.0f;
            current_swing_id++;
        }

        if (!is_attacking()) {
            if (Action::is_build_cycle()) {
                if (m_selected_fixture_type == FixtureType::Pipe) {
                    m_selected_fixture_type = FixtureType::Refiner;
                } else if (m_selected_fixture_type == FixtureType::Refiner) {
                    m_selected_fixture_type = FixtureType::Spire;
                } else {
                    m_selected_fixture_type = FixtureType::Pipe;
                }
            }

            if (Action::is_just_pressed(Action::DebugResource)) {
                m_cursed_alloy += 10;
            }

            if (Action::is_build_tile()) {
                try_build_tile(tiles, network);
            }

            if (Action::is_remove_tile()) {
                try_remove_tile(tiles, network);
            }
        }
    }
};

} // namespace alx
