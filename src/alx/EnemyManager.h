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

        int grid_w = tiles.get_width();
        int grid_h = tiles.get_height();
        int tile_size = tiles.get_tile_size();

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

        std::mt19937 rng(1337);
        std::shuffle(candidate_tiles.begin(), candidate_tiles.end(), rng);

        int spawn_num = std::min(count, static_cast<int>(candidate_tiles.size()));
        for (int i = 0; i < spawn_num; ++i) {
            auto [tx, ty] = candidate_tiles[i];
            float world_x = static_cast<float>(tx * tile_size + (tile_size - EnemyConstants::DEFAULT_WIDTH) / 2.0f);
            float world_y = static_cast<float>(ty * tile_size + (tile_size - EnemyConstants::DEFAULT_HEIGHT) / 2.0f);
            m_enemies.emplace_back(world_x, world_y);
        }

        update_threat_cache();
    }

    void update(float dt, Player* player = nullptr) {
        m_scan_timer += dt;
        if (m_scan_timer >= ThreatIndicatorConstants::SCAN_INTERVAL_SEC) {
            m_scan_timer = 0.0f;
            update_threat_cache();
        }

        update_enemy_push_separation();

        if (player) {
            update_combat_and_loot(*player);
        }
    }

    void update_enemy_push_separation() {
        for (size_t i = 0; i < m_enemies.size(); ++i) {
            for (size_t j = i + 1; j < m_enemies.size(); ++j) {
                Collision::Circle c1 = m_enemies[i].get_ground_circle();
                Collision::Circle c2 = m_enemies[j].get_ground_circle();
                float push_x1 = 0.0f, push_y1 = 0.0f, push_x2 = 0.0f, push_y2 = 0.0f;
                if (Collision::resolve_soft_circle_overlap(c1.cx, c1.cy, c1.radius, c2.cx, c2.cy, c2.radius, push_x1, push_y1, push_x2, push_y2)) {
                    m_enemies[i].x += push_x1;
                    m_enemies[i].y += push_y1;
                    m_enemies[j].x += push_x2;
                    m_enemies[j].y += push_y2;
                }
            }
        }
    }

    void update_combat_and_loot(Player& player) {
        // --- 1. MELEE ATTACK SWIPE PROCESSOR ---
        if (player.is_attacking()) {
            if (!m_attack_hit_registered) {
                m_attack_hit_registered = true;
                Collision::Circle hit_c = player.get_attack_hit_circle();

                for (auto& enemy : m_enemies) {
                    if (Collision::circle_vs_circle(enemy.get_hurt_circle(), hit_c)) {
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

    const std::vector<Enemy>& get_enemies() const { return m_enemies; }

private:
    void update_threat_cache() {
        m_cached_offscreen_indices.clear();
        for (size_t i = 0; i < m_enemies.size(); ++i) {
            m_cached_offscreen_indices.push_back(i);
        }
    }
};

} // namespace alx
