#pragma once

#include <string>
#include <cstdint>

namespace alx {

struct TelemetrySnapshot {
    int64_t ticks{0};
    float sim_time_sec{0.0f};
    float time_scale{1.0f};
    bool paused{false};
    float twilight_level{0.0f};
    float twilight_delta_per_sec{0.0f};
    int dark_towers_count{0};
    int shadow_eggs_count{0};
    int enemies_count{0};
    int spires_count{0};
    int refiners_count{0};
    int pipes_count{0};
    float spires_cleanse_rate_per_sec{0.0f};
    int player_hp{0};
    int player_max_hp{0};
    int player_alloy{0};
};

struct HeadlessSummaryStats {
    int64_t total_ticks{0};
    float total_sim_time_sec{0.0f};
    int64_t seed{-1};
    float initial_twilight{0.0f};
    float final_twilight{0.0f};
    float peak_twilight{0.0f};
    float min_twilight{1.0f};
    float avg_twilight{0.0f};
    float time_to_max_twilight{-1.0f};
    int dark_towers_count{0};
    int shadow_eggs_count{0};
    int enemies_count{0};
    int spires_count{0};
    int refiners_count{0};
    int pipes_count{0};
    float spires_cleanse_rate_per_sec{0.0f};
};

class TelemetryDumper {
public:
    static constexpr const char* TELEMETRY_FILE_PATH = "/tmp/alx_telemetry.json";
    static constexpr const char* TELEMETRY_TMP_PATH  = "/tmp/alx_telemetry.tmp";

    static bool dump_snapshot(const TelemetrySnapshot& snapshot);
    static void print_summary_report(const HeadlessSummaryStats& stats);
};

} // namespace alx
