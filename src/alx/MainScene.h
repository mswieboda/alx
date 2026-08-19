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
#include "alx/HUD.h"
#include "alx/TwilightOverlay.h"
#include "alx/MainSceneTelemetry.h"
#include "alx/TwilightMomentumTracker.h"

namespace alx {

enum class GameOverItem : uint8_t { Retry, Quit, Count };
enum class VictoryMenuItem : uint8_t { PlayAgain, MainMenu, Count };
enum class PauseMenuItem : uint8_t { Resume, Retry, MainMenu, Count };

class MainScene : public Scene {
private:
    // --- CONSTANTS ---
    static constexpr float TWILIGHT_MIN = -0.03f;
    static constexpr float TWILIGHT_HOLD_THRESHOLD = 0.0f;
    static constexpr float TWILIGHT_MAX = 0.99f;
    static constexpr float TWILIGHT_DECREASE_PER_MANA = 0.03f;
    static constexpr float SIM_TICK_RATE = 0.6f; // Speed of the mana flow
    static constexpr float VICTORY_HOLD_DURATION_SEC = 15.0f;
    static constexpr float VICTORY_HOLD_DRAIN_RATE = 2.0f;
    static constexpr float VICTORY_SEQUENCE_DURATION = 1.0f; // 1s win sequence before transition
    static constexpr float GAME_OVER_FADE_DURATION = 0.5f; // sec

    Tiles m_tiles;
    Network m_network;
    Player m_player;
    EnemyManager m_enemy_manager;
    ParticleSystem m_particle_system;
    alx::Camera m_camera;
    float m_sim_timer{0.0f};
    float m_last_dt{0.016f};
    bool m_paused = false;
    static constexpr std::array<std::string_view, static_cast<size_t>(PauseMenuItem::Count)> PAUSE_MENU_ITEMS = {
        "Resume", "Retry", "Main Menu"
    };
    Menu m_pause_menu{PAUSE_MENU_ITEMS};
    // victory
    float m_victory_hold_timer = 0.0f;
    float m_victory_sequence_timer = 0.0f;
    int m_last_countdown_second = -1;
    bool m_victory_achieved = false;
    bool m_is_victory_screen = false;
    static constexpr std::array<std::string_view, static_cast<size_t>(VictoryMenuItem::Count)> VICTORY_MENU_ITEMS = {
        "Play Again", "Main Menu"
    };
    Menu m_victory_menu{VICTORY_MENU_ITEMS};
    TwilightOverlay m_twilight_overlay;

    static constexpr float SHOCKWAVE_MAX_RING_THICKNESS = 5.0f;
    static constexpr float SHOCKWAVE_MIN_RING_THICKNESS = 1.0f;
    static constexpr uint32_t SHOCKWAVE_COLOR_PRIMARY   = 0x00E0FFFF;
    static constexpr uint32_t SHOCKWAVE_COLOR_ACCENT    = 0x0033FFFF;
    static constexpr uint32_t SHOCKWAVE_FLASH_COLOR     = 0x00E0FFFF;

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

    // Time Dilation & Simulation Tracking
    float m_time_scale{1.0f};
    int64_t m_sim_tick_count{0};
    float m_sim_elapsed_sec{0.0f};
    float m_last_twilight_level{TWILIGHT_MAX};
    float m_time_to_zero_twilight{-1.0f};

    MainSceneTelemetry m_telemetry;
    TwilightMomentumTracker m_momentum_tracker;

    static constexpr float CAMERA_SHAKE_PEAK_INTENSITY = 3.33f;
    static constexpr float CAMERA_SHAKE_DURATION = 1.33f;

    inline void record_twilight_event(float delta, const char* cause) {
        m_telemetry.record_event(delta, cause, m_sim_elapsed_sec);
    }
    void update_tick_simulation(float dt);
    void update_victory_condition(float raw_dt);
    void update_game_over_fade(float raw_dt);
    void update_time_dilation_hotkeys();
    void update_player_respawn();
    void update_twilight_metrics(float dt, float prev_twilight);
    void load_tiles_and_network(const Level& level);
    void setup_player_at_spawn(GridPos spawn_pos);
    void load_dark_towers(std::span<const DarkTowerSpawn> spawns);
    void reset_level_telemetry();
    void draw_tiles_and_network(std::vector<uint32_t>& pixel_buffer, float progress);
    void draw_victory_shockwave(float alpha);

public:
    void trigger_vignette_surge(float duration = TwilightOverlay::VIGNETTE_DURATION);
    void trigger_tower_spawn_alert(
        float vignette_duration = TwilightOverlay::VIGNETTE_DURATION,
        float shake_intensity = CAMERA_SHAKE_PEAK_INTENSITY,
        float shake_duration = CAMERA_SHAKE_DURATION
    );

    void update_headless_defense(float dt);
    void print_headless_summary_report(int64_t seed = -1);
    void set_headless(bool headless) noexcept { m_telemetry.set_headless(headless); }
    [[nodiscard]] bool is_headless() const noexcept { return m_telemetry.is_headless(); }

    void dump_telemetry_snapshot();
    Camera& camera() override { return m_camera; }
    const Camera& camera() const override { return m_camera; }

    explicit MainScene(int initial_level_id = 1) : m_current_level_id(initial_level_id) {}

    void init(SceneManager& sm) override;
    void load_level(int level_id);
    void update_camera_map_boundary();

    void update(SceneManager& sm, float dt) override;
    void sync_camera(float alpha) override;
    void draw_world(std::vector<uint32_t>& pixel_buffer, float alpha) override;
    void draw_screen(std::vector<uint32_t>& pixel_buffer, float alpha) override;

    Tiles& tiles() { return m_tiles; }
    const Tiles& tiles() const { return m_tiles; }
    Network& network() { return m_network; }
    const Network& network() const { return m_network; }
};

} // namespace alx
