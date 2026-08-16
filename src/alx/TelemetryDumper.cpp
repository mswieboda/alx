#include "alx/TelemetryDumper.h"
#include <cstdio>
#include <cmath>

#if ALX_ENABLE_TELEMETRY
#include "core/Log.h"
#include <filesystem>
#endif // ALX_ENABLE_TELEMETRY

namespace alx {

#if ALX_ENABLE_TELEMETRY
bool TelemetryDumper::dump_snapshot(const TelemetrySnapshot& snapshot) {
    FILE* file = std::fopen(TELEMETRY_TMP_PATH, "w");
    if (!file) {
        return false;
    }

    int twilight_pct = static_cast<int>(std::round(snapshot.twilight_level * 100.0f));

    std::fprintf(file,
        "{\n"
        "  \"ticks\": %lld,\n"
        "  \"sim_time_sec\": %.2f,\n"
        "  \"time_scale\": %.2f,\n"
        "  \"paused\": %s,\n"
        "  \"twilight_level\": %.4f,\n"
        "  \"twilight_pct\": %d,\n"
        "  \"twilight_delta_per_sec\": %.4f,\n"
        "  \"twilight_rolling_rate_per_sec\": %.4f,\n"
        "  \"twilight_rolling_rate_15s_per_sec\": %.4f,\n"
        "  \"twilight_session_net_rate_per_sec\": %.4f,\n"
        "  \"last_twilight_event_delta\": %.4f,\n"
        "  \"last_twilight_event_cause\": \"%s\",\n"
        "  \"seconds_since_last_event\": %.2f,\n"
        "  \"dark_towers_count\": %d,\n"
        "  \"shadow_eggs_count\": %d,\n"
        "  \"enemies_count\": %d,\n"
        "  \"spires_count\": %d,\n"
        "  \"refiners_count\": %d,\n"
        "  \"pipes_count\": %d,\n"
        "  \"spires_cleanse_rate_per_sec\": %.4f,\n"
        "  \"player_hp\": %d,\n"
        "  \"player_max_hp\": %d,\n"
        "  \"player_alloy\": %d\n"
        "}\n",
        static_cast<long long>(snapshot.ticks),
        snapshot.sim_time_sec,
        snapshot.time_scale,
        snapshot.paused ? "true" : "false",
        snapshot.twilight_level,
        twilight_pct,
        snapshot.twilight_delta_per_sec,
        snapshot.twilight_rolling_rate_per_sec,
        snapshot.twilight_rolling_rate_15s_per_sec,
        snapshot.twilight_session_net_rate_per_sec,
        snapshot.last_twilight_event_delta,
        snapshot.last_twilight_event_cause.c_str(),
        snapshot.seconds_since_last_event,
        snapshot.dark_towers_count,
        snapshot.shadow_eggs_count,
        snapshot.enemies_count,
        snapshot.spires_count,
        snapshot.refiners_count,
        snapshot.pipes_count,
        snapshot.spires_cleanse_rate_per_sec,
        snapshot.player_hp,
        snapshot.player_max_hp,
        snapshot.player_alloy
    );

    std::fclose(file);

    std::error_code ec;
    std::filesystem::rename(TELEMETRY_TMP_PATH, TELEMETRY_FILE_PATH, ec);
    if (ec) {
        // Fallback copy/remove if rename across filesystems fails
        std::filesystem::copy_file(TELEMETRY_TMP_PATH, TELEMETRY_FILE_PATH, std::filesystem::copy_options::overwrite_existing, ec);
        std::filesystem::remove(TELEMETRY_TMP_PATH, ec);
    }

    return true;
}
#endif // ALX_ENABLE_TELEMETRY

#if ALX_ENABLE_TELEMETRY || ALX_ENABLE_HEADLESS
void TelemetryDumper::print_summary_report(const HeadlessSummaryStats& stats) {
    int init_pct = static_cast<int>(std::round(stats.initial_twilight * 100.0f));
    int final_pct = static_cast<int>(std::round(stats.final_twilight * 100.0f));
    int peak_pct = static_cast<int>(std::round(stats.peak_twilight * 100.0f));
    int min_pct = static_cast<int>(std::round(stats.min_twilight * 100.0f));
    int avg_pct = static_cast<int>(std::round(stats.avg_twilight * 100.0f));

    char time_zero_buf[64];
    if (stats.time_to_zero_twilight >= 0.0f) {
        std::snprintf(time_zero_buf, sizeof(time_zero_buf), "%.2fs", stats.time_to_zero_twilight);
    } else {
        std::snprintf(time_zero_buf, sizeof(time_zero_buf), "N/A (Never reached)");
    }

    std::printf(
        "\n"
        "==================================================\n"
        "        HEADLESS SIMULATION SUMMARY REPORT        \n"
        "==================================================\n"
        "  Ticks Simulated:     %lld (%.2fs)\n"
        "  Random Seed:         %lld\n"
        "  ------------------------------------------------\n"
        "  TWILIGHT METRICS:\n"
        "    Initial Twilight:  %3d%% (%.4f)\n"
        "    Final Twilight:    %3d%% (%.4f)\n"
        "    Peak Twilight:     %3d%% (%.4f)\n"
        "    Min Twilight:      %3d%% (%.4f)\n"
        "    Average Twilight:  %3d%% (%.4f)\n"
        "    Time to 0%%:        %s\n"
        "  ------------------------------------------------\n"
        "  THREAT & INFRASTRUCTURE SUMMARY:\n"
        "    Active DarkTowers: %d\n"
        "    Shadow Eggs:       %d\n"
        "    Active Enemies:    %d\n"
        "    Light Spires:      %d\n"
        "    Refiners:          %d\n"
        "    Pipes:             %d\n"
        "    Total Cleanse Rate:%.4f/sec\n"
        "==================================================\n\n",
        static_cast<long long>(stats.total_ticks),
        stats.total_sim_time_sec,
        static_cast<long long>(stats.seed),
        init_pct, stats.initial_twilight,
        final_pct, stats.final_twilight,
        peak_pct, stats.peak_twilight,
        min_pct, stats.min_twilight,
        avg_pct, stats.avg_twilight,
        time_zero_buf,
        stats.dark_towers_count,
        stats.shadow_eggs_count,
        stats.enemies_count,
        stats.spires_count,
        stats.refiners_count,
        stats.pipes_count,
        stats.spires_cleanse_rate_per_sec
    );
}
#endif // ALX_ENABLE_TELEMETRY || ALX_ENABLE_HEADLESS

} // namespace alx
