#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <random>
#include <utility>
#include <vector>
#include "core/Draw.h"
#include "core/Entity.h"
#include "core/Collision.h"
#include "alx/Camera.h"
#include "alx/Layer.h"
#include "alx/DrawFX.h"
#include "Game.h"

namespace alx {

struct EnemyConstants {
    static constexpr float DEFAULT_WIDTH = 16.0f;
    static constexpr float DEFAULT_HEIGHT = 16.0f;
    static constexpr uint32_t COLOR = 0xFF800080; // Dusky Purple
    static constexpr int DEFAULT_MAX_HP = 3;
    static constexpr float KNOCKBACK_DIST = 2.0f;
    static constexpr float GROUND_RADIUS_RATIO = 0.375f;   // 37.5% of width (6.0px)
    static constexpr float GROUND_OFFSET_Y_RATIO = 1.00f; // Bottom aligned (y + height - r)
    static constexpr float HURT_RADIUS_RATIO = 0.4375f;   // 43.75% of width (7.0px)
    static constexpr float HURT_OFFSET_Y_RATIO = 0.50f;   // Center Y (y + height * 0.5)

    static constexpr float DEFAULT_SPEED = 45.0f;          // 0.75 px/tick at 60 FPS
    static constexpr float HIT_STUN_DURATION = 0.3f;
    static constexpr float MIN_IDLE_TIME = 0.5f;
    static constexpr float MAX_IDLE_TIME = 1.5f;
    static constexpr float MIN_MOVE_TIME = 1.0f;
    static constexpr float MAX_MOVE_TIME = 2.5f;

    // --- Phase 3 AI Movement Constants ---
    static constexpr float SPAWN_WANDER_DURATION     = 5.0f;  // Initial spawn delay before target locking
    static constexpr float POST_DESTROY_WANDER_TIME  = 5.0f;  // Search pause when target fixture is destroyed
    static constexpr float SIEGE_MARCH_DURATION      = 7.0f;  // Active march duration toward target fixture
    static constexpr float RESTLESS_WANDER_DURATION  = 2.5f;  // Intermission wander duration
    static constexpr float OBSTACLE_STUCK_THRESHOLD  = 1.5f;  // Seconds spent against obstacle before detour wander
    static constexpr float DETOUR_WANDER_DURATION    = 3.0f;  // Detour wander duration around obstacles
    static constexpr float TARGET_REEVAL_MIN_TIME    = 1.0f;  // Target re-evaluation min interval
    static constexpr float TARGET_REEVAL_MAX_TIME    = 3.0f;  // Target re-evaluation max interval
};

enum class EnemyState : uint8_t {
    SpawnWander,
    SeekTarget,
    RestlessWander,
    DetourWander,
    HitStun
};

struct Enemy : public Entity {
    int hp = EnemyConstants::DEFAULT_MAX_HP;

    float speed = EnemyConstants::DEFAULT_SPEED;
    float move_dx = 0.0f;
    float move_dy = 0.0f;
    float state_timer = EnemyConstants::SPAWN_WANDER_DURATION;
    float reeval_timer = 0.0f;
    float stuck_timer = 0.0f;
    bool is_moving = false;

    EnemyState state = EnemyState::SpawnWander;
    GridPos target_fixture_pos{-1, -1};
    bool has_target = false;

    Enemy(float px = 0.0f, float py = 0.0f, float w = EnemyConstants::DEFAULT_WIDTH, float h = EnemyConstants::DEFAULT_HEIGHT, uint32_t col = EnemyConstants::COLOR, int max_hp = EnemyConstants::DEFAULT_MAX_HP)
        : Entity(
            Transform{ px, py, w, h, Layer::WorldObj },
            RectangleRender{ col, true, 1 },
            true,
            "enemy"
          ),
          hp(max_hp)
    {}

    float center_x(float alpha = 1.0f) const {
        float draw_x = Draw::interpolate(transform_prev.x, transform.x, alpha);
        return draw_x + (transform.width / 2.0f);
    }

    float center_y(float alpha = 1.0f) const {
        float draw_y = Draw::interpolate(transform_prev.y, transform.y, alpha);
        return draw_y + (transform.height / 2.0f);
    }

    Collision::Circle ground_circle(float px, float py) const {
        float r = transform.width * EnemyConstants::GROUND_RADIUS_RATIO;
        float cy = py + (transform.height * EnemyConstants::GROUND_OFFSET_Y_RATIO) - r;
        return Collision::Circle{ px + (transform.width / 2.0f), cy, r };
    }

    Collision::Circle ground_circle() const {
        return ground_circle(transform.x, transform.y);
    }

    Collision::Circle hurt_circle(float px, float py) const {
        float r = transform.width * EnemyConstants::HURT_RADIUS_RATIO;
        float cy = py + (transform.height * EnemyConstants::HURT_OFFSET_Y_RATIO);
        return Collision::Circle{ px + (transform.width / 2.0f), cy, r };
    }

    Collision::Circle hurt_circle() const {
        return hurt_circle(transform.x, transform.y);
    }

    void sync_prev_transforms() {
        transform_prev = transform;
    }

    void set_steering_vector_8way(float target_world_x, float target_world_y) {
        float dx = target_world_x - (transform.x + transform.width * 0.5f);
        float dy = target_world_y - (transform.y + transform.height * 0.5f);

        float len_sq = dx * dx + dy * dy;
        if (len_sq < 0.0001f) {
            move_dx = 0.0f;
            move_dy = 0.0f;
            is_moving = false;
            return;
        }

        float angle = std::atan2(dy, dx);
        constexpr float pi = 3.14159265358979323846f;
        constexpr float inv_sqrt2 = 0.70710678118f;

        static constexpr std::pair<float, float> dirs8[8] = {
            {1.0f, 0.0f}, {inv_sqrt2, inv_sqrt2}, {0.0f, 1.0f}, {-inv_sqrt2, inv_sqrt2},
            {-1.0f, 0.0f}, {-inv_sqrt2, -inv_sqrt2}, {0.0f, -1.0f}, {inv_sqrt2, -inv_sqrt2}
        };

        float normalized_angle = angle;
        if (normalized_angle < 0.0f) normalized_angle += 2.0f * pi;

        int index = static_cast<int>(std::round(8.0f * (normalized_angle / (2.0f * pi)))) % 8;
        move_dx = dirs8[index].first;
        move_dy = dirs8[index].second;
        is_moving = true;
    }

    void pick_random_wander_state(std::mt19937& rng) {
        std::uniform_real_distribution<float> prob_dist(0.0f, 1.0f);
        if (prob_dist(rng) < 0.3f) {
            is_moving = false;
            move_dx = 0.0f;
            move_dy = 0.0f;
            std::uniform_real_distribution<float> idle_dist(EnemyConstants::MIN_IDLE_TIME, EnemyConstants::MAX_IDLE_TIME);
            state_timer = idle_dist(rng);
        } else {
            is_moving = true;
            constexpr float inv_sqrt2 = 0.70710678118f;
            static constexpr std::pair<float, float> dirs[8] = {
                {1.0f, 0.0f}, {-1.0f, 0.0f}, {0.0f, 1.0f}, {0.0f, -1.0f},
                {inv_sqrt2, inv_sqrt2}, {-inv_sqrt2, inv_sqrt2},
                {inv_sqrt2, -inv_sqrt2}, {-inv_sqrt2, -inv_sqrt2}
            };
            std::uniform_int_distribution<int> dir_dist(0, 7);
            auto [dx, dy] = dirs[dir_dist(rng)];
            move_dx = dx;
            move_dy = dy;
            std::uniform_real_distribution<float> move_time_dist(EnemyConstants::MIN_MOVE_TIME, EnemyConstants::MAX_MOVE_TIME);
            state_timer = move_time_dist(rng);
        }
    }

    void take_damage(int amount, float push_dx, float push_dy) {
        hp -= amount;
        transform.x += push_dx * EnemyConstants::KNOCKBACK_DIST;
        transform.y += push_dy * EnemyConstants::KNOCKBACK_DIST;
        is_moving = false;
        move_dx = 0.0f;
        move_dy = 0.0f;
        state = EnemyState::HitStun;
        state_timer = EnemyConstants::HIT_STUN_DURATION;
    }

    bool is_dead() const {
        return hp <= 0;
    }

    void draw(std::vector<uint32_t>& screen_buffer, float alpha) const {
        if (!active) return;

        float world_draw_x = Draw::interpolate(transform_prev.x, transform.x, alpha);
        float world_draw_y = Draw::interpolate(transform_prev.y, transform.y, alpha);
        float world_draw_w = transform.width;
        float world_draw_h = transform.height;
        float world_bottom_y = world_draw_y + world_draw_h;

        // Enemy shadow underneath enemy at bottom Y edge (foreshortened oval)
        DrawFX::shadow(
            world_draw_x,
            world_draw_y,
            world_draw_w,
            world_draw_h,
            transform.z_index
        );

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
                static_cast<int>(world_bottom_y)
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
    }
};

} // namespace alx
