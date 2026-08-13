#pragma once
#include <vector>
#include <cstdint>
#include "core/Collision.h"
#include "core/Entity.h"
#include "core/Facing.h"
#include "alx/Fixture.h"
#include "alx/GridPos.h"
#include "Game.h"

namespace alx {

struct Camera;
class Tiles;
class Network;
class ParticleSystem;
struct WorldStructure;

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
    FacingVector update_facing(float dt, float raw_dx, float raw_dy);
};

struct Player : public Entity {
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
    bool is_facing_left = false;

    // Cardinal facing vector (strictly 4-way orthogonal for placement & reticle)
    float m_cardinal_facing_dx = 0.0f;
    float m_cardinal_facing_dy = 1.0f;

    // Hysteresis & Hemisphere direction angle threshold cosines for placement lock
    static constexpr float COS_45_DEG  = 0.70710678f;  // cos(45 deg): forward cone limit
    static constexpr float COS_60_DEG  = 0.50000000f;  // cos(60 deg): hard sideways deflection threshold
    static constexpr float COS_135_DEG = -0.70710678f; // cos(135 deg): rear 90-deg hemisphere cone limit

    enum class AttackPhase { Idle, ActiveSweep, Recovery };

    float m_charge_timer = 0.0f;
    bool m_is_charging_spark = false;

    // Collision areas ratios
    static constexpr float GROUND_RADIUS_RATIO = 0.25f;   // % of transform.width
    static constexpr float GROUND_OFFSET_Y_RATIO = 1.00f; // Bottom aligned (transform.y + transform.height - r)
    static constexpr float HURT_RADIUS_RATIO = 0.25f;     // % of transform.width
    static constexpr float HURT_OFFSET_Y_RATIO = 0.50f;   // Torso center (transform.y + transform.height * %)

    // Attack timing and radius
    static constexpr float ATTACK_SWEEP_DURATION    = 0.15f; // 0.15s active arc sweep
    static constexpr float ATTACK_RECOVERY_DURATION = 0.15f; // 0.10s recovery delay
    static constexpr float ATTACK_REACH_RADIUS      = 12.0f; // Reach distance from player center
    static constexpr float ATTACK_HIT_RADIUS        = 8.0f;  // Radius of hit circle
    static constexpr int ATTACK_ARC_SWEEP_START_DEG = -60;   // in degrees (where swing starts)
    static constexpr int ATTACK_ARC_SWEEP_SWING_DEG = 125;    // in degrees (total swing motion)
    static constexpr float ATTACK_KNOCKBACK_SPEED   = 115.0f;

    // Shadow ratios
    static constexpr float SHADOW_RX_RATIO = 0.45f;
    static constexpr float SHADOW_RY_RATIO_OF_RX = 0.45f;

    struct State {
        static constexpr int DEFAULT_MAX_HP = 5;
        static constexpr float IFRAME_DURATION = 0.5f;
        static constexpr float DEFEAT_DURATION = 2.0f;

        int hp = DEFAULT_MAX_HP;
        int max_hp = DEFAULT_MAX_HP;
        float iframe_timer = 0.0f;
        bool defeated = false;
        float defeat_timer = 0.0f;
    };

    State state;

    AttackPhase attack_phase = AttackPhase::Idle;
    float attack_timer = 0.0f;
    uint32_t current_swing_id = 0;
    float swing_progress_prev = 0.0f;
    float swing_progress_curr = 0.0f;

    Player(float x = 128.0f, float y = 128.0f);

    bool take_damage(int amount);
    bool is_invulnerable() const { return state.iframe_timer > 0.0f || state.defeated; }


    float center_x(float alpha = 1.0f) const;
    float center_y(float alpha = 1.0f) const;

    Collision::Circle ground_circle(float px, float py) const;
    Collision::Circle ground_circle() const;

    struct PlacementPoint {
        float cx = 0.0f;
        float cy = 0.0f;
    };

    PlacementPoint placement_fixture_center(float px, float py, float tile_size = 16.0f) const;
    PlacementPoint placement_fixture_center(float tile_size = 16.0f) const;

    Collision::Circle hurt_circle(float px, float py) const;
    Collision::Circle hurt_circle() const;

    void sync_prev_transforms();

    Facing::Type facing() const;
    bool is_facing(Facing::Type dir) const;
    int base_facing_angle_deg() const;

    Collision::Circle calculate_attack_circle_at(float progress, float px, float py) const;
    bool is_attacking() const;
    Collision::Circle attack_hit_circle(float alpha = 1.0f) const;

    bool try_build_tile(const Tiles& tiles, Network& network, ParticleSystem* particle_system = nullptr);
    bool try_remove_tile(const Tiles& tiles, Network& network);

    void update(float dt, const Tiles& tiles, Network& network, const alx::Camera& camera, bool can_build, const std::vector<WorldStructure>* structures = nullptr, ParticleSystem* particle_system = nullptr);
    void draw(std::vector<uint32_t>& screen_buffer, float alpha, bool can_build, const Tiles* tiles = nullptr, const Network* network = nullptr);

    int cursed_alloy() const { return m_cursed_alloy; }
    void add_cursed_alloy(int amount) { m_cursed_alloy += amount; }
    FixtureType selected_fixture_type() const { return m_selected_fixture_type; }

    static int fixture_cost(FixtureType type);

    bool m_pending_spark = false;
    bool consume_mana_spark_fire() {
        if (m_pending_spark) {
            m_pending_spark = false;
            return true;
        }
        return false;
    }

private:
    int m_cursed_alloy = 5;
    FixtureType m_selected_fixture_type = FixtureType::Pipe;
    bool m_was_blocked_prev = false;

    // Continuous Pipe Line Drag tracking state
    GridPos m_last_drag_tile_pos{-32768, -32768};
    bool m_played_shortage_sfx = false;

    void update_movement(float dt, const Tiles& tiles, const Network& network, const alx::Camera& camera, const std::vector<WorldStructure>* structures = nullptr);
    void update_actions(float dt, const Tiles& tiles, Network& network, bool can_build, ParticleSystem* particle_system = nullptr);
    void update_build_actions(const Tiles& tiles, Network& network, ParticleSystem* particle_system = nullptr);
};

} // namespace alx
