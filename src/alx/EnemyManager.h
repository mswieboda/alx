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
#include "alx/ShadowEgg.h"
#include "alx/ManaSpark.h"
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
    std::vector<ShadowEgg> m_shadow_eggs;
    std::vector<ManaSpark> m_mana_sparks;
    float m_scan_timer = 0.0f;
    float m_next_scan_interval = 2.0f;
    float m_scan_age = 999.0f; // Prevent initial rendering until scan/spawn
    std::vector<CachedThreatPos> m_cached_threat_positions;
    bool m_attack_hit_registered = false;
    float m_pending_twilight_increase = 0.0f;


public:
    void clear() ;


    void spawn_dark_tower(float x, float y) ;


    std::vector<WorldStructure>& structures() { return m_world_structures; }
    const std::vector<WorldStructure>& structures() const { return m_world_structures; }

    float consume_pending_twilight_increase() ;


    void spawn_enemy_wave(const Tiles& tiles, const Network* network = nullptr, int count = -1, float player_start_x = -1.0f, float player_start_y = -1.0f, bool clear_existing = false) ;


    void update(float dt, Player* player, const Tiles& tiles, Network& network, ParticleSystem* particles = nullptr) ;


    bool is_solid_ground(const Collision::Circle& ground, const Tiles& tiles, const Network& network) const ;


    // static void enforce_solid_ground_ejection(Enemy& enemy, const Tiles& tiles, const Network& network) ;


    GridPos find_priority_target(const Enemy& enemy, const Network& network) const ;


    void update_enemy_ai(float dt, Player* player, const Tiles& tiles, Network& network, ParticleSystem* particles = nullptr) ;


    void update_enemy_push_separation(const Tiles& tiles, const Network& network) ;


    void update_combat_and_loot(Player& player, ParticleSystem* particles = nullptr) ;


    void draw_enemies(std::vector<uint32_t>& pixel_buffer, float alpha) const ;


    void draw_threat_indicators(const alx::Camera& camera) const ;


    const std::vector<Enemy>& enemies() const { return m_enemies; }

private:
    void update_threat_cache() ;

};

} // namespace alx
