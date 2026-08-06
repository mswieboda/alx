#include "alx/EnemyMovement.h"
#include "alx/Enemy.h"
#include "alx/Random.h"
#include "alx/WorldCollision.h"
#include <algorithm>
#include <vector>

namespace alx::EnemyMovement {

constexpr float inv_sqrt2 = 0.70710678118f;

const std::pair<float, float> DIRS[8] = {
    {1.0f, 0.0f},           // 0: Right (0 deg)
    {inv_sqrt2, inv_sqrt2}, // 1: Down-Right (45 deg)
    {0.0f, 1.0f},           // 2: Down (90 deg)
    {-inv_sqrt2, inv_sqrt2},// 3: Down-Left (135 deg)
    {-1.0f, 0.0f},          // 4: Left (180 deg)
    {-inv_sqrt2, -inv_sqrt2},// 5: Up-Left (225 deg)
    {0.0f, -1.0f},          // 6: Up (270 deg)
    {inv_sqrt2, -inv_sqrt2} // 7: Up-Right (315 deg)
};

static int get_nearest_dir_idx(float dx, float dy) {
    float max_dot = -1e9f;
    int best_idx = 0;
    for (int i = 0; i < 8; ++i) {
        float dot = dx * DIRS[i].first + dy * DIRS[i].second;
        if (dot > max_dot) {
            max_dot = dot;
            best_idx = i;
        }
    }
    return best_idx;
}

// Step 1: Facing 3-Cone (Current Facing + 2 Nearest Neighbors)
static std::vector<int> get_facing_3_cone(const Enemy& enemy) {
    int facing_idx = 0;
    if (enemy.move_dx != 0.0f || enemy.move_dy != 0.0f) {
        facing_idx = get_nearest_dir_idx(enemy.move_dx, enemy.move_dy);
    } else {
        facing_idx = Random::get_int(0, 7);
    }
    return {
        facing_idx,
        (facing_idx + 1) % 8,
        (facing_idx + 7) % 8
    };
}

// Steps 2+: Forward 5 Cone (0 deg, +-45 deg, +-90 deg)
static std::vector<int> get_forward_5_cone(int last_dir_idx) {
    if (last_dir_idx < 0) {
        return {0, 1, 2, 3, 4, 5, 6, 7};
    }
    return {
        last_dir_idx,
        (last_dir_idx + 1) % 8,
        (last_dir_idx + 7) % 8,
        (last_dir_idx + 2) % 8,
        (last_dir_idx + 6) % 8
    };
}

void reset_wander_state(MovementState& state) {
    state.micro_step_timer = 0.0f;
    state.last_dir_idx = -1;
    state.was_moving = false;
}

void handle_wall_collision(Enemy& enemy, MovementState& state, const Tiles& tiles, const Network& network, const WanderConfig& config, const std::vector<WorldStructure>* structures) {
    std::vector<int> candidate_indices = (state.last_dir_idx < 0)
        ? get_facing_3_cone(enemy)
        : get_forward_5_cone(state.last_dir_idx);

    std::shuffle(candidate_indices.begin(), candidate_indices.end(), Random::engine());

    for (int idx : candidate_indices) {
        auto [dx, dy] = DIRS[idx];
        float test_x = enemy.transform.x + dx * (enemy.speed * 0.2f);
        float test_y = enemy.transform.y + dy * (enemy.speed * 0.2f);
        if (!WorldCollision::is_solid_ground(enemy.ground_circle(test_x, test_y), tiles, network, structures)) {
            enemy.move_dx = dx;
            enemy.move_dy = dy;
            if (dx != 0.0f || dy != 0.0f) {
                enemy.facing_dx = dx;
                enemy.facing_dy = dy;
            }
            enemy.is_moving = true;
            state.last_dir_idx = idx;
            state.was_moving = true;
            state.micro_step_timer = Random::get_float(config.step_min_time, config.step_max_time);
            return;
        }
    }

    // If all candidates are blocked, pause briefly
    enemy.move_dx = 0.0f;
    enemy.move_dy = 0.0f;
    enemy.is_moving = false;
    state.was_moving = false;
    state.micro_step_timer = Random::get_float(config.step_min_time, config.step_max_time);
}

void update_wander_step(Enemy& enemy, MovementState& state, float dt, const Tiles& tiles, const Network& network, const WanderConfig& config, const std::vector<WorldStructure>* structures) {
    state.micro_step_timer -= dt;
    if (state.micro_step_timer > 0.0f && enemy.is_moving) {
        return; // Continue current micro-step
    }

    // Timer expired or was idle: pick next micro-step
    bool should_pause = false;
    if (state.was_moving && Random::chance(config.pause_chance)) {
        should_pause = true;
    }

    if (should_pause) {
        enemy.move_dx = 0.0f;
        enemy.move_dy = 0.0f;
        enemy.is_moving = false;
        state.was_moving = false;
        state.micro_step_timer = Random::get_float(config.step_min_time, config.step_max_time);
    } else {
        std::vector<int> candidate_indices = (state.last_dir_idx < 0)
            ? get_facing_3_cone(enemy)
            : get_forward_5_cone(state.last_dir_idx);

        std::shuffle(candidate_indices.begin(), candidate_indices.end(), Random::engine());

        int chosen_idx = candidate_indices.front();
        for (int idx : candidate_indices) {
            auto [dx, dy] = DIRS[idx];
            float test_x = enemy.transform.x + dx * (enemy.speed * 0.2f);
            float test_y = enemy.transform.y + dy * (enemy.speed * 0.2f);
            if (!WorldCollision::is_solid_ground(enemy.ground_circle(test_x, test_y), tiles, network, structures)) {
                chosen_idx = idx;
                break;
            }
        }

        auto [dx, dy] = DIRS[chosen_idx];
        enemy.move_dx = dx;
        enemy.move_dy = dy;
        if (dx != 0.0f || dy != 0.0f) {
            enemy.facing_dx = dx;
            enemy.facing_dy = dy;
        }
        enemy.is_moving = true;
        state.last_dir_idx = chosen_idx;
        state.was_moving = true;
        state.micro_step_timer = Random::get_float(config.step_min_time, config.step_max_time);
    }
}

} // namespace alx::EnemyMovement
