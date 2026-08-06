#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <random>
#include <vector>
#include "alx/Enemy.h"
#include "alx/EnemyMovement.h"
#include "alx/Tiles.h"
#include "alx/Camera.h"
#include "alx/Player.h"
#include "alx/AlloyItem.h"
#include "alx/WorldCollision.h"
#include "alx/ParticleEmitters.h"
#include "alx/Layer.h"
#include "alx/WorldStructure.h"
#include "core/Draw.h"

namespace alx {

namespace SpawnerConstants {

}

struct CachedThreatPos {
    float world_x = 0.0f;
    float world_y = 0.0f;
};

class EnemyManager {
private:

    // Spawning
    static constexpr int SPAWN_WAVE_MIN = 2;
    static constexpr int SPAWN_WAVE_MAX = 4;
    static constexpr int SPAWN_MIN_BORDER_OFFSET = 1;
    static constexpr int SPAWN_MAX_BORDER_OFFSET = 3;
    static constexpr int MAX_ACTIVE_ENEMIES = 20;
    static constexpr float SPAWN_MIN_PLAYER_DISTANCE = 128.0f; // Clearance distance from player in pixels (8 tiles @ 16px)
    static constexpr int SPAWN_CLUSTER_SEARCH_RADIUS = 3;       // Max tile radius around origin for grouped wave spawn
    static constexpr float SPAWN_TILE_OFFSET = 4.0f;     // Sub-tile random tile offset (in pixels)

    // Target Priority weighted values
    static constexpr int TARGET_PRIO_BASE = 50;
    static constexpr int TARGET_PRIO_HIGH = 200;
    static constexpr int TARGET_PRIO_BONUS_PIPE_DARK_MANA = 100;
    static constexpr int TARGET_PRIO_BONUS_PIPE_LIGHT_MANA = 25;

    // Threat Indicator
    static constexpr int INDICATOR_SIZE = 6;
    static constexpr int INDICATOR_MARGIN = 4;
    static constexpr uint32_t INDICATOR_COLOR = 0x33AA0000; // Dim Red
    static constexpr int INDICATOR_Z_INDEX = 101;
    static constexpr float INDICATOR_SCAN_INTERVAL_MIN = 2.0f;
    static constexpr float INDICATOR_SCAN_INTERVAL_MAX = 3.0f;
    static constexpr float INDICATOR_FADE_DURATION_SEC = 1.0f;
    static constexpr float OFFSCREEN_PADDING = 16.0f; // 1 tile inward screen margin

    std::vector<Enemy> m_enemies;
    std::vector<AlloyItem> m_alloy_items;
    std::vector<WorldStructure> m_world_structures;
    float m_scan_timer = 0.0f;
    float m_next_scan_interval = 2.0f;
    float m_scan_age = 999.0f; // Prevent initial rendering until scan/spawn
    std::vector<CachedThreatPos> m_cached_threat_positions;
    bool m_attack_hit_registered = false;
    float m_pending_twilight_increase = 0.0f;


public:
    void clear() {
        m_enemies.clear();
        m_alloy_items.clear();
        m_world_structures.clear();
        m_cached_threat_positions.clear();
        m_scan_timer = 0.0f;
        m_scan_age = 999.0f;
        m_next_scan_interval = 2.0f;
        m_attack_hit_registered = false;
        m_pending_twilight_increase = 0.0f;
    }

    void spawn_dark_tower(float x, float y) {
        m_world_structures.emplace_back(x, y, StructureType::DarkTower);
    }

    std::vector<WorldStructure>& structures() { return m_world_structures; }
    const std::vector<WorldStructure>& structures() const { return m_world_structures; }

    float consume_pending_twilight_increase() {
        float val = m_pending_twilight_increase;
        m_pending_twilight_increase = 0.0f;
        return val;
    }

    void spawn_enemy_wave(const Tiles& tiles, const Network* network = nullptr, int count = -1, float player_start_x = -1.0f, float player_start_y = -1.0f, bool clear_existing = false) {
        if (clear_existing) {
            clear();
        }

        int current_count = static_cast<int>(m_enemies.size());
        if (current_count >= MAX_ACTIVE_ENEMIES) {
            return;
        }

        int spawn_num = (count > 0) ? count : Random::get_int(SPAWN_WAVE_MIN, SPAWN_WAVE_MAX);

        int grid_w = tiles.width();
        int grid_h = tiles.height();
        int tile_size = tiles.tile_size();

        std::vector<std::pair<int, int>> candidate_border_tiles;

        for (int ty = 0; ty < grid_h; ++ty) {
            for (int tx = 0; tx < grid_w; ++tx) {
                if (!tiles.is_floor(tx, ty)) {
                    continue;
                }

                int dist_west = tx;
                int dist_east = (grid_w - 1) - tx;
                int dist_north = ty;
                int dist_south = (grid_h - 1) - ty;

                int min_dist = std::min({dist_west, dist_east, dist_north, dist_south});

                if (min_dist >= SPAWN_MIN_BORDER_OFFSET && min_dist <= SPAWN_MAX_BORDER_OFFSET) {
                    float world_x = static_cast<float>(tx * tile_size + tile_size / 2);
                    float world_y = static_cast<float>(ty * tile_size + tile_size / 2);

                    if (player_start_x >= 0.0f && player_start_y >= 0.0f) {
                        float dx = world_x - player_start_x;
                        float dy = world_y - player_start_y;
                        if ((dx * dx + dy * dy) < (SPAWN_MIN_PLAYER_DISTANCE * SPAWN_MIN_PLAYER_DISTANCE)) {
                            continue;
                        }
                    }

                    candidate_border_tiles.push_back({tx, ty});
                }
            }
        }

        if (candidate_border_tiles.empty()) return;

        std::shuffle(candidate_border_tiles.begin(), candidate_border_tiles.end(), Random::engine());
        auto [origin_tx, origin_ty] = candidate_border_tiles.front();

        struct ClusteredTile {
            int tx;
            int ty;
            int dist_sq;
        };

        std::vector<ClusteredTile> local_cluster;
        int rad = SPAWN_CLUSTER_SEARCH_RADIUS;

        for (int dy = -rad; dy <= rad; ++dy) {
            for (int dx = -rad; dx <= rad; ++dx) {
                int tx = origin_tx + dx;
                int ty = origin_ty + dy;
                if (tx >= 0 && tx < grid_w && ty >= 0 && ty < grid_h && tiles.is_floor(tx, ty)) {
                    local_cluster.push_back({tx, ty, dx * dx + dy * dy});
                }
            }
        }

        if (static_cast<int>(local_cluster.size()) < spawn_num) {
            local_cluster.clear();
            for (int ty = 0; ty < grid_h; ++ty) {
                for (int tx = 0; tx < grid_w; ++tx) {
                    if (tiles.is_floor(tx, ty)) {
                        int dx = tx - origin_tx;
                        int dy = ty - origin_ty;
                        local_cluster.push_back({tx, ty, dx * dx + dy * dy});
                    }
                }
            }
        }

        std::sort(local_cluster.begin(), local_cluster.end(), [](const ClusteredTile& a, const ClusteredTile& b) {
            return a.dist_sq < b.dist_sq;
        });

        std::vector<std::pair<int, int>> selected_tiles;
        size_t start_idx = 0;
        while (start_idx < local_cluster.size() && static_cast<int>(selected_tiles.size()) < spawn_num) {
            size_t end_idx = start_idx;
            while (end_idx < local_cluster.size() && local_cluster[end_idx].dist_sq == local_cluster[start_idx].dist_sq) {
                end_idx++;
            }
            std::vector<std::pair<int, int>> tier;
            for (size_t k = start_idx; k < end_idx; ++k) {
                tier.push_back({local_cluster[k].tx, local_cluster[k].ty});
            }
            std::shuffle(tier.begin(), tier.end(), Random::engine());
            for (const auto& t : tier) {
                selected_tiles.push_back(t);
                if (static_cast<int>(selected_tiles.size()) == spawn_num) break;
            }
            start_idx = end_idx;
        }

        for (const auto& [tx, ty] : selected_tiles) {
            float base_x = static_cast<float>(tx * tile_size + (tile_size - Enemy::DEFAULT_WIDTH) / 2.0f);
            float base_y = static_cast<float>(ty * tile_size + (tile_size - Enemy::DEFAULT_HEIGHT) / 2.0f);

            float offset_x = Random::get_float(-SPAWN_TILE_OFFSET, SPAWN_TILE_OFFSET);
            float offset_y = Random::get_float(-SPAWN_TILE_OFFSET, SPAWN_TILE_OFFSET);

            float final_x = base_x + offset_x;
            float final_y = base_y + offset_y;

            Enemy temp_enemy(final_x, final_y);
            if (network && is_solid_ground(temp_enemy.ground_circle(), tiles, *network)) {
                final_x = base_x;
                final_y = base_y;
            }

            Enemy& enemy = m_enemies.emplace_back(final_x, final_y);
            enemy.state = EnemyState::Wander;
            enemy.state_timer = Random::get_float(4.0f, 6.0f);
            EnemyMovement::reset_wander_state(enemy.move_state);
        }

        update_threat_cache();
        m_scan_timer = 0.0f;
        m_scan_age = 0.0f;
        m_next_scan_interval = Random::get_float(INDICATOR_SCAN_INTERVAL_MIN, INDICATOR_SCAN_INTERVAL_MAX);
    }

    void update(float dt, Player* player, const Tiles& tiles, Network& network, ParticleSystem* particles = nullptr) {
        m_scan_timer += dt;
        m_scan_age += dt;
        if (m_scan_timer >= m_next_scan_interval) {
            m_scan_timer = 0.0f;
            m_scan_age = 0.0f;
            update_threat_cache();
            m_next_scan_interval = Random::get_float(INDICATOR_SCAN_INTERVAL_MIN, INDICATOR_SCAN_INTERVAL_MAX);
        }

        for (auto& struct_obj : m_world_structures) {
            struct_obj.update(dt);
        }

        update_enemy_ai(dt, player, tiles, network, particles);
        update_enemy_push_separation(tiles, network);

        if (player) {
            update_combat_and_loot(*player, particles);
        }
    }

    bool is_solid_ground(const Collision::Circle& ground, const Tiles& tiles, const Network& network) const {
        return WorldCollision::is_solid_ground(ground, tiles, network, &m_world_structures);
    }

    // static void enforce_solid_ground_ejection(Enemy& enemy, const Tiles& tiles, const Network& network) {
    //     WorldCollision::enforce_solid_ground_ejection(enemy.transform.x, enemy.transform.y, enemy.ground_circle(), tiles, network, 2.0f, enemy.tag);
    // }

    GridPos find_priority_target(const Enemy& enemy, const Network& network) const {
        float tile_size = static_cast<float>(network.tile_size());
        float enemy_cx = enemy.center_x();
        float enemy_cy = enemy.center_y();

        GridPos best_pos{ -1, -1 };
        float max_score = -1e9f;

        for (int32_t idx : network.active_indices()) {
            int tx = idx % network.width();
            int ty = idx / network.width();
            const Fixture& fixture = network.fixture(tx, ty);
            if (fixture.type == FixtureType::None || fixture.type == FixtureType::Seep) {
                continue;
            }

            GridPos pos{ static_cast<int16_t>(tx), static_cast<int16_t>(ty) };
            float fcx = pos.x * tile_size + tile_size * 0.5f;
            float fcy = pos.y * tile_size + tile_size * 0.5f;

            float dist = std::sqrt((fcx - enemy_cx) * (fcx - enemy_cx) + (fcy - enemy_cy) * (fcy - enemy_cy));
            dist = std::max(dist, 1.0f);

            float base_value = (fixture.type == FixtureType::Pipe) ? 100.0f : 300.0f;
            float score = base_value / dist;

            if (fixture.type == FixtureType::Pipe && fixture.mana_state == ManaState::Dark) {
                score += 50.0f;
            }

            int crowd_count = 0;
            for (const auto& other : m_enemies) {
                if (other.active && other.has_target && other.target_fixture_pos == pos) {
                    crowd_count++;
                }
            }
            score -= (crowd_count * 80.0f);

            if (score > max_score) {
                max_score = score;
                best_pos = pos;
            }
        }

        return best_pos;
    }

    void update_enemy_ai(float dt, Player* player, const Tiles& tiles, Network& network, ParticleSystem* particles = nullptr) {
        float tile_size = static_cast<float>(tiles.tile_size());

        for (auto& enemy : m_enemies) {
            // NOTE: WorldCollision::try_move() prevents geometry penetration during normal gameplay.
            // Enable ejection safety net if adding heavy knockback, teleports, or phase-dashes.
            // enforce_solid_ground_ejection(enemy, tiles, network);
            enemy.sync_prev_transforms();

            if (enemy.state_timer > 0.0f) {
                enemy.state_timer -= dt;
            }

            // --- Player Aggro Interception & Dynamic Retargeting [EPAT] [ERET] ---
            if (player != nullptr && enemy.state != EnemyState::HitStun && enemy.state != EnemyState::AttackWindup && enemy.state != EnemyState::AttackRecoilRest) {
                float px = player->center_x();
                float py = player->center_y();
                float edx = px - enemy.center_x();
                float edy = py - enemy.center_y();
                float dist_sq = edx * edx + edy * edy;

                constexpr float aggro_r_sq = Enemy::AGGRO_DETECTION_RADIUS * Enemy::AGGRO_DETECTION_RADIUS;
                constexpr float leash_r = Enemy::AGGRO_DETECTION_RADIUS * 1.5f;
                constexpr float leash_r_sq = leash_r * leash_r;

                if (dist_sq <= aggro_r_sq) {
                    enemy.state = EnemyState::ChasePlayer;
                    enemy.set_steering_vector_8way(px, py);
                } else if (enemy.state == EnemyState::ChasePlayer && dist_sq > leash_r_sq) {
                    enemy.state = EnemyState::RestlessWander;
                    enemy.state_timer = Enemy::RESTLESS_WANDER_DURATION;
                    EnemyMovement::reset_wander_state(enemy.move_state);
                    enemy.has_target = false;
                }
            }


            switch (enemy.state) {
                case EnemyState::Wander:
                case EnemyState::RestlessWander:
                case EnemyState::DetourWander: {
                    if (enemy.state_timer <= 0.0f) {
                        GridPos target = find_priority_target(enemy, network);
                        if (target.x >= 0 && target.y >= 0) {
                            enemy.state = EnemyState::SeekTarget;
                            enemy.target_fixture_pos = target;
                            enemy.has_target = true;
                            enemy.state_timer = Enemy::SIEGE_MARCH_DURATION;

                            enemy.reeval_timer = Random::get_float(Enemy::TARGET_REEVAL_MIN_TIME, Enemy::TARGET_REEVAL_MAX_TIME);
                            enemy.stuck_timer = 0.0f;
                            EnemyMovement::reset_wander_state(enemy.move_state);
                        } else {
                            enemy.state = EnemyState::RestlessWander;
                            enemy.state_timer = Enemy::RESTLESS_WANDER_DURATION;
                            enemy.has_target = false;
                            EnemyMovement::update_wander_step(enemy, enemy.move_state, dt, tiles, network, {}, &m_world_structures);
                        }
                    } else {
                        EnemyMovement::update_wander_step(enemy, enemy.move_state, dt, tiles, network, {}, &m_world_structures);
                    }
                    break;
                }

                case EnemyState::SeekTarget: {
                    enemy.reeval_timer -= dt;

                    // Expiration of SIEGE_MARCH_DURATION -> intermission micro-wander
                    if (enemy.state_timer <= 0.0f) {
                        enemy.state = EnemyState::Wander;
                        enemy.state_timer = Enemy::MARCH_INTERMISSION_WANDER_TIME;
                        EnemyMovement::reset_wander_state(enemy.move_state);
                        enemy.has_target = false;
                        break;
                    }

                    bool target_valid = false;
                    if (enemy.has_target && network.in_bounds(enemy.target_fixture_pos)) {
                        const Fixture& fix = network.fixture(enemy.target_fixture_pos);
                        target_valid = (!fix.is_empty() && fix.type != FixtureType::Seep);
                    }

                    if (!target_valid || enemy.reeval_timer <= 0.0f) {
                        GridPos new_target = find_priority_target(enemy, network);
                        if (new_target.x >= 0 && new_target.y >= 0) {
                            enemy.target_fixture_pos = new_target;
                            enemy.has_target = true;
                            enemy.reeval_timer = Random::get_float(Enemy::TARGET_REEVAL_MIN_TIME, Enemy::TARGET_REEVAL_MAX_TIME);
                        } else if (!target_valid) {
                            enemy.state = EnemyState::Wander;
                            enemy.state_timer = Enemy::POST_DESTROY_WANDER_TIME;
                            EnemyMovement::reset_wander_state(enemy.move_state);
                            enemy.has_target = false;
                            break;
                        }
                    }

                    if (enemy.has_target) {
                        Collision::AABB fix_aabb = fixture_ground_aabb(enemy.target_fixture_pos.x, enemy.target_fixture_pos.y, tile_size, network.fixture(enemy.target_fixture_pos.x, enemy.target_fixture_pos.y).type);
                        float target_cx = fix_aabb.x + fix_aabb.w * 0.5f;
                        float target_cy = fix_aabb.y + fix_aabb.h * 0.5f;

                        // Fix 3: Outer Reach Padding (+2.0px) so mobs trigger attack before penetrating solid AABB
                        Collision::AABB padded_aabb{ fix_aabb.x - 2.0f, fix_aabb.y - 2.0f, fix_aabb.w + 4.0f, fix_aabb.h + 4.0f };
                        if (Collision::circle_vs_aabb(enemy.ground_circle(), padded_aabb)) {
                            enemy.state = EnemyState::AttackWindup;
                            enemy.state_timer = Enemy::ATTACK_WINDUP_TIME;
                            enemy.is_moving = false;
                            enemy.move_dx = 0.0f;
                            enemy.move_dy = 0.0f;
                            enemy.stuck_timer = 0.0f;
                            enemy.target_is_player = false;
                            break;
                        }

                        enemy.set_steering_vector_8way(target_cx, target_cy);
                    }
                    break;
                }

                case EnemyState::AttackWindup: {
                    enemy.is_moving = false;
                    enemy.stuck_timer = 0.0f; // Fix 1: Reset stuck_timer during attack windup
                    if (enemy.state_timer <= 0.0f) {
                        if (enemy.target_is_player) {
                            float target_cx = enemy.center_x();
                            float target_cy = enemy.center_y();

                            if (player != nullptr) {
                                target_cx = player->center_x();
                                target_cy = player->center_y();

                                Collision::Circle player_hurt = player->hurt_circle();
                                Collision::Circle enemy_ground = enemy.ground_circle();
                                float edx = player_hurt.cx - enemy_ground.cx;
                                float edy = player_hurt.cy - enemy_ground.cy;
                                float dist = std::sqrt(edx * edx + edy * edy);
                                float attack_hit_reach = enemy_ground.radius + player_hurt.radius + 8.0f;

                                if (dist <= attack_hit_reach) {
                                    player->take_damage(1);
                                }
                            }

                            float rdx = enemy.center_x() - target_cx;
                            float rdy = enemy.center_y() - target_cy;
                            float rlen = std::sqrt(rdx * rdx + rdy * rdy);
                            if (rlen > 0.001f) {
                                rdx /= rlen;
                                rdy /= rlen;
                            } else {
                                rdx = 0.0f;
                                rdy = -1.0f;
                            }

                            enemy.recoil_dx = rdx;
                            enemy.recoil_dy = rdy;
                            enemy.recoil_dist_remaining = Enemy::RECOIL_DIST;
                            enemy.target_is_player = false;

                            enemy.state = EnemyState::AttackRecoilRest;
                            enemy.state_timer = Random::get_float(Enemy::RECOVERY_REST_MIN_TIME, Enemy::RECOVERY_REST_MAX_TIME);
                            break;
                        }

                        float twilight_inc = 0.0f;
                        bool destroyed = network.damage_fixture(enemy.target_fixture_pos, 1, twilight_inc);
                        if (twilight_inc > 0.0f) {
                            m_pending_twilight_increase += twilight_inc;
                        }

                        Collision::AABB fix_aabb = fixture_ground_aabb(enemy.target_fixture_pos.x, enemy.target_fixture_pos.y, tile_size, network.fixture(enemy.target_fixture_pos.x, enemy.target_fixture_pos.y).type);
                        float target_cx = fix_aabb.x + fix_aabb.w * 0.5f;
                        float target_cy = fix_aabb.y + fix_aabb.h * 0.5f;
                        float rdx = enemy.center_x() - target_cx;
                        float rdy = enemy.center_y() - target_cy;
                        float rlen = std::sqrt(rdx * rdx + rdy * rdy);
                        if (rlen > 0.001f) {
                            rdx /= rlen;
                            rdy /= rlen;
                        } else {
                            rdx = 0.0f;
                            rdy = -1.0f;
                        }

                        enemy.recoil_dx = rdx;
                        enemy.recoil_dy = rdy;
                        enemy.recoil_dist_remaining = Enemy::RECOIL_DIST;

                        if (destroyed) {
                            enemy.state = EnemyState::Wander;
                            enemy.state_timer = Enemy::POST_DESTROY_WANDER_TIME;
                            EnemyMovement::reset_wander_state(enemy.move_state);
                            enemy.has_target = false;
                        } else {
                            enemy.state = EnemyState::AttackRecoilRest;
                            enemy.state_timer = Random::get_float(Enemy::RECOVERY_REST_MIN_TIME, Enemy::RECOVERY_REST_MAX_TIME);
                        }
                    }
                    break;
                }

                case EnemyState::AttackRecoilRest: {
                    enemy.is_moving = false;
                    enemy.stuck_timer = 0.0f; // Fix 1: Reset stuck_timer during recoil rest

                    if (enemy.recoil_dist_remaining > 0.0f) {
                        float step = std::min(enemy.recoil_dist_remaining, Enemy::RECOIL_SLIDE_SPEED * dt);
                        float target_x = enemy.transform.x + enemy.recoil_dx * step;
                        float target_y = enemy.transform.y + enemy.recoil_dy * step;

                        if (!is_solid_ground(enemy.ground_circle(target_x, target_y), tiles, network)) {
                            enemy.transform.x = target_x;
                            enemy.transform.y = target_y;
                            enemy.recoil_dist_remaining -= step;
                        } else {
                            enemy.recoil_dist_remaining = 0.0f;
                        }
                    }

                    if (enemy.state_timer <= 0.0f) {
                        if (enemy.has_target && network.in_bounds(enemy.target_fixture_pos) && !network.fixture(enemy.target_fixture_pos).is_empty()) {
                            enemy.state = EnemyState::SeekTarget;
                            enemy.state_timer = Enemy::SIEGE_MARCH_DURATION;
                        } else {
                            enemy.state = EnemyState::Wander;
                            enemy.state_timer = Enemy::POST_DESTROY_WANDER_TIME;
                            EnemyMovement::reset_wander_state(enemy.move_state);
                            enemy.has_target = false;
                        }
                    }
                    break;
                }

                case EnemyState::ChasePlayer: {
                    if (player != nullptr && !player->state.defeated) {
                        float pcx = player->center_x();
                        float pcy = player->center_y();

                        Collision::Circle player_hurt = player->hurt_circle();
                        Collision::Circle enemy_ground = enemy.ground_circle();
                        float edx = player_hurt.cx - enemy_ground.cx;
                        float edy = player_hurt.cy - enemy_ground.cy;
                        float dist = std::sqrt(edx * edx + edy * edy);
                        float attack_reach = enemy_ground.radius + player_hurt.radius + 4.0f;

                        if (dist <= attack_reach) {
                            enemy.state = EnemyState::AttackWindup;
                            enemy.state_timer = Enemy::ATTACK_WINDUP_TIME;
                            enemy.is_moving = false;
                            enemy.move_dx = 0.0f;
                            enemy.move_dy = 0.0f;
                            enemy.stuck_timer = 0.0f;
                            enemy.target_is_player = true;
                            break;
                        }

                        enemy.set_steering_vector_8way(pcx, pcy);
                    } else {
                        enemy.state = EnemyState::RestlessWander;
                        enemy.state_timer = Enemy::RESTLESS_WANDER_DURATION;
                        EnemyMovement::reset_wander_state(enemy.move_state);
                        enemy.has_target = false;
                    }
                    break;
                }

                case EnemyState::HitStun: {
                    // [MWB]: Multi-wave continuous bleed spurting from moving wound during knockback
                    if (enemy.bleed_waves_left > 0 && particles != nullptr) {
                        enemy.bleed_timer -= dt;
                        if (enemy.bleed_timer <= 0.0f) {
                            enemy.bleed_timer = 0.07f;
                            int wave_index = 3 - enemy.bleed_waves_left;
                            enemy.bleed_waves_left--;

                            float emit_x = enemy.transform.x + enemy.hit_wound_offset_x;
                            float emit_y = enemy.transform.y + enemy.hit_wound_offset_y;
                            float kb_vx = enemy.knockback_dx * enemy.knockback_speed;
                            float kb_vy = enemy.knockback_dy * enemy.knockback_speed;
                            int enemy_sort_y = static_cast<int>(enemy.transform.y + enemy.transform.height);

                            int blood_z = Layer::WorldObj + 5; // z = 15: Renders on top of player and enemy sprites (z = 10)
                            int blood_sort_y = enemy_sort_y + 20;

                            if (wave_index == 1) {
                                // Wave 2: 17 droplets mid-slide, 50% momentum
                                ParticleEmitters::spawn_hit_sparks(*particles, emit_x, emit_y, kb_vx, kb_vy, 17, blood_z, blood_sort_y, 0.50f);
                            } else {
                                // Wave 3: 13 droplets final deceleration drip, 20% momentum
                                ParticleEmitters::spawn_hit_sparks(*particles, emit_x, emit_y, kb_vx, kb_vy, 13, blood_z, blood_sort_y, 0.20f);
                            }
                        }
                    }

                    if (enemy.knockback_speed > 5.0f) {
                        float step = enemy.knockback_speed * dt;
                        bool blocked_x = false;
                        bool blocked_y = false;

                        if (enemy.knockback_dx != 0.0f) {
                            float tx = enemy.transform.x + enemy.knockback_dx * step;
                            if (!is_solid_ground(enemy.ground_circle(tx, enemy.transform.y), tiles, network)) {
                                enemy.transform.x = tx;
                            } else {
                                blocked_x = true;
                            }
                        }

                        if (enemy.knockback_dy != 0.0f) {
                            float ty = enemy.transform.y + enemy.knockback_dy * step;
                            if (!is_solid_ground(enemy.ground_circle(enemy.transform.x, ty), tiles, network)) {
                                enemy.transform.y = ty;
                            } else {
                                blocked_y = true;
                            }
                        }

                        if (blocked_x) {
                            enemy.knockback_dx = -enemy.knockback_dx * 0.5f;
                        }
                        if (blocked_y) {
                            enemy.knockback_dy = -enemy.knockback_dy * 0.5f;
                        }

                        float speed_ratio = enemy.knockback_speed / std::max(0.001f, enemy.initial_knockback_speed);
                        float friction_coeff = 1.0f - (speed_ratio * speed_ratio);
                        float deceleration = 100.0f + 1100.0f * friction_coeff;

                        enemy.knockback_speed = std::max(0.0f, enemy.knockback_speed - deceleration * dt);
                    } else {
                        enemy.knockback_speed = 0.0f;
                    }

                    if (enemy.state_timer <= 0.0f && enemy.knockback_speed <= 0.0f) {
                        enemy.state = EnemyState::Wander;
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
                        // Fix 2: If enemy is already within reach of its target fixture, ignore stuck counting
                        bool near_target = false;
                        if (enemy.has_target && network.in_bounds(enemy.target_fixture_pos)) {
                            Collision::AABB fix_aabb = fixture_ground_aabb(enemy.target_fixture_pos.x, enemy.target_fixture_pos.y, tile_size, network.fixture(enemy.target_fixture_pos.x, enemy.target_fixture_pos.y).type);
                            Collision::AABB padded_aabb{ fix_aabb.x - 4.0f, fix_aabb.y - 4.0f, fix_aabb.w + 8.0f, fix_aabb.h + 8.0f };
                            near_target = Collision::circle_vs_aabb(enemy.ground_circle(), padded_aabb);
                        }

                        if (near_target) {
                            enemy.stuck_timer = 0.0f;
                        } else {
                            enemy.stuck_timer += dt;
                            if (enemy.stuck_timer >= Enemy::OBSTACLE_STUCK_THRESHOLD) {
                                enemy.stuck_timer = 0.0f;
                                enemy.state = EnemyState::DetourWander;
                                enemy.state_timer = Enemy::DETOUR_WANDER_DURATION;
                                EnemyMovement::reset_wander_state(enemy.move_state);
                            }
                        }
                    } else if (enemy.state == EnemyState::Wander || enemy.state == EnemyState::RestlessWander || enemy.state == EnemyState::DetourWander) {
                        EnemyMovement::handle_wall_collision(enemy, enemy.move_state, tiles, network, {}, &m_world_structures);
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

    void update_enemy_push_separation(const Tiles& tiles, const Network& network) {
        for (size_t i = 0; i < m_enemies.size(); ++i) {
            for (size_t j = i + 1; j < m_enemies.size(); ++j) {
                Collision::Circle c1 = m_enemies[i].ground_circle();
                Collision::Circle c2 = m_enemies[j].ground_circle();
                float push_x1 = 0.0f, push_y1 = 0.0f, push_x2 = 0.0f, push_y2 = 0.0f;
                if (Collision::resolve_soft_circle_overlap(c1.cx, c1.cy, c1.radius, c2.cx, c2.cy, c2.radius, push_x1, push_y1, push_x2, push_y2)) {
                    bool i_can_move = !is_solid_ground(m_enemies[i].ground_circle(m_enemies[i].transform.x + push_x1, m_enemies[i].transform.y + push_y1), tiles, network);
                    bool j_can_move = !is_solid_ground(m_enemies[j].ground_circle(m_enemies[j].transform.x + push_x2, m_enemies[j].transform.y + push_y2), tiles, network);

                    if (i_can_move && j_can_move) {
                        m_enemies[i].transform.x += push_x1;
                        m_enemies[i].transform.y += push_y1;
                        m_enemies[j].transform.x += push_x2;
                        m_enemies[j].transform.y += push_y2;
                    } else if (i_can_move && !j_can_move) {
                        float full_push_x = push_x1 - push_x2;
                        float full_push_y = push_y1 - push_y2;
                        if (!is_solid_ground(m_enemies[i].ground_circle(m_enemies[i].transform.x + full_push_x, m_enemies[i].transform.y + full_push_y), tiles, network)) {
                            m_enemies[i].transform.x += full_push_x;
                            m_enemies[i].transform.y += full_push_y;
                        }
                    } else if (!i_can_move && j_can_move) {
                        float full_push_x = push_x2 - push_x1;
                        float full_push_y = push_y2 - push_y1;
                        if (!is_solid_ground(m_enemies[j].ground_circle(m_enemies[j].transform.x + full_push_x, m_enemies[j].transform.y + full_push_y), tiles, network)) {
                            m_enemies[j].transform.x += full_push_x;
                            m_enemies[j].transform.y += full_push_y;
                        }
                    }
                }
            }
        }
    }

    void update_combat_and_loot(Player& player, ParticleSystem* particles = nullptr) {
        // --- 1. MELEE ATTACK SWIPE PROCESSOR ---
        if (player.is_attacking()) {
            static constexpr int SUB_STEPS = 3;
            float prev_p = player.swing_progress_prev;
            float curr_p = player.swing_progress_curr;

            for (int s = 0; s < SUB_STEPS; ++s) {
                float sub_t = prev_p + (curr_p - prev_p) * (static_cast<float>(s + 1) / static_cast<float>(SUB_STEPS));
                Collision::Circle hit_c = player.calculate_attack_circle_at(sub_t, player.transform.x, player.transform.y);

                int attack_cx = static_cast<int>(hit_c.cx);
                int attack_cy = static_cast<int>(hit_c.cy);
                int attack_r  = static_cast<int>(hit_c.radius);

                for (auto& enemy : m_enemies) {
                    if (enemy.is_dead()) {
                        continue;
                    }
                    if (enemy.last_hit_swing_id == player.current_swing_id) {
                        continue;
                    }

                    Collision::Circle hurt_c = enemy.hurt_circle();
                    float contact_x = 0.0f, contact_y = 0.0f;

                    if (Collision::circle_contact_point(hit_c, hurt_c, contact_x, contact_y)) {
                        enemy.last_hit_swing_id = player.current_swing_id;

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
                        // Calculate wound offset relative to enemy sprite (+1px to +3px shifted toward enemy center)
                        float wound_ox = contact_x - enemy.transform.x;
                        float wound_oy = contact_y - enemy.transform.y;
                        float to_center_x = enemy.center_x() - contact_x;
                        float to_center_y = enemy.center_y() - contact_y;
                        float c_len = std::sqrt(to_center_x * to_center_x + to_center_y * to_center_y);
                        if (c_len > 0.001f) {
                            float shift_dist = Random::get_float(1.0f, 3.0f);
                            wound_ox += (to_center_x / c_len) * shift_dist;
                            wound_oy += (to_center_y / c_len) * shift_dist;
                        }

                        enemy.take_damage(1, push_dx, push_dy, Player::ATTACK_KNOCKBACK_SPEED, wound_ox, wound_oy);
                        if (particles) {
                            float kb_vx = push_dx * Player::ATTACK_KNOCKBACK_SPEED;
                            float kb_vy = push_dy * Player::ATTACK_KNOCKBACK_SPEED;
                            int enemy_sort_y = static_cast<int>(enemy.transform.y + enemy.transform.height);
                            int blood_z = Layer::WorldObj + 5; // z = 15: Renders on top of player and enemy sprites (z = 10)
                            int blood_sort_y = enemy_sort_y + 20;

                            // Wave 1: 20 droplets on hit contact, 85% momentum
                            ParticleEmitters::spawn_hit_sparks(*particles, contact_x, contact_y, kb_vx, kb_vy, 20, blood_z, blood_sort_y, 0.85f);
                        }
                    }
                }
            }
        }

        // --- 2. ENEMY DEATH & LOOT DROP ---
        bool removed_any = false;
        for (auto it = m_enemies.begin(); it != m_enemies.end(); ) {
            if (it->is_dead() && it->state != EnemyState::HitStun) {
                m_alloy_items.emplace_back(it->center_x() - 5.0f, it->center_y() - 2.0f);
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
                    if (particles) {
                        int alloy_sort_y = static_cast<int>(it->y + it->height);
                        ParticleEmitters::spawn_alloy_pickup(*particles, it->center_x(), it->center_y(), 15, Layer::WorldObj, alloy_sort_y);
                    }
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

        for (const auto& struct_obj : m_world_structures) {
            struct_obj.draw(pixel_buffer, alpha);
        }

        for (const auto& enemy : m_enemies) {
            enemy.draw(pixel_buffer, alpha);
        }
    }

    void draw_threat_indicators(const alx::Camera& camera) const {
        if (m_scan_age >= INDICATOR_FADE_DURATION_SEC) {
            return;
        }

        float sw = static_cast<float>(Game::WIDTH);
        float sh = static_cast<float>(Game::HEIGHT);

        float csx = sw / 2.0f;
        float csy = sh / 2.0f;

        float margin_x_min = static_cast<float>(INDICATOR_MARGIN);
        float margin_x_max = sw - INDICATOR_MARGIN - INDICATOR_SIZE;
        float margin_y_min = static_cast<float>(INDICATOR_MARGIN);
        float margin_y_max = sh - INDICATOR_MARGIN - INDICATOR_SIZE;

        // Ease-in & Ease-out sine bell curve pulse: 0% -> 100% -> 0% over 0.5s
        float progress = std::clamp(m_scan_age / INDICATOR_FADE_DURATION_SEC, 0.0f, 1.0f);
        float pulse_curve = std::sin(progress * 3.1415926535f);
        uint8_t base_alpha = (INDICATOR_COLOR >> 24) & 0xFF;
        uint8_t faded_alpha = static_cast<uint8_t>(base_alpha * pulse_curve);
        uint32_t faded_color = (static_cast<uint32_t>(faded_alpha) << 24) | (INDICATOR_COLOR & 0x00FFFFFF);

        for (const auto& cached : m_cached_threat_positions) {
            float esx = static_cast<float>(camera.to_screen_x(cached.world_x));
            float esy = static_cast<float>(camera.to_screen_y(cached.world_y));

            // Viewport is padded inwards by OFFSCREEN_PADDING (16px)
            // Indicators only show if enemy is past screen bounds + 16px buffer
            bool off_screen = (esx < -OFFSCREEN_PADDING || esx > sw + OFFSCREEN_PADDING ||
                               esy < -OFFSCREEN_PADDING || esy > sh + OFFSCREEN_PADDING);

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
                    INDICATOR_SIZE,
                    INDICATOR_SIZE,
                    faded_color,
                    true, // fill
                    1,    // thickness
                    INDICATOR_Z_INDEX
                );
            }
        }
    }

    const std::vector<Enemy>& enemies() const { return m_enemies; }

private:
    void update_threat_cache() {
        m_cached_threat_positions.clear();
        m_cached_threat_positions.reserve(m_enemies.size());
        for (const auto& enemy : m_enemies) {
            m_cached_threat_positions.push_back(CachedThreatPos{ enemy.center_x(), enemy.center_y() });
        }
    }
};

} // namespace alx
