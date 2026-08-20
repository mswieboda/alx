#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <random>
#include <vector>
#include <span>
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
#include "alx/CorruptedDarkTowerTile.h"
#include "core/Draw.h"

namespace alx {

namespace DarkTowerConstants {
    constexpr float SPAWN_INTERVAL_MIN = 6.0f;
    constexpr float SPAWN_INTERVAL_MAX = 12.0f;
    constexpr float INITIAL_SPAWN_DELAY = 3.0f;
    constexpr int WAVE_EGG_COUNT_MIN = 1;
    constexpr int WAVE_EGG_COUNT_MAX = 3;
    constexpr int MAX_ACTIVE_EGGS_PER_TOWER = 5;
    constexpr float SPAWN_TILE_OFFSET_MIN_RATIO = 0.25f;
    constexpr float SPAWN_TILE_OFFSET_MAX_RATIO = 2.00f;
    constexpr int SPAWN_RING_SEARCH_STEPS = 16;
    constexpr int MAX_ACTIVE_DARK_TOWERS = 3;
    constexpr float TELEGRAPH_DURATION = 1.0f;
    constexpr float CRITICAL_TWILIGHT_THRESHOLD = 0.75f;
    constexpr float TWILIGHT_SPEEDUP_FACTOR = 1.25f;

    // Twilight Scaling Curve Thresholds & Exponent
    constexpr float TWILIGHT_LOW_THRESHOLD = 0.075f;
    constexpr float TWILIGHT_HIGH_THRESHOLD = 0.90f;
    constexpr float DEFAULT_CURVE_EXPONENT = 0.66f;

    // Pulse & Egg Twilight Bumps
    constexpr float PULSE_INTERVAL_MIN = 6.6f;
    constexpr float PULSE_INTERVAL_MAX = 11.3f;
    constexpr float PULSE_TWILIGHT_INCREASE = 0.05f;
    constexpr float EGG_WAVE_TWILIGHT_INCREASE = 0.005f;

    // Inverse Twilight Cooldown Scaling (Seconds)
    // High Twilight (1.0 = Corrupted room): Slower tower spawn cooldown range
    constexpr float TWILIGHT_CORRUPTED_EGG_SPAWN_MIN_COOLDOWN = 23.0f;
    constexpr float TWILIGHT_CORRUPTED_EGG_SPAWN_MAX_COOLDOWN = 33.0f;

    // Low Twilight (0.0 = Purified room): Faster tower spawn cooldown range (escalation)
    constexpr float TWILIGHT_PURIFIED_EGG_SPAWN_MIN_COOLDOWN = 15.0f;
    constexpr float TWILIGHT_PURIFIED_EGG_SPAWN_MAX_COOLDOWN = 25.0f;

    // Normal Periodic Emergence Settings (for Twilight > 10%) might be `TWILIGHT_LOW_THRESHOLD` instead
    constexpr float EMERGENCE_COOLDOWN_MIN = 45.0f;
    constexpr float EMERGENCE_COOLDOWN_MAX = 60.0f;

    // Low Twilight Crunch Emergence Settings (for Twilight <= 10%) might be `TWILIGHT_LOW_THRESHOLD` instead
    constexpr float CRUNCH_EMERGENCE_TOWER_SPAWN_COOLDOWN_MIN = 30.0f;
    constexpr float CRUNCH_EMERGENCE_TOWER_SPAWN_COOLDOWN_MAX = 45.0f;
} // namespace DarkTowerConstants

namespace FrenzyConstants {
    constexpr float FREQ_MULTIPLIER_LV1 = 1.20f; // Level 1: +20% spawn frequency
    constexpr float FREQ_MULTIPLIER_LV2 = 1.35f; // Level 2: +35% spawn frequency
    constexpr float FREQ_MULTIPLIER_LV3 = 1.50f; // Level 3: +50% spawn frequency
    constexpr float DEFAULT_FREQ_MULTIPLIER = 1.0f;

    [[nodiscard]] constexpr float get_frenzy_multiplier(int level_id) noexcept {
        switch (level_id) {
            case 1: return FREQ_MULTIPLIER_LV1;
            case 2: return FREQ_MULTIPLIER_LV2;
            case 3: return FREQ_MULTIPLIER_LV3;
            default: return (level_id > 3) ? FREQ_MULTIPLIER_LV3 : DEFAULT_FREQ_MULTIPLIER;
        }
    }
} // namespace FrenzyConstants

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
    std::vector<CorruptedDarkTowerTile> m_corrupted_tiles;
    float m_scan_timer = 0.0f;
    float m_next_scan_interval = 2.0f;
    float m_scan_age = 999.0f; // Prevent initial rendering until scan/spawn
    float m_tower_emergence_timer = 0.0f;
    float m_next_emergence_cooldown = DarkTowerConstants::EMERGENCE_COOLDOWN_MIN;
    std::vector<CachedThreatPos> m_cached_threat_positions;
    bool m_attack_hit_registered = false;
    float m_pending_twilight_increase = 0.0f;
    bool m_tower_spawned_event = false;
    bool m_tower_hit_event = false;
    int m_last_destroyed_tile_index = -1;
    bool m_is_frenzy = false;
    float m_frenzy_multiplier = 1.0f;
    float m_frenzy_timer = 0.0f;

public:
    void clear();

    void register_corrupted_tiles(const std::vector<std::pair<int, int>>& coords, const Tiles& tiles);
    int clear_enemies_near(float center_x, float center_y, float radius);
    [[nodiscard]] int count_active_dark_towers() const;
    [[nodiscard]] int find_unoccupied_corrupted_tile_index() const;
    void spawn_dark_tower_at_corrupted_tile(size_t tile_index, const Tiles& tiles);
    void spawn_dark_tower(float x, float y);
    [[nodiscard]] float calculate_twilight_cooldown(float twilight_level, float min_cd, float max_cd, float exponent = DarkTowerConstants::DEFAULT_CURVE_EXPONENT) const;
    [[nodiscard]] float calculate_inverse_twilight_cooldown(float twilight_level) const;

    [[nodiscard]] std::span<const CorruptedDarkTowerTile> corrupted_tiles() const noexcept { return m_corrupted_tiles; }
    std::vector<WorldStructure>& structures() { return m_world_structures; }
    const std::vector<WorldStructure>& structures() const { return m_world_structures; }

    [[nodiscard]] bool is_frenzy() const noexcept { return m_is_frenzy; }
    [[nodiscard]] float frenzy_multiplier() const noexcept { return m_frenzy_multiplier; }

    float consume_pending_twilight_increase();
    [[nodiscard]] bool consume_tower_spawned_event();
    [[nodiscard]] bool consume_tower_hit_event() noexcept;

    void spawn_enemy_wave(const Tiles& tiles, const Network* network = nullptr, int count = -1, float player_start_x = -1.0f, float player_start_y = -1.0f, bool clear_existing = false);

    void update(float dt, Player* player, const Tiles& tiles, Network& network, ParticleSystem* particles = nullptr, float twilight_level = 0.0f, bool is_frenzy = false, float frenzy_multiplier = 1.0f);


    bool is_solid_ground(const Collision::Circle& ground, const Tiles& tiles, const Network& network) const;


    GridPos find_priority_target(const Enemy& enemy, const Network& network) const;


    void update_enemy_ai(float dt, Player* player, const Tiles& tiles, Network& network, ParticleSystem* particles = nullptr);


    void update_enemy_push_separation(const Tiles& tiles, const Network& network);


    void update_combat_and_loot(Player& player, ParticleSystem* particles = nullptr);


    void draw_enemies(std::vector<uint32_t>& pixel_buffer, float alpha) const;


    void draw_threat_indicators(const alx::Camera& camera) const;


    void draw_corrupted_tiles(int tile_size) const;


    const std::vector<Enemy>& enemies() const { return m_enemies; }
    const std::vector<ShadowEgg>& shadow_eggs() const { return m_shadow_eggs; }
    const std::vector<AlloyItem>& alloy_items() const { return m_alloy_items; }

private:
    void update_threat_cache();
    void spawn_dark_tower_wave(WorldStructure& tower, const Tiles& tiles, Network& network, const Player* player = nullptr);
    [[nodiscard]] float calculate_fixture_threat(const Fixture& fixture) const;
    void update_player_aggro(Enemy& enemy, const Player* player, float dt, const Tiles& tiles, const Network* network = nullptr);
};

} // namespace alx
