#pragma once

#include <cstdint>
#include <string>
#include "alx/GridPos.h"

namespace alx {

class EnemyManager;
class Network;
class Player;
class Tiles;

#if ALX_ENABLE_TELEMETRY || ALX_ENABLE_HEADLESS

class MainSceneTelemetry {
private:
#if ALX_ENABLE_TELEMETRY
    static constexpr float ROLLING_WINDOW_SHORT_SEC = 3.0f;
    static constexpr float ROLLING_WINDOW_LONG_SEC = 15.0f;
    static constexpr size_t ROLLING_BUFFER_MAX_SAMPLES = 960; // 16s @ 60fps
    static constexpr float TELEMETRY_DUMP_INTERVAL = 0.1f;

    struct RollingSample {
        float dt{0.0f};
        float delta{0.0f};
    };
    RollingSample m_rolling_samples[ROLLING_BUFFER_MAX_SAMPLES]{};
    size_t m_rolling_sample_head{0};
    size_t m_rolling_sample_count{0};

    float m_twilight_delta_per_sec{0.0f};
    float m_telemetry_dump_timer{0.0f};
#endif // ALX_ENABLE_TELEMETRY

    // Last Event Tracking
    float m_last_event_delta{0.0f};
    std::string m_last_event_cause{"None"};
    float m_last_event_timestamp{0.0f};

    // Summary Statistics Tracking
    float m_initial_twilight{0.99f};
    float m_peak_twilight{0.0f};
    float m_min_twilight{1.0f};
    double m_sum_twilight{0.0};

#if ALX_ENABLE_HEADLESS
    bool m_is_headless{false};
    float m_headless_defend_timer{0.0f};

    struct HeadlessConstants {
        static constexpr float DEFEND_INTERVAL_SEC = 7.0f;
        static constexpr float DEFEND_RADIUS_PX = 96.0f;
    };
#endif

public:
    static constexpr float OFFSCREEN_PLAYER_POS = -100.0f;

    void reset(float initial_twilight);
    void record_event(float delta, const char* cause, float sim_elapsed_sec);
    void update_metrics(float dt, float current_twilight, float prev_twilight);

#if ALX_ENABLE_TELEMETRY
    [[nodiscard]] float calculate_rolling_rate(float duration_sec = ROLLING_WINDOW_SHORT_SEC) const;
#else
    [[nodiscard]] constexpr float calculate_rolling_rate(float = 3.0f) const { return 0.0f; }
#endif // ALX_ENABLE_TELEMETRY

#if ALX_ENABLE_TELEMETRY
    void update_telemetry_dump(
        float raw_dt,
        int64_t sim_tick_count,
        float sim_elapsed_sec,
        float time_scale,
        bool paused,
        float twilight_level,
        const EnemyManager& enemy_manager,
        const Network& network,
        const Player& player,
        float sim_tick_rate,
        float twilight_decrease_per_mana
    );
#else
    inline void update_telemetry_dump(float, int64_t, float, float, bool, float, const EnemyManager&, const Network&, const Player&, float, float) {}
#endif

#if ALX_ENABLE_HEADLESS
    void set_headless(bool headless) noexcept { m_is_headless = headless; }
    [[nodiscard]] bool is_headless() const noexcept { return m_is_headless; }
    void update_headless_defense(float dt, GridPos player_spawn, const Tiles& tiles, EnemyManager& enemy_manager);
    void print_headless_summary_report(
        int64_t seed,
        int64_t sim_tick_count,
        float sim_elapsed_sec,
        float current_twilight,
        float time_to_zero_twilight,
        const EnemyManager& enemy_manager,
        const Network& network,
        float sim_tick_rate,
        float twilight_decrease_per_mana
    );
#else
    inline void set_headless(bool) noexcept {}
    [[nodiscard]] constexpr bool is_headless() const noexcept { return false; }
    inline void update_headless_defense(float, GridPos, const Tiles&, EnemyManager&) {}
    inline void print_headless_summary_report(int64_t, int64_t, float, float, float, const EnemyManager&, const Network&, float, float) {}
#endif
};

#else

// Zero-cost no-op stub for release builds
class MainSceneTelemetry {
public:
    static constexpr float OFFSCREEN_PLAYER_POS = -100.0f;

    inline void reset(float) {}
    inline void record_event(float, const char*, float) {}
    inline void update_metrics(float, float, float) {}
    [[nodiscard]] constexpr float calculate_rolling_rate(float = 3.0f) const { return 0.0f; }

    inline void update_telemetry_dump(float, int64_t, float, float, bool, float, const EnemyManager&, const Network&, const Player&, float, float) {}

    inline void set_headless(bool) noexcept {}
    [[nodiscard]] constexpr bool is_headless() const noexcept { return false; }
    inline void update_headless_defense(float, GridPos, const Tiles&, EnemyManager&) {}
    inline void print_headless_summary_report(int64_t, int64_t, float, float, float, const EnemyManager&, const Network&, float, float) {}
};

#endif // ALX_ENABLE_TELEMETRY || ALX_ENABLE_HEADLESS

} // namespace alx
