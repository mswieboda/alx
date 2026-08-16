#pragma once
#include <array>
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>
#include "core/Scene.h"
#include "alx/Camera.h"
#include "alx/Tiles.h"
#include "alx/Network.h"
#include "alx/Player.h"
#include "alx/EnemyManager.h"
#include "alx/ParticleSystem.h"
#include "alx/Levels.h"
#include "alx/Menu.h"

namespace alx {

enum class GameOverItem : uint8_t { Retry, Quit, Count };

class MainScene : public Scene {
private:
    // --- CONSTANTS ---
    static constexpr float TWILIGHT_MIN = -0.03f;
    static constexpr float TWILIGHT_HOLD_THRESHOLD = 0.0f;
    static constexpr float TWILIGHT_MAX = 0.99f;
    static constexpr float TWILIGHT_DECREASE_PER_MANA = 0.03f;
    static constexpr float TELEMETRY_DUMP_INTERVAL = 0.1f;
    static constexpr float VICTORY_HOLD_DURATION_SEC = 15.0f;
    static constexpr float VICTORY_HOLD_DRAIN_RATE = 2.0f;
    static constexpr uint32_t COLOR_VICTORY_TEXT = 0xFF00FF88;
    static constexpr uint32_t COLOR_GAME_OVER_TEXT = 0xFF66001C;
    static constexpr uint32_t COLOR_PAUSE_TEXT   = 0xFFFFCC00;
    static constexpr float GAME_OVER_FADE_DURATION = 0.5f; // sec
    static constexpr int HUD_BAR_WIDTH = 120;
    static constexpr int HUD_BAR_HEIGHT = 16;
    static constexpr float HUD_BAR_CORNER_RADIUS = 3.0f;
    static constexpr int HUD_BAR_BORDER_THICKNESS = 2;
    static constexpr uint32_t HUD_BAR_BG_COLOR = 0xCC00141C;
    static constexpr uint32_t HUD_BAR_BORDER_COLOR = 0xFF004455;
    static constexpr uint32_t HUD_BAR_FILL_HOLD = 0xDD33FFFF;
    static constexpr uint32_t HUD_BAR_FILL_TWILIGHT = 0xCC662288;

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
#if ALX_ENABLE_HEADLESS
    bool m_is_headless = false;
    float m_headless_defend_timer = 0.0f;
#endif
    // victory
    float m_victory_hold_timer = 0.0f;
    bool m_victory_achieved = false;
    std::vector<uint32_t> m_twilight_pixel_buffer;

    // game over
    bool m_is_game_over = false;
    float m_game_over_fade_timer = 0.0f;
    static constexpr std::array<std::string_view, static_cast<size_t>(GameOverItem::Count)> GAME_OVER_ITEMS = {
        "Retry", "Quit"
    };
    Menu m_game_over_menu{GAME_OVER_ITEMS};

    // Level-specific features
    bool m_can_build = false;

    // Level-specific progress stats
    float m_twilight_level = TWILIGHT_MAX;
    float m_wand_radius = 56.0f;
    int m_current_level_id = 1;
    GridPos m_player_spawn{9, 9};

    // Sword slash tip tracking for gapless swipe line rendering
    float m_slash_prev_tip_x = 0.0f;
    float m_slash_prev_tip_y = 0.0f;
    bool m_slash_was_attacking = false;

    // Time Dilation & Simulation Tracking
    float m_time_scale{1.0f};
    int64_t m_sim_tick_count{0};
    float m_sim_elapsed_sec{0.0f};
    float m_last_twilight_level{TWILIGHT_MAX};
    float m_time_to_zero_twilight{-1.0f};

#if ALX_ENABLE_TELEMETRY || ALX_ENABLE_HEADLESS
    // Telemetry Rolling Buffer & Event Tracking
    static constexpr float ROLLING_WINDOW_SHORT_SEC = 3.0f;
    static constexpr float ROLLING_WINDOW_LONG_SEC = 15.0f;
    static constexpr size_t ROLLING_BUFFER_MAX_SAMPLES = 960; // 16s @ 60fps

    struct RollingSample {
        float dt{0.0f};
        float delta{0.0f};
    };
    RollingSample m_rolling_samples[ROLLING_BUFFER_MAX_SAMPLES]{};
    size_t m_rolling_sample_head{0};
    size_t m_rolling_sample_count{0};

    float m_twilight_delta_per_sec{0.0f};

    // Last Event Tracking
    float m_last_event_delta{0.0f};
    std::string m_last_event_cause{"None"};
    float m_last_event_timestamp{0.0f};

    // Summary Statistics Tracking
    float m_initial_twilight{TWILIGHT_MAX};
    float m_peak_twilight{0.0f};
    float m_min_twilight{1.0f};
    double m_sum_twilight{0.0};
#endif

#if ALX_ENABLE_TELEMETRY
    float m_telemetry_dump_timer{0.0f};
#endif

    static constexpr float VIGNETTE_SURGE_DURATION = 1.0f;
    static constexpr float VIGNETTE_PEAK_INTENSITY = 0.33f;
    static constexpr float VIGNETTE_INNER_RADIUS = 0.90f;
    static constexpr float VIGNETTE_OUTER_RADIUS = 1.05f;
    static constexpr uint32_t VIGNETTE_COLOR = 0x00CC44FF;

    static constexpr float CAMERA_SHAKE_PEAK_INTENSITY = 3.33f;
    static constexpr float CAMERA_SHAKE_DURATION = 1.33f;

    float m_vignette_surge_timer = 0.0f;

    // Headless Simulation Parameters
    struct HeadlessConstants {
        static constexpr float DEFEND_INTERVAL_SEC = 7.0f;
        static constexpr float DEFEND_RADIUS_PX = 96.0f;
        static constexpr float OFFSCREEN_PLAYER_POS = -100.0f;
    };

#if ALX_ENABLE_TELEMETRY || ALX_ENABLE_HEADLESS
    void record_twilight_event(float delta, const char* cause);
    [[nodiscard]] float calculate_rolling_twilight_rate(float duration_sec = ROLLING_WINDOW_SHORT_SEC) const;
#else
    inline void record_twilight_event(float, const char*) {}
    [[nodiscard]] constexpr float calculate_rolling_twilight_rate(float = 3.0f) const { return 0.0f; }
#endif
    void update_tick_simulation(float dt);
    void update_victory_condition(float raw_dt);
    void update_game_over_fade(float raw_dt);
    void update_time_dilation_hotkeys();
    void update_player_respawn();
    void update_sword_slash_trail();
    void update_twilight_metrics(float dt, float prev_twilight);
    void draw_twilight(std::vector<uint32_t>& pixel_buffer, float alpha);
    void draw_vignette_surge();
    void draw_hud();
    void draw_game_over_fade();
    void draw_game_over_hud();
    void load_tiles_and_network(const Level& level);
    void setup_player_at_spawn(GridPos spawn_pos);
    void load_dark_towers(std::span<const DarkTowerSpawn> spawns);
    void reset_level_telemetry();
    void draw_tiles_and_network(std::vector<uint32_t>& pixel_buffer, float progress);
    void draw_terrain_tile(const Tile& tile, int world_x, int world_y, int tile_size);

public:
    void trigger_vignette_surge(float duration = VIGNETTE_SURGE_DURATION);
    void trigger_tower_spawn_alert(
        float vignette_duration = VIGNETTE_SURGE_DURATION,
        float shake_intensity = CAMERA_SHAKE_PEAK_INTENSITY,
        float shake_duration = CAMERA_SHAKE_DURATION
    );
#if ALX_ENABLE_HEADLESS
    void update_headless_defense(float dt);
    void print_headless_summary_report(int64_t seed = -1);
    void set_headless(bool headless) noexcept { m_is_headless = headless; }
    [[nodiscard]] bool is_headless() const noexcept { return m_is_headless; }
#else
    inline void update_headless_defense(float) {}
    inline void print_headless_summary_report(int64_t = -1) {}
    inline void set_headless(bool) noexcept {}
    [[nodiscard]] constexpr bool is_headless() const noexcept { return false; }
#endif

#if ALX_ENABLE_TELEMETRY
    void dump_telemetry_snapshot();
#else
    inline void dump_telemetry_snapshot() {}
#endif
    Camera& camera() override { return m_camera; }
    const Camera& camera() const override { return m_camera; }

    void init(SceneManager& sm) override;
    void load_level(int level_id);
    void update_camera_map_boundary();

    void update(SceneManager& sm, float dt) override;
    void sync_camera(float alpha) override;
    void draw_world(std::vector<uint32_t>& pixel_buffer, float alpha) override;
    void draw_screen(std::vector<uint32_t>& pixel_buffer, float alpha) override;

    void draw_victory_and_pause_overlays(int screen_width, int screen_height, const FontData& font);

    bool is_connectable_fixture(int gx, int gy) const;
    bool connects_dark_mana(int gx, int gy) const;
    bool is_node_fixture(int gx, int gy) const;

    Tiles& tiles() { return m_tiles; }
    const Tiles& tiles() const { return m_tiles; }
    Network& network() { return m_network; }
    const Network& network() const { return m_network; }
};

} // namespace alx
