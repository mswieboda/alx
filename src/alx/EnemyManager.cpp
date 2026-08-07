#include "alx/EnemyManager.h"
#include "alx/Random.h"
#include "alx/ParticleEmitters.h"
#include <algorithm>

namespace {

[[nodiscard]] bool is_valid_corrupted_tile_footprint(int tile_x, int tile_y, const alx::Tiles& tiles) noexcept {
    constexpr int foot_w_tiles = alx::WorldStructure::DARK_TOWER_TILE_WIDTH;
    constexpr int foot_h_tiles = alx::WorldStructure::DARK_TOWER_TILE_HEIGHT;

    for (int dy = 0; dy < foot_h_tiles; ++dy) {
        for (int dx = 0; dx < foot_w_tiles; ++dx) {
            int tx = tile_x + dx;
            int ty = tile_y + dy;
            if (!tiles.in_bounds(tx, ty) || !tiles.is_floor(tx, ty)) {
                return false;
            }
        }
    }
    return true;
}

} // anonymous namespace

namespace alx {

    void EnemyManager::clear() {
        m_enemies.clear();
        m_alloy_items.clear();
        m_world_structures.clear();
        m_shadow_eggs.clear();
        m_cached_threat_positions.clear();
        for (auto& tile : m_corrupted_tiles) {
            tile.is_occupied = false;
        }
        m_scan_timer = 0.0f;
        m_scan_age = 999.0f;
        m_next_scan_interval = 2.0f;
        m_tower_emergence_timer = 0.0f;
        m_next_emergence_cooldown = DarkTowerConstants::EMERGENCE_COOLDOWN_MIN;
        m_attack_hit_registered = false;
        m_pending_twilight_increase = 0.0f;
    }

    void EnemyManager::register_corrupted_tiles(const std::vector<std::pair<int, int>>& coords, const Tiles& tiles) {
        m_corrupted_tiles.clear();
        m_corrupted_tiles.reserve(coords.size());

        for (const auto& [tx, ty] : coords) {
            if (is_valid_corrupted_tile_footprint(tx, ty, tiles)) {
                m_corrupted_tiles.push_back(CorruptedDarkTowerTile{ tx, ty, false, 0.0f });
            }
        }
    }

    int EnemyManager::find_unoccupied_corrupted_tile_index() const {
        int free_count = 0;
        int selected_idx = -1;
        for (size_t i = 0; i < m_corrupted_tiles.size(); ++i) {
            if (m_corrupted_tiles[i].is_available()) {
                ++free_count;
                if (Random::get_int(1, free_count) == 1) {
                    selected_idx = static_cast<int>(i);
                }
            }
        }
        return selected_idx;
    }

    void EnemyManager::spawn_dark_tower_at_corrupted_tile(size_t tile_index, const Tiles& tiles) {
        if (tile_index >= m_corrupted_tiles.size() || !m_corrupted_tiles[tile_index].is_available()) {
            return;
        }

        m_corrupted_tiles[tile_index].is_occupied = true;
        float px = static_cast<float>(m_corrupted_tiles[tile_index].tile_x * tiles.tile_size());
        float py = static_cast<float>(m_corrupted_tiles[tile_index].tile_y * tiles.tile_size());

        WorldStructure& dt = m_world_structures.emplace_back(px, py, StructureType::DarkTower);
        dt.corrupted_tile_index = static_cast<int>(tile_index);
        dt.next_spawn_cooldown = DarkTowerConstants::INITIAL_SPAWN_DELAY;
        m_tower_spawned_event = true;
    }

    void EnemyManager::spawn_dark_tower(float x, float y) {
        WorldStructure& dt = m_world_structures.emplace_back(x, y, StructureType::DarkTower);
        dt.next_spawn_cooldown = DarkTowerConstants::INITIAL_SPAWN_DELAY;
        m_tower_spawned_event = true;
    }

    float EnemyManager::calculate_inverse_twilight_cooldown(float twilight_level) const {
        float clamped_t = std::clamp(twilight_level, 0.0f, 1.0f);
        float min_cd = std::lerp(DarkTowerConstants::TWILIGHT_PURIFIED_MIN_COOLDOWN, DarkTowerConstants::TWILIGHT_CORRUPTED_MIN_COOLDOWN, clamped_t);
        float max_cd = std::lerp(DarkTowerConstants::TWILIGHT_PURIFIED_MAX_COOLDOWN, DarkTowerConstants::TWILIGHT_CORRUPTED_MAX_COOLDOWN, clamped_t);
        return Random::get_float(min_cd, max_cd);
    }

    float EnemyManager::consume_pending_twilight_increase() {
        float val = m_pending_twilight_increase;
        m_pending_twilight_increase = 0.0f;
        return val;
    }

    bool EnemyManager::consume_tower_spawned_event() {
        bool val = m_tower_spawned_event;
        m_tower_spawned_event = false;
        return val;
    }

    void EnemyManager::spawn_enemy_wave(const Tiles& tiles, const Network* network , int count , float player_start_x , float player_start_y , bool clear_existing ) {
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

    void EnemyManager::update(float dt, Player* player, const Tiles& tiles, Network& network, ParticleSystem* particles, float twilight_level) {
        m_scan_timer += dt;
        m_scan_age += dt;
        if (m_scan_timer >= m_next_scan_interval) {
            m_scan_timer = 0.0f;
            m_scan_age = 0.0f;
            update_threat_cache();
            m_next_scan_interval = Random::get_float(INDICATOR_SCAN_INTERVAL_MIN, INDICATOR_SCAN_INTERVAL_MAX);
        }

        // Process Dark Towers
        for (auto& struct_obj : m_world_structures) {
            struct_obj.update(dt);
            if (struct_obj.type == StructureType::DarkTower) {
                // Pulse Logic
                if (struct_obj.pulse_timer >= 10.0f) {
                    struct_obj.pulse_timer = 0.0f;
                    m_pending_twilight_increase += 0.02f;
                    if (particles) {
                        ParticleEmitters::spawn_tower_pulse(*particles, struct_obj.center_x(), struct_obj.center_y(), 64.0f);
                    }
                }
                
                // Spawner Logic with twilight speedup
                float speedup = (twilight_level >= DarkTowerConstants::CRITICAL_TWILIGHT_THRESHOLD) ? DarkTowerConstants::TWILIGHT_SPEEDUP_FACTOR : 1.0f;
                struct_obj.spawn_timer += dt * speedup;

                if (struct_obj.spawn_timer >= struct_obj.next_spawn_cooldown) {
                    struct_obj.spawn_timer = 0.0f;
                    struct_obj.next_spawn_cooldown = calculate_inverse_twilight_cooldown(twilight_level);
                    spawn_dark_tower_wave(struct_obj, tiles, network);
                }
            }
        }

        // Emergence Spawner Loop: Re-emerge a Dark Tower if active towers drop below target count
        int active_dark_towers = 0;
        for (const auto& s : m_world_structures) {
            if (s.type == StructureType::DarkTower && s.hp > 0) {
                ++active_dark_towers;
            }
        }

        if (active_dark_towers < DarkTowerConstants::TARGET_ACTIVE_DARK_TOWERS) {
            m_tower_emergence_timer += dt;
            if (m_tower_emergence_timer >= m_next_emergence_cooldown) {
                m_tower_emergence_timer = 0.0f;
                m_next_emergence_cooldown = Random::get_float(DarkTowerConstants::EMERGENCE_COOLDOWN_MIN, DarkTowerConstants::EMERGENCE_COOLDOWN_MAX);
                int target_tile_idx = find_unoccupied_corrupted_tile_index();
                if (target_tile_idx >= 0) {
                    spawn_dark_tower_at_corrupted_tile(static_cast<size_t>(target_tile_idx), tiles);
                }
            }
        } else {
            m_tower_emergence_timer = 0.0f;
        }
        
        // Process Shadow Eggs
        for (auto it = m_shadow_eggs.begin(); it != m_shadow_eggs.end(); ) {
            it->update(dt);
            if (it->hatched) {
                if (particles) ParticleEmitters::spawn_egg_hatch(*particles, it->center_x(), it->center_y());
                Enemy& e = m_enemies.emplace_back(it->x, it->y);
                e.state = EnemyState::Wander;
                e.state_timer = Random::get_float(4.0f, 6.0f);
                EnemyMovement::reset_wander_state(e.move_state);
                it = m_shadow_eggs.erase(it);
            } else if (it->destroyed) {
                if (particles) ParticleEmitters::spawn_egg_shatter(*particles, it->center_x(), it->center_y());
                it = m_shadow_eggs.erase(it);
            } else {
                ++it;
            }
        }

        update_enemy_ai(dt, player, tiles, network, particles);
        update_enemy_push_separation(tiles, network);

        if (player) {
            update_combat_and_loot(*player, particles);
            
            if (player->consume_mana_spark_fire()) {
                m_mana_sparks.emplace_back(
                    player->center_x(), player->center_y(),
                    player->facing_dx * 200.0f, player->facing_dy * 200.0f
                );
            }
        }
        
        // Process Alloy Items (Magnet and Lifetime)
        for (auto it = m_alloy_items.begin(); it != m_alloy_items.end(); ) {
            it->update(dt);
            if (!it->active) {
                it = m_alloy_items.erase(it);
                continue;
            }
            
            if (player) {
                float dx = player->center_x() - it->center_x();
                float dy = player->center_y() - it->center_y();
                float dist_sq = dx * dx + dy * dy;
                // Magnet radius: 1 tile (16px) -> 256
                if (dist_sq < 256.0f && dist_sq > 0.001f) {
                    float dist = std::sqrt(dist_sq);
                    float magnet_speed = 60.0f; // px/s
                    it->x += (dx / dist) * magnet_speed * dt;
                    it->y += (dy / dist) * magnet_speed * dt;
                }
            }
            ++it;
        }
        
        // Process ManaSparks
        for (auto it = m_mana_sparks.begin(); it != m_mana_sparks.end(); ) {
            it->update(dt);
            if (it->lifetime <= 0.0f) {
                it = m_mana_sparks.erase(it);
                continue;
            }
            
            // Check collision with enemies
            bool hit = false;
            for (auto& enemy : m_enemies) {
                if (enemy.is_dead()) continue;
                float contact_x, contact_y;
                Collision::Circle spark_c{it->x, it->y, 2.0f};
                if (Collision::circle_contact_point(spark_c, enemy.hurt_circle(), contact_x, contact_y)) {
                    enemy.take_damage(it->damage, it->vx, it->vy, 50.0f, contact_x - enemy.transform.x, contact_y - enemy.transform.y);
                    if (particles) {
                        ParticleEmitters::spawn_hit_blood(*particles, contact_x, contact_y, it->vx, it->vy, 15, Layer::WorldObjFX, enemy.transform.y + enemy.transform.height, 0.5f);
                    }
                    hit = true;
                    break; // Spark destroyed
                }
            }
            
            if (hit) {
                it = m_mana_sparks.erase(it);
                continue;
            }
            
            // Check DarkTowers
            for (auto& struct_obj : m_world_structures) {
                if (struct_obj.type != StructureType::DarkTower) continue;
                Collision::Circle spark_c{it->x, it->y, 2.0f};
                if (Collision::circle_vs_aabb(spark_c, struct_obj.ground_aabb())) { 
                    struct_obj.take_damage(it->damage);
                    if (particles) {
                        int tower_sort_y = static_cast<int>(struct_obj.transform.y + struct_obj.transform.height);
                        ParticleEmitters::spawn_hit_blood(*particles, it->x, it->y, it->vx, it->vy, 10, Layer::WorldObjFX, tower_sort_y, 0.5f);
                    }
                    hit = true;
                    break;
                }
            }

            if (hit) {
                it = m_mana_sparks.erase(it);
                continue;
            }

            // Check walls and tall fixtures (STFH)
            int tx = static_cast<int>(it->x) / tiles.tile_size();
            int ty = static_cast<int>(it->y) / tiles.tile_size();
            bool wall_hit = false;
            
            if (tiles.is_wall(tx, ty)) {
                wall_hit = true;
            } else if (network.is_tall_fixture(tx, ty)) {
                wall_hit = true;
            }
            
            if (wall_hit) {
                if (particles) {
                    ParticleEmitters::spawn_hit_blood(*particles, it->x, it->y, -it->vx * 0.5f, -it->vy * 0.5f, 5, Layer::WorldObjFX, static_cast<int>(it->y) + 8, 0.5f);
                }
                it = m_mana_sparks.erase(it);
                continue;
            }
            
            ++it;
        }
    }

    bool EnemyManager::is_solid_ground(const Collision::Circle& ground, const Tiles& tiles, const Network& network) const {
        return WorldCollision::is_solid_ground(ground, tiles, network, &m_world_structures);
    }

    // static void EnemyManager::enforce_solid_ground_ejection(Enemy& enemy, const Tiles& tiles, const Network& network) {
    //     WorldCollision::enforce_solid_ground_ejection(enemy.transform.x, enemy.transform.y, enemy.ground_circle(), tiles, network, 2.0f, enemy.tag);
    // }

    GridPos EnemyManager::find_priority_target(const Enemy& enemy, const Network& network) const {
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

    void EnemyManager::update_enemy_ai(float dt, Player* player, const Tiles& tiles, Network& network, ParticleSystem* particles ) {
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

                            int blood_z = Layer::WorldObjFX; // z = 16: Renders on top of player and enemy sprites
                            int blood_sort_y = enemy_sort_y + 20;

                            if (wave_index == 1) {
                                // Wave 2: 17 droplets mid-slide, 50% momentum
                                ParticleEmitters::spawn_hit_blood(*particles, emit_x, emit_y, kb_vx, kb_vy, 17, blood_z, blood_sort_y, 0.50f);
                            } else {
                                // Wave 3: 13 droplets final deceleration drip, 20% momentum
                                ParticleEmitters::spawn_hit_blood(*particles, emit_x, emit_y, kb_vx, kb_vy, 13, blood_z, blood_sort_y, 0.20f);
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

    void EnemyManager::update_enemy_push_separation(const Tiles& tiles, const Network& network) {
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

    void EnemyManager::update_combat_and_loot(Player& player, ParticleSystem* particles ) {
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
                            int blood_z = Layer::WorldObjFX; // z = 16: Renders on top of player and enemy sprites
                            int blood_sort_y = enemy_sort_y + 20;

                            // Wave 1: 20 droplets on hit contact, 85% momentum
                            ParticleEmitters::spawn_hit_blood(*particles, contact_x, contact_y, kb_vx, kb_vy, 20, blood_z, blood_sort_y, 0.85f);
                        }
                    }
                } // end m_enemies loop
                for (auto& struct_obj : m_world_structures) {
                    if (struct_obj.type != StructureType::DarkTower) continue;
                    if (struct_obj.last_hit_swing_id == player.current_swing_id) continue;
                    if (Collision::circle_vs_aabb(hit_c, struct_obj.ground_aabb())) {
                        struct_obj.last_hit_swing_id = player.current_swing_id;
                        struct_obj.take_damage(1);
                        if (particles) {
                            int tower_sort_y = static_cast<int>(struct_obj.transform.y + struct_obj.transform.height);
                            ParticleEmitters::spawn_hit_blood(*particles, hit_c.cx, hit_c.cy, player.facing_dx * 100.0f, player.facing_dy * 100.0f, 10, Layer::WorldObjFX, tower_sort_y, 0.5f);
                        }
                    }
                }

                for (auto& egg : m_shadow_eggs) {
                    if (egg.hatched || egg.destroyed) continue;
                    if (egg.last_hit_swing_id == player.current_swing_id) continue;
                    float contact_x = 0.0f, contact_y = 0.0f;
                    if (Collision::circle_contact_point(hit_c, egg.hurt_circle(), contact_x, contact_y)) {
                        egg.last_hit_swing_id = player.current_swing_id;
                        egg.take_damage(1);
                        if (particles) {
                            int egg_sort_y = static_cast<int>(egg.y + egg.height);
                            ParticleEmitters::spawn_hit_blood(*particles, contact_x, contact_y, player.facing_dx * 150.0f, player.facing_dy * 150.0f, 15, Layer::WorldObjFX, egg_sort_y, 0.7f);
                        }
                    }
                }
            }
        }

        // --- 2. ENEMY & STRUCTURE DEATH & LOOT DROP ---
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
        
        for (auto it = m_world_structures.begin(); it != m_world_structures.end(); ) {
            if (it->type == StructureType::DarkTower && it->hp <= 0) {
                if (particles) {
                    ParticleEmitters::spawn_tower_shatter(*particles, it->center_x(), it->center_y());
                }
                // Free associated corrupted tile slot
                if (it->corrupted_tile_index >= 0 && static_cast<size_t>(it->corrupted_tile_index) < m_corrupted_tiles.size()) {
                    m_corrupted_tiles[it->corrupted_tile_index].is_occupied = false;
                }
                // Scatter loot (5 Alloy pieces)
                for (int i = 0; i < 5; ++i) {
                    float ax = it->center_x() + Random::get_float(-24.0f, 24.0f);
                    float ay = it->center_y() + Random::get_float(-24.0f, 24.0f);
                    m_alloy_items.emplace_back(ax, ay);
                }
                it = m_world_structures.erase(it);
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

    void EnemyManager::draw_enemies(std::vector<uint32_t>& pixel_buffer, float alpha) const {
        const alx::Camera* cam = static_cast<const alx::Camera*>(Draw::active_camera());

        for (const auto& item : m_alloy_items) {
            if (!cam || cam->is_aabb_visible(item.x, item.y, item.width, item.height)) {
                item.draw(pixel_buffer, alpha);
            }
        }

        for (const auto& struct_obj : m_world_structures) {
            if (!cam || cam->is_aabb_visible(struct_obj.transform.x, struct_obj.transform.y, struct_obj.transform.width, struct_obj.transform.height)) {
                struct_obj.draw(pixel_buffer, alpha);
            }
        }

        for (const auto& egg : m_shadow_eggs) {
            if (!cam || cam->is_aabb_visible(egg.x, egg.y, egg.width, egg.height)) {
                egg.draw(pixel_buffer, alpha);
            }
        }

        for (const auto& enemy : m_enemies) {
            if (!cam || cam->is_aabb_visible(enemy.transform.x, enemy.transform.y, enemy.transform.width, enemy.transform.height)) {
                enemy.draw(pixel_buffer, alpha);
            }
        }

        for (const auto& spark : m_mana_sparks) {
            if (!cam || cam->is_aabb_visible(spark.x - ManaSpark::HALF_SIZE, spark.y - ManaSpark::HALF_SIZE, ManaSpark::DEFAULT_SIZE, ManaSpark::DEFAULT_SIZE)) {
                spark.draw(pixel_buffer, alpha);
            }
        }
    }

    void EnemyManager::draw_threat_indicators(const alx::Camera& camera) const {
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

    void EnemyManager::update_threat_cache() {
        m_cached_threat_positions.clear();
        m_cached_threat_positions.reserve(m_enemies.size());
        for (const auto& enemy : m_enemies) {
            m_cached_threat_positions.push_back(CachedThreatPos{ enemy.center_x(), enemy.center_y() });
        }
    }

    void EnemyManager::spawn_dark_tower_wave(WorldStructure& tower, const Tiles& tiles, Network& network) {
        int active_eggs_count = 0;
        Collision::AABB tower_aabb = tower.ground_aabb();
        float tc_x = tower.center_x();
        float tc_y = tower.center_y();

        // Count active unhatched eggs belonging to/around this tower
        float tower_radius_check = (WorldStructure::DARK_TOWER_WIDTH + WorldStructure::DARK_TOWER_HEIGHT) * 0.5f + (tiles.tile_size() * 4.0f);
        for (const auto& egg : m_shadow_eggs) {
            if (egg.hatched || egg.destroyed) continue;
            float dx = egg.center_x() - tc_x;
            float dy = egg.center_y() - tc_y;
            if ((dx * dx + dy * dy) <= (tower_radius_check * tower_radius_check)) {
                active_eggs_count++;
            }
        }

        if (active_eggs_count >= DarkTowerConstants::MAX_ACTIVE_EGGS_PER_TOWER) {
            return;
        }

        int eggs_to_spawn = Random::get_int(DarkTowerConstants::WAVE_EGG_COUNT_MIN, DarkTowerConstants::WAVE_EGG_COUNT_MAX);
        int remaining_room_capacity = static_cast<int>(MAX_ACTIVE_ENEMIES) - static_cast<int>(m_enemies.size() + m_shadow_eggs.size());
        int max_can_spawn = std::min(DarkTowerConstants::MAX_ACTIVE_EGGS_PER_TOWER - active_eggs_count, remaining_room_capacity);
        eggs_to_spawn = std::clamp(eggs_to_spawn, 0, std::max(0, max_can_spawn));

        if (eggs_to_spawn <= 0) return;

        static constexpr float TWO_PI = 6.28318530718f;
        float base_angle = Random::get_float(0.0f, TWO_PI);
        float angle_step = TWO_PI / static_cast<float>(eggs_to_spawn);

        float tower_max_extent = std::max(WorldStructure::DARK_TOWER_WIDTH * 0.5f, WorldStructure::DARK_TOWER_HEIGHT * 0.5f) + (ShadowEgg::EGG_WIDTH * 0.5f);

        for (int i = 0; i < eggs_to_spawn; ++i) {
            float angle = base_angle + i * angle_step + Random::get_float(-0.2f, 0.2f);
            float offset_ratio = Random::get_float(DarkTowerConstants::SPAWN_TILE_OFFSET_MIN_RATIO, DarkTowerConstants::SPAWN_TILE_OFFSET_MAX_RATIO);
            float dist = tower_max_extent + (tiles.tile_size() * offset_ratio);

            float target_x = tc_x + dist * std::cos(angle) - (ShadowEgg::EGG_WIDTH * 0.5f);
            float target_y = tc_y + dist * std::sin(angle) - (ShadowEgg::EGG_HEIGHT * 0.5f);

            bool spot_found = false;
            // 16-step Rotational Ring Search Safety Fallback
            for (int step = 0; step < DarkTowerConstants::SPAWN_RING_SEARCH_STEPS; ++step) {
                float test_angle = angle + (step * (TWO_PI / DarkTowerConstants::SPAWN_RING_SEARCH_STEPS));
                float candidate_x = tc_x + dist * std::cos(test_angle) - (ShadowEgg::EGG_WIDTH * 0.5f);
                float candidate_y = tc_y + dist * std::sin(test_angle) - (ShadowEgg::EGG_HEIGHT * 0.5f);

                ShadowEgg test_egg(candidate_x, candidate_y);
                Collision::AABB egg_aabb = test_egg.ground_aabb();

                // 1. Ensure egg does not overlap DarkTower bounding box
                if (Collision::aabb_vs_aabb(egg_aabb, tower_aabb)) continue;

                // 2. Ensure egg is on solid floor (not in walls, out of bounds, or inside pipe fixtures)
                if (!is_solid_ground(test_egg.hurt_circle(), tiles, network)) continue;

                // 3. Ensure candidate does not heavily overlap existing eggs
                bool overlaps_egg = false;
                for (const auto& existing_egg : m_shadow_eggs) {
                    if (existing_egg.hatched || existing_egg.destroyed) continue;
                    if (Collision::aabb_vs_aabb(egg_aabb, existing_egg.ground_aabb())) {
                        overlaps_egg = true;
                        break;
                    }
                }
                if (overlaps_egg) continue;

                // Found valid landing spot! Launch egg in arc trajectory from tower top center
                float start_x = tc_x - (ShadowEgg::EGG_WIDTH * 0.5f);
                float start_y = tc_y - (ShadowEgg::EGG_HEIGHT * 0.5f);
                m_shadow_eggs.emplace_back(start_x, start_y, candidate_x, candidate_y, ShadowEggConstants::EJECT_FLIGHT_DURATION);
                spot_found = true;
                break;
            }

            if (!spot_found) {
                // Fallback: Launch directly to initial candidate coordinate
                float start_x = tc_x - (ShadowEgg::EGG_WIDTH * 0.5f);
                float start_y = tc_y - (ShadowEgg::EGG_HEIGHT * 0.5f);
                m_shadow_eggs.emplace_back(start_x, start_y, target_x, target_y, ShadowEggConstants::EJECT_FLIGHT_DURATION);
            }
        }
    }

    void EnemyManager::draw_corrupted_tiles(int tile_size) const {
        float w = WorldStructure::DARK_TOWER_WIDTH;
        float h = WorldStructure::DARK_TOWER_HEIGHT;
        uint32_t fill_color = 0x7F120B1C;   // 50% opacity Deep Obsidian Base
        uint32_t border_color = 0x7F2A153D; // 50% opacity Dark Trim Accent Border

        for (const auto& tile : m_corrupted_tiles) {
            if (tile.is_available()) {
                float wx = static_cast<float>(tile.tile_x * tile_size);
                float wy = static_cast<float>(tile.tile_y * tile_size);
                int roof_sort_y = static_cast<int>(wy + 16.0f);

                Draw::rect(wx, wy, w, h, fill_color, true, 1, Layer::WorldObjBG, roof_sort_y);
                Draw::rect(wx, wy, w, h, border_color, false, 2, Layer::WorldObjBG, roof_sort_y);
            }
        }
    }

} // namespace alx
