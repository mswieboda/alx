#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <random>
#include <vector>
#include "alx/Enemy.h"
#include "alx/Tiles.h"
#include "alx/Camera.h"
#include "alx/Player.h"
#include "alx/AlloyItem.h"
#include "core/Draw.h"

namespace alx {

namespace SpawnerConstants {
    constexpr int DEFAULT_SPAWN_COUNT = 6;
    constexpr int MIN_BORDER_OFFSET = 1;
    constexpr int MAX_BORDER_OFFSET = 3;
    constexpr float MIN_PLAYER_DISTANCE = 96.0f;
}

struct ThreatIndicatorConstants {
    static constexpr int INDICATOR_SIZE = 8;
    static constexpr int MARGIN = 4;
    static constexpr uint32_t COLOR = 0xFFFF0000; // Bright Red
    static constexpr int Z_INDEX = 101;
    static constexpr float SCAN_INTERVAL_SEC = 0.5f;
};

class EnemyManager {
private:
    std::vector<Enemy> m_enemies;
    std::vector<AlloyItem> m_alloy_items;
    float m_scan_timer = 0.0f;
    std::vector<size_t> m_cached_offscreen_indices;
    bool m_attack_hit_registered = false;
    std::mt19937 m_rng{1337};

public:
    void clear() {
        m_enemies.clear();
        m_alloy_items.clear();
        m_cached_offscreen_indices.clear();
        m_scan_timer = 0.0f;
        m_attack_hit_registered = false;
    }

    void spawn_random_enemies(const Tiles& tiles, int count = SpawnerConstants::DEFAULT_SPAWN_COUNT, float player_start_x = -1.0f, float player_start_y = -1.0f) {
        clear();

        int grid_w = tiles.width();
        int grid_h = tiles.height();
        int tile_size = tiles.tile_size();

        std::vector<std::pair<int, int>> candidate_tiles;

        for (int ty = 0; ty < grid_h; ++ty) {
            for (int tx = 0; tx < grid_w; ++tx) {
                if (!tiles.is_floor(tx, ty)) {
                    continue;
                }

                int dist_west = tx - 1;
                int dist_east = (grid_w - 2) - tx;
                int dist_north = ty - 1;
                int dist_south = (grid_h - 2) - ty;

                int min_dist = std::min({dist_west, dist_east, dist_north, dist_south});

                if (min_dist >= SpawnerConstants::MIN_BORDER_OFFSET && min_dist <= SpawnerConstants::MAX_BORDER_OFFSET) {
                    float world_x = static_cast<float>(tx * tile_size);
                    float world_y = static_cast<float>(ty * tile_size);

                    if (player_start_x >= 0.0f && player_start_y >= 0.0f) {
                        float dx = world_x - player_start_x;
                        float dy = world_y - player_start_y;
                        if ((dx * dx + dy * dy) < static_cast<float>((tile_size * 3) * (tile_size * 3))) {
                            continue;
                        }
                    }

                    candidate_tiles.push_back({tx, ty});
                }
            }
        }

        if (candidate_tiles.empty()) return;

        std::shuffle(candidate_tiles.begin(), candidate_tiles.end(), m_rng);

        int spawn_num = std::min(count, static_cast<int>(candidate_tiles.size()));
        for (int i = 0; i < spawn_num; ++i) {
            auto [tx, ty] = candidate_tiles[i];
            float world_x = static_cast<float>(tx * tile_size + (tile_size - EnemyConstants::DEFAULT_WIDTH) / 2.0f);
            float world_y = static_cast<float>(ty * tile_size + (tile_size - EnemyConstants::DEFAULT_HEIGHT) / 2.0f);
            Enemy& enemy = m_enemies.emplace_back(world_x, world_y);
            enemy.state = EnemyState::SpawnWander;
            enemy.state_timer = EnemyConstants::SPAWN_WANDER_DURATION;
            enemy.pick_random_wander_state(m_rng);
        }

        update_threat_cache();
    }

    void update(float dt, Player* player, const Tiles& tiles, const Network& network) {
        m_scan_timer += dt;
        if (m_scan_timer >= ThreatIndicatorConstants::SCAN_INTERVAL_SEC) {
            m_scan_timer = 0.0f;
            update_threat_cache();
        }

        update_enemy_ai(dt, tiles, network);
        update_enemy_push_separation();

        if (player) {
            update_combat_and_loot(*player);
        }
    }

    static bool is_solid_ground(const Collision::Circle& ground, const Tiles& tiles, const Network& network) {
        float tile_size = static_cast<float>(tiles.tile_size());

        int min_tx = static_cast<int>(std::floor((ground.cx - ground.radius) / tile_size));
        int max_tx = static_cast<int>(std::floor((ground.cx + ground.radius) / tile_size));
        int min_ty = static_cast<int>(std::floor((ground.cy - ground.radius) / tile_size));
        int max_ty = static_cast<int>(std::floor((ground.cy + ground.radius) / tile_size));

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

    static GridPos find_priority_target(float enemy_center_x, float enemy_center_y, const Network& network) {
        float tile_size = static_cast<float>(network.tile_size());

        std::vector<GridPos> tier1_candidates; // Refiners & Spires
        std::vector<GridPos> tier2_candidates; // Pipes

        for (int32_t idx : network.active_indices()) {
            int tx = idx % network.width();
            int ty = idx / network.width();
            const Fixture& fix = network.fixture(tx, ty);

            if (fix.type == FixtureType::Refiner || fix.type == FixtureType::Spire) {
                tier1_candidates.push_back(GridPos{ static_cast<int16_t>(tx), static_cast<int16_t>(ty) });
            } else if (fix.type == FixtureType::Pipe) {
                tier2_candidates.push_back(GridPos{ static_cast<int16_t>(tx), static_cast<int16_t>(ty) });
            }
        }

        const auto& candidates = !tier1_candidates.empty() ? tier1_candidates : tier2_candidates;
        if (candidates.empty()) {
            return GridPos{ -1, -1 };
        }

        GridPos best_pos{ -1, -1 };
        float min_dist_sq = 1e18f;

        for (const auto& pos : candidates) {
            Collision::AABB fix_aabb = fixture_ground_aabb(pos.x, pos.y, tile_size);
            float target_cx = fix_aabb.x + fix_aabb.w * 0.5f;
            float target_cy = fix_aabb.y + fix_aabb.h * 0.5f;
            float dx = target_cx - enemy_center_x;
            float dy = target_cy - enemy_center_y;
            float dist_sq = dx * dx + dy * dy;

            if (dist_sq < min_dist_sq) {
                min_dist_sq = dist_sq;
                best_pos = pos;
            }
        }

        return best_pos;
    }

    void update_enemy_ai(float dt, const Tiles& tiles, const Network& network) {
        float tile_size = static_cast<float>(tiles.tile_size());

        for (auto& enemy : m_enemies) {
            enemy.sync_prev_transforms();

            if (enemy.state_timer > 0.0f) {
                enemy.state_timer -= dt;
            }

            switch (enemy.state) {
                case EnemyState::SpawnWander:
                case EnemyState::RestlessWander:
                case EnemyState::DetourWander: {
                    if (enemy.state_timer <= 0.0f) {
                        GridPos target = find_priority_target(enemy.center_x(), enemy.center_y(), network);
                        if (target.x >= 0 && target.y >= 0) {
                            enemy.state = EnemyState::SeekTarget;
                            enemy.target_fixture_pos = target;
                            enemy.has_target = true;
                            enemy.state_timer = EnemyConstants::SIEGE_MARCH_DURATION;

                            std::uniform_real_distribution<float> reeval_dist(EnemyConstants::TARGET_REEVAL_MIN_TIME, EnemyConstants::TARGET_REEVAL_MAX_TIME);
                            enemy.reeval_timer = reeval_dist(m_rng);
                            enemy.stuck_timer = 0.0f;
                        } else {
                            enemy.state = EnemyState::RestlessWander;
                            enemy.pick_random_wander_state(m_rng);
                            enemy.state_timer = EnemyConstants::RESTLESS_WANDER_DURATION;
                            enemy.has_target = false;
                        }
                    } else if (!enemy.is_moving) {
                        enemy.pick_random_wander_state(m_rng);
                    }
                    break;
                }

                case EnemyState::SeekTarget: {
                    enemy.reeval_timer -= dt;

                    bool target_valid = false;
                    if (enemy.has_target && network.in_bounds(enemy.target_fixture_pos)) {
                        const Fixture& fix = network.fixture(enemy.target_fixture_pos);
                        target_valid = (!fix.is_empty() && fix.type != FixtureType::Seep);
                    }

                    if (!target_valid || enemy.reeval_timer <= 0.0f) {
                        GridPos new_target = find_priority_target(enemy.center_x(), enemy.center_y(), network);
                        if (new_target.x >= 0 && new_target.y >= 0) {
                            enemy.target_fixture_pos = new_target;
                            enemy.has_target = true;
                            std::uniform_real_distribution<float> reeval_dist(EnemyConstants::TARGET_REEVAL_MIN_TIME, EnemyConstants::TARGET_REEVAL_MAX_TIME);
                            enemy.reeval_timer = reeval_dist(m_rng);
                        } else if (!target_valid) {
                            enemy.state = EnemyState::SpawnWander;
                            enemy.state_timer = EnemyConstants::POST_DESTROY_WANDER_TIME;
                            enemy.pick_random_wander_state(m_rng);
                            enemy.has_target = false;
                            break;
                        }
                    }

                    if (enemy.state_timer <= 0.0f) {
                        enemy.state = EnemyState::RestlessWander;
                        enemy.state_timer = EnemyConstants::RESTLESS_WANDER_DURATION;
                        enemy.pick_random_wander_state(m_rng);
                        break;
                    }

                    if (enemy.has_target) {
                        Collision::AABB fix_aabb = fixture_ground_aabb(enemy.target_fixture_pos.x, enemy.target_fixture_pos.y, tile_size);
                        float target_cx = fix_aabb.x + fix_aabb.w * 0.5f;
                        float target_cy = fix_aabb.y + fix_aabb.h * 0.5f;

                        if (Collision::circle_vs_aabb(enemy.ground_circle(), fix_aabb)) {
                            enemy.is_moving = false;
                            enemy.move_dx = 0.0f;
                            enemy.move_dy = 0.0f;
                            enemy.stuck_timer = 0.0f;
                            break;
                        }

                        enemy.set_steering_vector_8way(target_cx, target_cy);
                    }
                    break;
                }

                case EnemyState::HitStun: {
                    if (enemy.state_timer <= 0.0f) {
                        enemy.state = EnemyState::SpawnWander;
                        enemy.state_timer = 0.0f;
                    }
                    break;
                }
            }

            if (enemy.is_moving && enemy.state != EnemyState::HitStun) {
                bool blocked_x = false;
                bool blocked_y = false;

                if (enemy.move_dx != 0.0f) {
                    float target_x = enemy.transform.x + enemy.move_dx * enemy.speed * dt;
                    if (!is_solid_ground(enemy.ground_circle(target_x, enemy.transform.y), tiles, network)) {
                        enemy.transform.x = target_x;
                    } else {
                        blocked_x = true;
                    }
                }

                if (enemy.move_dy != 0.0f) {
                    float target_y = enemy.transform.y + enemy.move_dy * enemy.speed * dt;
                    if (!is_solid_ground(enemy.ground_circle(enemy.transform.x, target_y), tiles, network)) {
                        enemy.transform.y = target_y;
                    } else {
                        blocked_y = true;
                    }
                }

                if (blocked_x || blocked_y) {
                    if (enemy.state == EnemyState::SeekTarget) {
                        enemy.stuck_timer += dt;
                        if (enemy.stuck_timer >= EnemyConstants::OBSTACLE_STUCK_THRESHOLD) {
                            enemy.stuck_timer = 0.0f;
                            enemy.state = EnemyState::DetourWander;
                            enemy.state_timer = EnemyConstants::DETOUR_WANDER_DURATION;
                            enemy.pick_random_wander_state(m_rng);
                        }
                    } else {
                        enemy.is_moving = false;
                        enemy.move_dx = 0.0f;
                        enemy.move_dy = 0.0f;
                        enemy.state_timer = 0.0f;
                    }
                } else {
                    enemy.stuck_timer = 0.0f;
                }
            }
        }
    }

    void update_enemy_push_separation() {
        for (size_t i = 0; i < m_enemies.size(); ++i) {
            for (size_t j = i + 1; j < m_enemies.size(); ++j) {
                Collision::Circle c1 = m_enemies[i].ground_circle();
                Collision::Circle c2 = m_enemies[j].ground_circle();
                float push_x1 = 0.0f, push_y1 = 0.0f, push_x2 = 0.0f, push_y2 = 0.0f;
                if (Collision::resolve_soft_circle_overlap(c1.cx, c1.cy, c1.radius, c2.cx, c2.cy, c2.radius, push_x1, push_y1, push_x2, push_y2)) {
                    m_enemies[i].transform.x += push_x1;
                    m_enemies[i].transform.y += push_y1;
                    m_enemies[j].transform.x += push_x2;
                    m_enemies[j].transform.y += push_y2;
                }
            }
        }
    }

    void update_combat_and_loot(Player& player) {
        // --- 1. MELEE ATTACK SWIPE PROCESSOR ---
        if (player.is_attacking()) {
            if (!m_attack_hit_registered) {
                m_attack_hit_registered = true;
                Collision::Circle hit_c = player.attack_hit_circle();

                for (auto& enemy : m_enemies) {
                    if (Collision::circle_vs_circle(enemy.hurt_circle(), hit_c)) {
                        float push_dx = enemy.center_x() - player.center_x();
                        float push_dy = enemy.center_y() - player.center_y();
                        float len_sq = push_dx * push_dx + push_dy * push_dy;
                        if (len_sq > 0.0001f) {
                            float inv_len = 1.0f / std::sqrt(len_sq);
                            push_dx *= inv_len;
                            push_dy *= inv_len;
                        } else {
                            push_dx = player.facing_dx;
                            push_dy = player.facing_dy;
                        }
                        enemy.take_damage(1, push_dx, push_dy);
                    }
                }
            }
        } else {
            m_attack_hit_registered = false;
        }

        // --- 2. ENEMY DEATH & LOOT DROP ---
        bool removed_any = false;
        for (auto it = m_enemies.begin(); it != m_enemies.end(); ) {
            if (it->is_dead()) {
                m_alloy_items.emplace_back(it->center_x() - 4.0f, it->center_y() - 4.0f);
                it = m_enemies.erase(it);
                removed_any = true;
            } else {
                ++it;
            }
        }

        if (removed_any) {
            update_threat_cache();
        }

        // --- 3. ALLOY ITEM WALK-OVER COLLECTION ---
        float px = player.transform.x;
        float py = player.transform.y;
        float pw = player.transform.width;
        float ph = player.transform.height;

        for (auto it = m_alloy_items.begin(); it != m_alloy_items.end(); ) {
            if (it->active) {
                bool collected = (px < it->x + it->width &&
                                  px + pw > it->x &&
                                  py < it->y + it->height &&
                                  py + ph > it->y);
                if (collected) {
                    player.add_cursed_alloy(1);
                    it = m_alloy_items.erase(it);
                    continue;
                }
            }
            ++it;
        }
    }

    void draw_enemies(std::vector<uint32_t>& pixel_buffer, float alpha) const {
        for (const auto& item : m_alloy_items) {
            item.draw(pixel_buffer, alpha);
        }

        for (const auto& enemy : m_enemies) {
            enemy.draw(pixel_buffer, alpha);
        }
    }

    void draw_threat_indicators(const alx::Camera& camera) const {
        float sw = static_cast<float>(Game::WIDTH);
        float sh = static_cast<float>(Game::HEIGHT);

        float csx = sw / 2.0f;
        float csy = sh / 2.0f;

        float margin_x_min = static_cast<float>(ThreatIndicatorConstants::MARGIN);
        float margin_x_max = sw - ThreatIndicatorConstants::MARGIN - ThreatIndicatorConstants::INDICATOR_SIZE;
        float margin_y_min = static_cast<float>(ThreatIndicatorConstants::MARGIN);
        float margin_y_max = sh - ThreatIndicatorConstants::MARGIN - ThreatIndicatorConstants::INDICATOR_SIZE;

        for (const auto& enemy : m_enemies) {
            float enemy_cx = enemy.center_x();
            float enemy_cy = enemy.center_y();

            float esx = static_cast<float>(camera.to_screen_x(enemy_cx));
            float esy = static_cast<float>(camera.to_screen_y(enemy_cy));

            bool off_screen = (esx < 0.0f || esx > sw || esy < 0.0f || esy > sh);

            if (!off_screen) {
                continue;
            }

            float dx = esx - csx;
            float dy = esy - csy;

            if (dx == 0.0f && dy == 0.0f) {
                continue;
            }

            float t_min = 1e9f;

            if (dx < 0.0f) {
                float t = (margin_x_min - csx) / dx;
                if (t > 0.0f) t_min = std::min(t_min, t);
            } else if (dx > 0.0f) {
                float t = (margin_x_max - csx) / dx;
                if (t > 0.0f) t_min = std::min(t_min, t);
            }

            if (dy < 0.0f) {
                float t = (margin_y_min - csy) / dy;
                if (t > 0.0f) t_min = std::min(t_min, t);
            } else if (dy > 0.0f) {
                float t = (margin_y_max - csy) / dy;
                if (t > 0.0f) t_min = std::min(t_min, t);
            }

            if (t_min > 0.0f && t_min < 1e8f) {
                float ix = std::clamp(csx + t_min * dx, margin_x_min, margin_x_max);
                float iy = std::clamp(csy + t_min * dy, margin_y_min, margin_y_max);

                Draw::rect(
                    static_cast<int>(std::round(ix)),
                    static_cast<int>(std::round(iy)),
                    ThreatIndicatorConstants::INDICATOR_SIZE,
                    ThreatIndicatorConstants::INDICATOR_SIZE,
                    ThreatIndicatorConstants::COLOR,
                    true, // fill
                    1,    // thickness
                    ThreatIndicatorConstants::Z_INDEX
                );
            }
        }
    }

    const std::vector<Enemy>& enemies() const { return m_enemies; }

private:
    void update_threat_cache() {
        m_cached_offscreen_indices.clear();
        for (size_t i = 0; i < m_enemies.size(); ++i) {
            m_cached_offscreen_indices.push_back(i);
        }
    }
};

} // namespace alx
