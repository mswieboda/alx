#include "alx/MainSceneTelemetry.h"
#include <algorithm>
#include <cmath>
#include "alx/TelemetryDumper.h"
#include "alx/EnemyManager.h"
#include "alx/Network.h"
#include "alx/Player.h"
#include "alx/Tiles.h"

namespace alx {

#if ALX_ENABLE_TELEMETRY || ALX_ENABLE_HEADLESS

void MainSceneTelemetry::reset(float initial_twilight) {
    m_initial_twilight = initial_twilight;
    m_peak_twilight = initial_twilight;
    m_min_twilight = initial_twilight;
    m_sum_twilight = 0.0;
    m_rolling_sample_head = 0;
    m_rolling_sample_count = 0;
    m_last_event_delta = 0.0f;
    m_last_event_cause = "None";
    m_last_event_timestamp = 0.0f;
}

void MainSceneTelemetry::record_event(float delta, const char* cause, float sim_elapsed_sec) {
    m_last_event_delta = delta;
    m_last_event_cause = cause ? cause : "Unknown";
    m_last_event_timestamp = sim_elapsed_sec;
}

void MainSceneTelemetry::update_metrics(float dt, float current_twilight, float prev_twilight) {
    m_peak_twilight = std::max(m_peak_twilight, current_twilight);
    m_min_twilight = std::min(m_min_twilight, current_twilight);
    m_sum_twilight += current_twilight;

    const float frame_delta = current_twilight - prev_twilight;
    m_twilight_delta_per_sec = (dt > 0.0001f) ? (frame_delta / dt) : 0.0f;

#if ALX_ENABLE_TELEMETRY
    m_rolling_samples[m_rolling_sample_head] = RollingSample{ dt, frame_delta };
    m_rolling_sample_head = (m_rolling_sample_head + 1) % ROLLING_BUFFER_MAX_SAMPLES;
    if (m_rolling_sample_count < ROLLING_BUFFER_MAX_SAMPLES) {
        ++m_rolling_sample_count;
    }
#endif // ALX_ENABLE_TELEMETRY
}

float MainSceneTelemetry::calculate_rolling_rate(float duration_sec) const {
    if (m_rolling_sample_count == 0) return 0.0f;

    float total_dt = 0.0f;
    float total_delta = 0.0f;

    for (size_t i = 0; i < m_rolling_sample_count; ++i) {
        size_t idx = (m_rolling_sample_head + ROLLING_BUFFER_MAX_SAMPLES - 1 - i) % ROLLING_BUFFER_MAX_SAMPLES;
        const auto& sample = m_rolling_samples[idx];
        if (total_dt + sample.dt > duration_sec && total_dt > 0.0f) {
            break;
        }
        total_dt += sample.dt;
        total_delta += sample.delta;
    }

    if (total_dt <= 0.0001f) return 0.0f;
    return total_delta / total_dt;
}

#if ALX_ENABLE_TELEMETRY
void MainSceneTelemetry::update_telemetry_dump(
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
) {
    m_telemetry_dump_timer += raw_dt;
    if (m_telemetry_dump_timer < TELEMETRY_DUMP_INTERVAL) return;

    m_telemetry_dump_timer = 0.0f;

    TelemetrySnapshot snap;
    snap.ticks = sim_tick_count;
    snap.sim_time_sec = sim_elapsed_sec;
    snap.time_scale = time_scale;
    snap.paused = paused;
    snap.twilight_level = twilight_level;
    snap.twilight_delta_per_sec = m_twilight_delta_per_sec;
    snap.twilight_rolling_rate_per_sec = calculate_rolling_rate(ROLLING_WINDOW_SHORT_SEC);
    snap.twilight_rolling_rate_15s_per_sec = calculate_rolling_rate(ROLLING_WINDOW_LONG_SEC);
    snap.twilight_session_net_rate_per_sec = (sim_elapsed_sec > 0.001f) ? ((twilight_level - m_initial_twilight) / sim_elapsed_sec) : 0.0f;
    snap.last_twilight_event_delta = m_last_event_delta;
    snap.last_twilight_event_cause = m_last_event_cause;
    snap.seconds_since_last_event = std::max(0.0f, sim_elapsed_sec - m_last_event_timestamp);

    int towers = 0;
    for (const auto& s : enemy_manager.structures()) {
        if (s.type == StructureType::DarkTower && s.hp > 0) {
            ++towers;
        }
    }
    snap.dark_towers_count = towers;
    snap.shadow_eggs_count = static_cast<int>(enemy_manager.shadow_eggs().size());
    snap.enemies_count = static_cast<int>(enemy_manager.enemies().size());

    int spires = 0;
    int refiners = 0;
    int pipes = 0;
    int net_w = network.width();
    if (net_w > 0) {
        for (int idx : network.active_indices()) {
            int x = idx % net_w;
            int y = idx / net_w;
            const auto& fix = network.fixture(x, y);
            if (fix.root_offset_x == 0 && fix.root_offset_y == 0) {
                if (fix.type == FixtureType::Spire) ++spires;
                else if (fix.type == FixtureType::Refiner) ++refiners;
                else if (fix.type == FixtureType::Pipe) ++pipes;
            }
        }
    }
    snap.spires_count = spires;
    snap.refiners_count = refiners;
    snap.pipes_count = pipes;
    snap.spires_cleanse_rate_per_sec = (sim_tick_rate > 0.001f) ? (spires * twilight_decrease_per_mana / sim_tick_rate) : 0.0f;

    snap.player_hp = player.state.hp;
    snap.player_max_hp = player.state.max_hp;
    snap.player_alloy = player.cursed_alloy();

    TelemetryDumper::dump_snapshot(snap);
}
#endif // ALX_ENABLE_TELEMETRY

#if ALX_ENABLE_HEADLESS
void MainSceneTelemetry::update_headless_defense(
    float dt,
    GridPos player_spawn,
    const Tiles& tiles,
    EnemyManager& enemy_manager
) {
    m_headless_defend_timer += dt;
    if (m_headless_defend_timer >= HeadlessConstants::DEFEND_INTERVAL_SEC) {
        m_headless_defend_timer = 0.0f;
        const float base_x = player_spawn.to_world_x(tiles.tile_size());
        const float base_y = player_spawn.to_world_y(tiles.tile_size());
        enemy_manager.clear_enemies_near(base_x, base_y, HeadlessConstants::DEFEND_RADIUS_PX);
    }
}

void MainSceneTelemetry::print_headless_summary_report(
    int64_t seed,
    int64_t sim_tick_count,
    float sim_elapsed_sec,
    float current_twilight,
    float time_to_zero_twilight,
    const EnemyManager& enemy_manager,
    const Network& network,
    float sim_tick_rate,
    float twilight_decrease_per_mana
) {
    HeadlessSummaryStats stats;
    stats.total_ticks = sim_tick_count;
    stats.total_sim_time_sec = sim_elapsed_sec;
    stats.seed = seed;
    stats.initial_twilight = m_initial_twilight;
    stats.final_twilight = current_twilight;
    stats.peak_twilight = m_peak_twilight;
    stats.min_twilight = m_min_twilight;
    stats.avg_twilight = (sim_tick_count > 0) ? static_cast<float>(m_sum_twilight / static_cast<double>(sim_tick_count)) : current_twilight;
    stats.time_to_zero_twilight = time_to_zero_twilight;

    int towers = 0;
    for (const auto& s : enemy_manager.structures()) {
        if (s.type == StructureType::DarkTower && s.hp > 0) {
            ++towers;
        }
    }
    stats.dark_towers_count = towers;
    stats.shadow_eggs_count = static_cast<int>(enemy_manager.shadow_eggs().size());
    stats.enemies_count = static_cast<int>(enemy_manager.enemies().size());

    int spires = 0;
    int refiners = 0;
    int pipes = 0;
    int net_w = network.width();
    if (net_w > 0) {
        for (int idx : network.active_indices()) {
            int x = idx % net_w;
            int y = idx / net_w;
            const auto& fix = network.fixture(x, y);
            if (fix.root_offset_x == 0 && fix.root_offset_y == 0) {
                if (fix.type == FixtureType::Spire) ++spires;
                else if (fix.type == FixtureType::Refiner) ++refiners;
                else if (fix.type == FixtureType::Pipe) ++pipes;
            }
        }
    }
    stats.spires_count = spires;
    stats.refiners_count = refiners;
    stats.pipes_count = pipes;
    stats.spires_cleanse_rate_per_sec = (sim_tick_rate > 0.001f) ? (spires * twilight_decrease_per_mana / sim_tick_rate) : 0.0f;

    TelemetryDumper::print_summary_report(stats);
}
#endif // ALX_ENABLE_HEADLESS

#endif // ALX_ENABLE_TELEMETRY || ALX_ENABLE_HEADLESS

} // namespace alx
