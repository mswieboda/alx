#pragma once
#include <cstdint>
#include <vector>
#include "core/Entity.h"
#include "core/Collision.h"
#include "alx/EnemyMovement.h"
#include "alx/GridPos.h"
#include "Game.h"

namespace alx {

struct EnemyThreatConstants {
    static constexpr float THREAT_LIGHT_SPIRE_REFINER = 400.0f; // Top threat: Active Light Spires / Refiners
    static constexpr float THREAT_DARK_SPIRE_REFINER  = 300.0f; // Inactive / Dark Spires / Refiners
    static constexpr float THREAT_LIGHT_PIPE          = 200.0f; // Active Light Mana Conduit
    static constexpr float THREAT_DARK_PIPE           = 100.0f; // Inactive / Dark Pipe
    static constexpr float CROWD_PENALTY_PER_ENEMY    = 80.0f;  // Score penalty per enemy targeting same cell
    static constexpr float TARGET_LOCK_DURATION       = 5.0f;  // Lock duration before re-evaluating target
};

struct EnemyAggroConstants {
    static constexpr float AGGRO_DETECTION_RADIUS   = 40.0f;  // ~2.5 tiles (40px)
    static constexpr float AGGRO_CHECK_INTERVAL_MIN = 0.50f;  // Min check interval (sec)
    static constexpr float AGGRO_CHECK_INTERVAL_MAX = 2.00f;  // Max check interval (sec)
    static constexpr float BASE_AGGRO_CHANCE        = 0.45f;  // % baseline aggro roll
    static constexpr float ACTION_AGGRO_CHANCE      = 0.65f;  // % aggro roll when player attacks
    static constexpr float LEASH_RADIUS             = 48.0f;  // ~3 tiles (48px) leash drop radius
    static constexpr float PROVOKED_AGGRO_DURATION  = 3.0f;   // 3.0s provoked aggro decay timer
};

struct EnemyDebugConstants {
    static constexpr float AGGRO_PULSE_DURATION = 0.5f; // 0.5s visual pulse when aggro check occurs
    static constexpr int NORMAL_AGGRO_THICKNESS = 1;
    static constexpr int PULSE_AGGRO_THICKNESS  = 3;
    static constexpr uint32_t COLOR_AGGRO_DEFAULT = 0xCCFF9900; // Light Orange
    static constexpr uint32_t COLOR_AGGRO_CHASING = 0xCCFF0000; // Bright Red when chasing player
};

enum class EnemyState : uint8_t {
    Wander,
    SeekTarget,
    RestlessWander,
    DetourWander,
    HitStun,
    AttackWindup,
    AttackRecoilRest,
    ChasePlayer
};

struct Enemy : public Entity {
    // Default size, color
    static constexpr float DEFAULT_WIDTH = 16.0f;
    static constexpr float DEFAULT_HEIGHT = 16.0f;
    static constexpr uint32_t COLOR = 0xFF800080; // Dusky Purple

    // Speed
    static constexpr float SPEED_PX_PER_TICK = 0.5f; // 0.25 px/tick at 60 FPS
    static constexpr float SPEED = SPEED_PX_PER_TICK * Game::TARGET_FPS; // 15.0 px/s

    static constexpr int DEFAULT_MAX_HP = 3;

    static constexpr float KNOCKBACK_DIST = 16.0f;
    static constexpr float GROUND_RADIUS_RATIO = 0.25f;   // % of width
    static constexpr float GROUND_OFFSET_Y_RATIO = 1.00f; // Bottom aligned (y + height - r)
    static constexpr float HURT_RADIUS_RATIO = 0.30f;     // % of width
    static constexpr float HURT_OFFSET_Y_RATIO = 0.50f;   // Center Y (y + height * %)

    static constexpr float HIT_STUN_DURATION = 0.3f;
    static constexpr float MIN_IDLE_TIME = 0.5f;
    static constexpr float MAX_IDLE_TIME = 1.5f;
    static constexpr float MIN_MOVE_TIME = 1.0f;
    static constexpr float MAX_MOVE_TIME = 2.5f;

    // Movement & Combat
    static constexpr float WANDER_DURATION           = 5.0f;  // Initial spawn delay / wander duration
    static constexpr float POST_DESTROY_WANDER_TIME  = 2.0f;  // Search pause when target fixture is destroyed
    static constexpr float MARCH_INTERMISSION_WANDER_TIME = 1.5f; // Intermission micro-wander when march timer expires
    static constexpr float SIEGE_MARCH_DURATION      = 7.0f;  // Active march duration toward target fixture
    static constexpr float RESTLESS_WANDER_DURATION  = 2.5f;  // Intermission wander duration
    static constexpr float OBSTACLE_STUCK_THRESHOLD  = 1.5f;  // Seconds spent against obstacle before detour wander
    static constexpr float DETOUR_WANDER_DURATION    = 3.0f;  // Detour wander duration around obstacles
    static constexpr float TARGET_REEVAL_MIN_TIME    = 1.0f;  // Target re-evaluation min interval
    static constexpr float TARGET_REEVAL_MAX_TIME    = 3.0f;  // Target re-evaluation max interval
    static constexpr float AGGRO_DETECTION_RADIUS    = 64.0f; // 4 tiles (64px)
    static constexpr float ATTACK_WINDUP_TIME        = 0.3f;  // Attack windup telegraph
    static constexpr float RECOIL_DIST               = 8.0f;  // Recoil step-back distance (px)
    static constexpr float RECOIL_SLIDE_SPEED        = 50.0f; // Recoil slide speed (px/s)
    static constexpr float RECOVERY_REST_MIN_TIME    = 1.0f;  // Recoil rest min time
    static constexpr float RECOVERY_REST_MAX_TIME    = 2.0f;  // Recoil rest max time
    static constexpr float TARGET_LOCK_DURATION       = 5.0f;  // Target lock duration to prevent flickering
    static constexpr float ATTACK_STRIKE_OFFSET       = 10.0f; // Melee swipe directional offset
    static constexpr float ATTACK_STRIKE_RADIUS       = 14.0f; // Melee swipe hit detection radius

    int hp = DEFAULT_MAX_HP;
    float speed = SPEED;
    float move_dx = 0.0f;
    float move_dy = 0.0f;
    float facing_dx = 0.0f;
    float facing_dy = 1.0f;
    float recoil_dx = 0.0f;
    float recoil_dy = 0.0f;
    float recoil_dist_remaining = 0.0f;
    float knockback_dx = 0.0f;
    float knockback_dy = 0.0f;
    float knockback_speed = 0.0f;
    float initial_knockback_speed = 0.0f;
    float state_timer = WANDER_DURATION;
    float reeval_timer = 0.0f;
    float target_lock_timer = 0.0f;
    float aggro_check_timer = 0.0f;
    float stuck_timer = 0.0f;
    bool is_moving = false;
    bool is_provoked = false;
    float provoked_timer = 0.0f;

#if ALX_ENABLE_DEBUG
    float debug_aggro_pulse_timer = 0.0f;
#endif

    // [EBS]: Multi-wave continuous bleed tracking fields
    float hit_wound_offset_x = 0.0f;
    float hit_wound_offset_y = 0.0f;
    int bleed_waves_left = 0;
    float bleed_timer = 0.0f;

    EnemyState state = EnemyState::Wander;
    EnemyMovement::MovementState move_state;
    GridPos target_fixture_pos{-1, -1};
    bool has_target = false;
    bool target_is_player = false;
    uint32_t last_hit_swing_id = 0;

    Enemy(float px = 0.0f, float py = 0.0f, float w = DEFAULT_WIDTH, float h = DEFAULT_HEIGHT, uint32_t col = COLOR, int max_hp = DEFAULT_MAX_HP);

    float center_x(float alpha = 1.0f) const;
    float center_y(float alpha = 1.0f) const;

    Collision::Circle ground_circle(float px, float py) const;
    Collision::Circle ground_circle() const { return ground_circle(transform.x, transform.y); }

    Collision::Circle hurt_circle(float px, float py) const;
    Collision::Circle hurt_circle() const { return hurt_circle(transform.x, transform.y); }

    void sync_prev_transforms() { transform_prev = transform; }

    void set_steering_vector_8way(float target_world_x, float target_world_y);
    void take_damage(int amount, float kb_dx, float kb_dy, float kb_speed = 250.0f, float wound_ox = 0.0f, float wound_oy = 0.0f);

    bool is_dead() const { return hp <= 0; }

    void draw(std::vector<uint32_t>& screen_buffer, float alpha) const;

private:
    void draw_shadow(float draw_x, float draw_y, float draw_w, float draw_h, int sort_y) const;
    void draw_body(float draw_x, float draw_y, float draw_w, float draw_h, int sort_y) const;
    void draw_debug_overlays(float draw_x, float draw_y, float draw_w, float draw_h, int sort_y) const;
};

} // namespace alx

