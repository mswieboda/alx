#pragma once
#include <cstdint>
#include <vector>
#include "core/Scene.h"
#include "alx/Camera.h"
#include "alx/Tiles.h"
#include "alx/Network.h"
#include "alx/Player.h"
#include "alx/EnemyManager.h"
#include "alx/ParticleSystem.h"

namespace alx {

class MainScene : public Scene {
private:
    // --- CONSTANTS ---
    static constexpr float TWILIGHT_MAX = 0.9f;
    static constexpr float TWILIGHT_DECREASE_PER_MANA = 0.005f;
    static constexpr float TELEMETRY_DUMP_INTERVAL = 0.1f;

    Tiles m_tiles;
    Network m_network;
    Player m_player;
    EnemyManager m_enemy_manager;
    ParticleSystem m_particle_system;
    alx::Camera m_camera;
    float m_sim_timer = 0;
    float m_last_dt = 0.016f;
    const float SIM_TICK_RATE = 0.6f; // Speed of the mana flow
    bool m_paused = false;
    std::vector<uint32_t> m_twilight_pixel_buffer;

    // Level-specific progress stats
    float m_twilight_level = TWILIGHT_MAX;
    float m_wand_radius = 56.0f;
    int m_current_level_id = 1;

    // Sword slash tip tracking for gapless swipe line rendering
    float m_slash_prev_tip_x = 0.0f;
    float m_slash_prev_tip_y = 0.0f;
    bool m_slash_was_attacking = false;

    // Telemetry & Time Dilation
    float m_time_scale{1.0f};
    int64_t m_sim_tick_count{0};
    float m_sim_elapsed_sec{0.0f};
    float m_last_twilight_level{TWILIGHT_MAX};
    float m_twilight_delta_per_sec{0.0f};
    float m_telemetry_dump_timer{0.0f};

    // Summary Statistics Tracking
    float m_initial_twilight{TWILIGHT_MAX};
    float m_peak_twilight{0.0f};
    float m_min_twilight{1.0f};
    double m_sum_twilight{0.0};
    float m_time_to_max_twilight{-1.0f};

    void update_tick_simulation(float dt);
    void dump_telemetry_snapshot();
    void draw_twilight(std::vector<uint32_t>& pixel_buffer, float alpha);
    void draw_hud();
    void draw_tiles_and_network(std::vector<uint32_t>& pixel_buffer, float progress);
    void draw_terrain_tile(const Tile& tile, int world_x, int world_y, int tile_size);

public:
    void print_headless_summary_report(int64_t seed = -1);
    Camera& camera() override { return m_camera; }
    const Camera& camera() const override { return m_camera; }

    void init(SceneManager& sm) override;
    void load_level(int level_id);
    void load_tiles_and_network(
        const std::vector<std::pair<int, int>>& seeps,
        const std::vector<std::pair<int, int>>& refiners,
        const std::vector<std::pair<int, int>>& spires,
        const std::vector<std::pair<int, int>>& pipes
    );
    void update_camera_map_boundary();

    void update(SceneManager& sm, float dt) override;
    void sync_camera(float alpha) override;
    void draw_world(std::vector<uint32_t>& pixel_buffer, float alpha) override;
    void draw_screen(std::vector<uint32_t>& pixel_buffer, float alpha) override;

    bool is_connectable_fixture(int gx, int gy) const;
    bool connects_dark_mana(int gx, int gy) const;
    bool is_node_fixture(int gx, int gy) const;

    Tiles& tiles() { return m_tiles; }
    const Tiles& tiles() const { return m_tiles; }
    Network& network() { return m_network; }
    const Network& network() const { return m_network; }
};

} // namespace alx

