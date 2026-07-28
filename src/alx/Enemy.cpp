#include "alx/Enemy.h"
#include "alx/EnemyManager.h"
#include "alx/Random.h"
#include "alx/Tiles.h"
#include "alx/Network.h"
#include "alx/WorldCollision.h"

namespace alx {

void Enemy::pick_random_wander_state(const Tiles* tiles, const Network* network) {
    if (Random::chance(0.3f)) {
        is_moving = false;
        move_dx = 0.0f;
        move_dy = 0.0f;
        state_timer = Random::get_float(MIN_IDLE_TIME, MAX_IDLE_TIME);
    } else {
        constexpr float inv_sqrt2 = 0.70710678118f;
        static constexpr std::pair<float, float> dirs[8] = {
            {1.0f, 0.0f}, {-1.0f, 0.0f}, {0.0f, 1.0f}, {0.0f, -1.0f},
            {inv_sqrt2, inv_sqrt2}, {-inv_sqrt2, inv_sqrt2},
            {inv_sqrt2, -inv_sqrt2}, {-inv_sqrt2, -inv_sqrt2}
        };

        std::vector<int> candidate_indices = {0, 1, 2, 3, 4, 5, 6, 7};
        std::shuffle(candidate_indices.begin(), candidate_indices.end(), Random::engine());

        int chosen_idx = candidate_indices.front();

        if (tiles && network) {
            for (int idx : candidate_indices) {
                auto [dx, dy] = dirs[idx];
                float test_x = transform.x + dx * (speed * 0.2f);
                float test_y = transform.y + dy * (speed * 0.2f);
                if (!WorldCollision::is_solid_ground(ground_circle(test_x, test_y), *tiles, *network)) {
                    chosen_idx = idx;
                    break;
                }
            }
        }

        auto [dx, dy] = dirs[chosen_idx];
        move_dx = dx;
        move_dy = dy;
        is_moving = true;
        state_timer = Random::get_float(MIN_MOVE_TIME, MAX_MOVE_TIME);
    }
}

} // namespace alx
