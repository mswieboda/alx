#include "alx/MainScene.h"
#include <algorithm>
#include <cmath>
#include <string>
#include <utility>
#include "core/Audio.h"
#include "core/Draw.h"
#include "core/Input.h"
#include "assets/Fonts.h"
#include "assets/Images.h"
#include "assets/Music.h"
#include "Game.h"
#include "Debug.h"
#include "Action.h"
#include "Random.h"
#include "Layer.h"
#include "alx/SFX.h"
#include "alx/ParticleEmitters.h"
#include "alx/TelemetryDumper.h"

namespace alx {

void MainScene::init(SceneManager& sm) {
    background_color = 0xFF131313; // very dark gray

    if (m_twilight_pixel_buffer.size() != static_cast<size_t>(Game::WIDTH * Game::HEIGHT)) {
        m_twilight_pixel_buffer.resize(Game::WIDTH * Game::HEIGHT);
    }

    Draw::set_y_sort_mode(Draw::YSortMode::YPlusHeight);

    if (Audio::load_music_from_memory(Assets::Music::awm, Assets::Music::awm_len)) {
        Audio::set_music_volume(Audio::DEFAULT_MUSIC_VOLUME);
        Audio::play_music(true);
    }

    load_level(m_current_level_id);
}

void MainScene::load_level(int level_id) {
    const Level* lvl = Levels::get_level(level_id);
    if (!lvl) return;

    m_current_level_id = level_id;
    m_player_spawn = lvl->player_spawn;
    m_twilight_level = std::clamp(lvl->initial_twilight, 0.0f, TWILIGHT_MAX);

    m_tiles = Tiles(lvl->map_width, lvl->map_height);
    m_network = Network(lvl->map_width, lvl->map_height);

    setup_player_at_spawn(m_player_spawn);
    update_camera_map_boundary();
    load_tiles_and_network(*lvl);
    load_dark_towers(lvl->dark_tower_spawns);
    reset_level_telemetry();
}

void MainScene::setup_player_at_spawn(GridPos spawn_pos) {
    const float spawn_x_px = spawn_pos.to_world_x(m_tiles.tile_size());
    const float spawn_y_px = spawn_pos.to_world_y(m_tiles.tile_size());

#if ALX_ENABLE_HEADLESS
    const float spawn_x = m_is_headless ? HeadlessConstants::OFFSCREEN_PLAYER_POS : spawn_x_px;
    const float spawn_y = m_is_headless ? HeadlessConstants::OFFSCREEN_PLAYER_POS : spawn_y_px;
#else
    const float spawn_x = spawn_x_px;
    const float spawn_y = spawn_y_px;
#endif

    m_player = Player(spawn_x, spawn_y);
}

void MainScene::load_dark_towers(std::span<const DarkTowerSpawn> spawns) {
    std::vector<std::pair<int, int>> corrupted_tile_coords;
    corrupted_tile_coords.reserve(spawns.size());
    for (const auto& spawn : spawns) {
        corrupted_tile_coords.emplace_back(spawn.pos.x, spawn.pos.y);
    }

    m_enemy_manager.clear();
    m_enemy_manager.register_corrupted_tiles(corrupted_tile_coords, m_tiles);
}

void MainScene::reset_level_telemetry() {
    m_time_to_zero_twilight = -1.0f;
#if ALX_ENABLE_TELEMETRY || ALX_ENABLE_HEADLESS
    m_initial_twilight = m_twilight_level;
    m_peak_twilight = m_twilight_level;
    m_min_twilight = m_twilight_level;
    m_sum_twilight = 0.0;
    m_rolling_sample_head = 0;
    m_rolling_sample_count = 0;
    m_last_event_delta = 0.0f;
    m_last_event_cause = "None";
    m_last_event_timestamp = 0.0f;
#endif
}

void MainScene::load_tiles_and_network(const Level& level) {
    int width = m_tiles.width();
    int height = m_tiles.height();

    // 1. Static Floor & Void initialization
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            if (x == 0 || x == width - 1 || y == 0 || y == height - 1) {
                m_tiles.set_tile(x, y, TileType::Empty);
            } else {
                m_tiles.set_tile(x, y, TileType::Floor);
            }
        }
    }

    // 2. Explicit custom tile placements
    for (const auto& tile : level.custom_tiles) {
        m_tiles.set_tile(tile.pos, tile.type);
    }

    // 3. Clear & populate Network fixtures
    m_network.clear();

    for (const auto& placement : level.fixtures) {
        m_network.place_fixture(placement.pos, placement.type);
    }
}

void MainScene::update_camera_map_boundary() {
    int tile_size = m_tiles.tile_size();
    int bound_width = m_tiles.width() * tile_size;
    int bound_height = (m_tiles.height() * tile_size);

    m_camera.set_limits(0, 0, bound_width, bound_height);
}

#if ALX_ENABLE_TELEMETRY || ALX_ENABLE_HEADLESS
void MainScene::record_twilight_event(float delta, const char* cause) {
    m_last_event_delta = delta;
    m_last_event_cause = cause ? cause : "Unknown";
    m_last_event_timestamp = m_sim_elapsed_sec;
}

float MainScene::calculate_rolling_twilight_rate(float duration_sec) const {
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
#endif

#if ALX_ENABLE_TELEMETRY
void MainScene::dump_telemetry_snapshot() {
    TelemetrySnapshot snap;
    snap.ticks = m_sim_tick_count;
    snap.sim_time_sec = m_sim_elapsed_sec;
    snap.time_scale = m_time_scale;
    snap.paused = m_paused;
    snap.twilight_level = m_twilight_level;
    snap.twilight_delta_per_sec = m_twilight_delta_per_sec;
    snap.twilight_rolling_rate_per_sec = calculate_rolling_twilight_rate(ROLLING_WINDOW_SHORT_SEC);
    snap.twilight_rolling_rate_15s_per_sec = calculate_rolling_twilight_rate(ROLLING_WINDOW_LONG_SEC);
    snap.twilight_session_net_rate_per_sec = (m_sim_elapsed_sec > 0.001f) ? ((m_twilight_level - m_initial_twilight) / m_sim_elapsed_sec) : 0.0f;
    snap.last_twilight_event_delta = m_last_event_delta;
    snap.last_twilight_event_cause = m_last_event_cause;
    snap.seconds_since_last_event = std::max(0.0f, m_sim_elapsed_sec - m_last_event_timestamp);

    int towers = 0;
    for (const auto& s : m_enemy_manager.structures()) {
        if (s.type == StructureType::DarkTower && s.hp > 0) {
            ++towers;
        }
    }
    snap.dark_towers_count = towers;
    snap.shadow_eggs_count = static_cast<int>(m_enemy_manager.shadow_eggs().size());
    snap.enemies_count = static_cast<int>(m_enemy_manager.enemies().size());

    int spires = 0;
    int refiners = 0;
    int pipes = 0;
    int net_w = m_network.width();
    if (net_w > 0) {
        for (int idx : m_network.active_indices()) {
            int x = idx % net_w;
            int y = idx / net_w;
            const auto& fix = m_network.fixture(x, y);
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
    snap.spires_cleanse_rate_per_sec = (SIM_TICK_RATE > 0.001f) ? (spires * TWILIGHT_DECREASE_PER_MANA / SIM_TICK_RATE) : 0.0f;

    snap.player_hp = m_player.state.hp;
    snap.player_max_hp = m_player.state.max_hp;
    snap.player_alloy = m_player.cursed_alloy();

    TelemetryDumper::dump_snapshot(snap);
}
#endif

void MainScene::update(SceneManager& sm, float raw_dt) {
    if constexpr (Debug::CAN_PAUSE) {
        if (Action::is_just_pressed(Action::Menu) || Input::is_key_just_pressed(KeyCode::P)) {
            m_paused = !m_paused;
            if (m_paused) {
                Audio::pause_music();
            } else {
                Audio::resume_music();
            }
        }
    }
    if (m_paused) return;

    update_victory_condition(raw_dt);
    update_time_dilation_hotkeys();

    const float dt = raw_dt * m_time_scale;
    m_last_dt = dt;
    ++m_sim_tick_count;
    m_sim_elapsed_sec += dt;
    const float prev_twilight = m_twilight_level;

    update_tick_simulation(dt);
    m_camera.follow(m_player.center_x(1.0f), m_player.center_y(1.0f));
    m_camera.update(dt, m_player.facing_dx, m_player.facing_dy, m_player.input_buffer.was_moving);
    m_player.update(dt, m_tiles, m_network, m_camera, &m_enemy_manager.structures(), &m_particle_system);

#if ALX_ENABLE_HEADLESS
    if (m_is_headless) {
        update_headless_defense(raw_dt);
    }
#endif

    update_player_respawn();

    m_enemy_manager.update(dt, &m_player, m_tiles, m_network, &m_particle_system, m_twilight_level);
    if (m_enemy_manager.consume_tower_spawned_event()) {
        trigger_tower_spawn_alert();
        Audio::play_sfx(SFX::dark_tower_spawn());
    }

    if (m_vignette_surge_timer > 0.0f) {
        m_vignette_surge_timer = std::max(0.0f, m_vignette_surge_timer - dt);
    }

    m_particle_system.update(dt);
    update_sword_slash_trail();

    float tw_inc = m_enemy_manager.consume_pending_twilight_increase();
    if (tw_inc > 0.0f) {
        m_twilight_level = std::clamp(m_twilight_level + tw_inc, 0.0f, TWILIGHT_MAX);
        record_twilight_event(tw_inc, "Tower/Fixture Corruption");
        Audio::play_sfx(SFX::twilight_pulse());
    }

    update_twilight_metrics(dt, prev_twilight);

#if ALX_ENABLE_TELEMETRY
    m_telemetry_dump_timer += raw_dt;
    if (m_telemetry_dump_timer >= TELEMETRY_DUMP_INTERVAL) {
        m_telemetry_dump_timer = 0.0f;
        dump_telemetry_snapshot();
    }
#endif
}

void MainScene::update_victory_condition(float raw_dt) {
    if (m_twilight_level < VICTORY_TWILIGHT_THRESHOLD) {
        if (m_time_to_zero_twilight < 0.0f) {
            m_time_to_zero_twilight = m_sim_elapsed_sec;
        }
        m_victory_hold_timer += raw_dt;
        if (m_victory_hold_timer >= VICTORY_HOLD_DURATION_SEC) {
            if (!m_victory_achieved) {
                m_victory_achieved = true;
                m_paused = true;
                Audio::pause_music();
            }
        }
    } else if (!m_victory_achieved) {
        m_victory_hold_timer = 0.0f;
    }
}

void MainScene::update_time_dilation_hotkeys() {
    if constexpr (ALX_ENABLE_DEBUG) {
        if (Input::is_key_just_pressed(KeyCode::Key1)) {
            m_time_scale = 1.0f;
        } else if (Input::is_key_just_pressed(KeyCode::Key2)) {
            m_time_scale = 2.0f;
        } else if (Input::is_key_just_pressed(KeyCode::Key3)) {
            m_time_scale = 5.0f;
        } else if (Input::is_key_just_pressed(KeyCode::Key4)) {
            m_time_scale = 10.0f;
        }
    }
}

void MainScene::update_player_respawn() {
    if (m_player.state.defeated && m_player.state.defeat_timer <= 0.0f) {
        m_player.state.hp = m_player.state.max_hp;
        m_player.state.defeated = false;
        m_player.state.iframe_timer = Player::State::IFRAME_DURATION;

        setup_player_at_spawn(m_player_spawn);
        m_player.sync_prev_transforms();
    }
}

void MainScene::update_sword_slash_trail() {
    if (!m_player.is_attacking()) {
        m_slash_was_attacking = false;
        return;
    }

    const Collision::Circle hit_circle = m_player.attack_hit_circle(1.0f);
    const float pcx = m_player.center_x(1.0f);
    const float pcy = m_player.center_y(1.0f);

    float dir_x = hit_circle.cx - pcx;
    float dir_y = hit_circle.cy - pcy;
    const float len = std::sqrt(dir_x * dir_x + dir_y * dir_y);
    if (len > 0.001f) {
        dir_x /= len;
        dir_y /= len;
    } else {
        dir_x = 0.0f;
        dir_y = 1.0f;
    }

    const float curr_tip_x = hit_circle.cx + dir_x * (hit_circle.radius - 1.0f);
    const float curr_tip_y = hit_circle.cy + dir_y * (hit_circle.radius - 1.0f);

    if (!m_slash_was_attacking) {
        m_slash_prev_tip_x = curr_tip_x;
        m_slash_prev_tip_y = curr_tip_y;
        m_slash_was_attacking = true;
    }

    const int player_sort_y = static_cast<int>(m_player.transform.y + m_player.transform.height);
    ParticleEmitters::spawn_sword_slash_trail(
        m_particle_system, m_slash_prev_tip_x, m_slash_prev_tip_y, curr_tip_x, curr_tip_y,
        m_player.swing_progress_curr, Layer::WorldObjFX, Layer::WorldObj, player_sort_y
    );

    m_slash_prev_tip_x = curr_tip_x;
    m_slash_prev_tip_y = curr_tip_y;
}

void MainScene::update_twilight_metrics(float dt, float prev_twilight) {
    if constexpr (ALX_ENABLE_DEBUG) {
        if (Action::is_just_pressed(Action::DebugEnemyWave)) {
            m_enemy_manager.spawn_enemy_wave(m_tiles, &m_network, -1, m_player.center_x(1.0f), m_player.center_y(1.0f), false);
        }

        if (Action::is_pressed(Action::DebugTwUp)) {
            m_twilight_level += 1 * dt;
            m_twilight_level = std::clamp(m_twilight_level, 0.0f, TWILIGHT_MAX);
            record_twilight_event(1.0f * dt, "Debug TwUp");
        } else if (Action::is_pressed(Action::DebugTwDown)) {
            m_twilight_level -= 1 * dt;
            m_twilight_level = std::clamp(m_twilight_level, 0.0f, TWILIGHT_MAX);
            record_twilight_event(-1.0f * dt, "Debug TwDown");
        }
    }

#if ALX_ENABLE_TELEMETRY || ALX_ENABLE_HEADLESS
    m_peak_twilight = std::max(m_peak_twilight, m_twilight_level);
    m_min_twilight = std::min(m_min_twilight, m_twilight_level);
    m_sum_twilight += m_twilight_level;

    const float frame_delta = m_twilight_level - prev_twilight;
    m_twilight_delta_per_sec = (dt > 0.0001f) ? (frame_delta / dt) : 0.0f;
#endif

#if ALX_ENABLE_TELEMETRY
    m_rolling_samples[m_rolling_sample_head] = RollingSample{ dt, frame_delta };
    m_rolling_sample_head = (m_rolling_sample_head + 1) % ROLLING_BUFFER_MAX_SAMPLES;
    if (m_rolling_sample_count < ROLLING_BUFFER_MAX_SAMPLES) {
        ++m_rolling_sample_count;
    }
#endif
}

#if ALX_ENABLE_HEADLESS
void MainScene::update_headless_defense(float dt) {
    m_headless_defend_timer += dt;
    if (m_headless_defend_timer >= HeadlessConstants::DEFEND_INTERVAL_SEC) {
        m_headless_defend_timer = 0.0f;
        const float base_x = m_player_spawn.to_world_x(m_tiles.tile_size());
        const float base_y = m_player_spawn.to_world_y(m_tiles.tile_size());
        m_enemy_manager.clear_enemies_near(base_x, base_y, HeadlessConstants::DEFEND_RADIUS_PX);
    }
}

void MainScene::print_headless_summary_report(int64_t seed) {
    HeadlessSummaryStats stats;
    stats.total_ticks = m_sim_tick_count;
    stats.total_sim_time_sec = m_sim_elapsed_sec;
    stats.seed = seed;
    stats.initial_twilight = m_initial_twilight;
    stats.final_twilight = m_twilight_level;
    stats.peak_twilight = m_peak_twilight;
    stats.min_twilight = m_min_twilight;
    stats.avg_twilight = (m_sim_tick_count > 0) ? static_cast<float>(m_sum_twilight / static_cast<double>(m_sim_tick_count)) : m_twilight_level;
    stats.time_to_zero_twilight = m_time_to_zero_twilight;

    int towers = 0;
    for (const auto& s : m_enemy_manager.structures()) {
        if (s.type == StructureType::DarkTower && s.hp > 0) {
            ++towers;
        }
    }
    stats.dark_towers_count = towers;
    stats.shadow_eggs_count = static_cast<int>(m_enemy_manager.shadow_eggs().size());
    stats.enemies_count = static_cast<int>(m_enemy_manager.enemies().size());

    int spires = 0;
    int refiners = 0;
    int pipes = 0;
    int net_w = m_network.width();
    if (net_w > 0) {
        for (int idx : m_network.active_indices()) {
            int x = idx % net_w;
            int y = idx / net_w;
            const auto& fix = m_network.fixture(x, y);
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
    stats.spires_cleanse_rate_per_sec = (SIM_TICK_RATE > 0.001f) ? (spires * TWILIGHT_DECREASE_PER_MANA / SIM_TICK_RATE) : 0.0f;

    TelemetryDumper::print_summary_report(stats);
}
#endif

void MainScene::update_tick_simulation(float dt) {
    if (m_paused) return;

    m_sim_timer += dt;

    if (m_sim_timer >= SIM_TICK_RATE) {
        m_sim_timer = 0.0f;

        NetworkSimResults sim_res = m_network.sim_tick();
        if (sim_res.spires_converted > 0) {
            float dec = TWILIGHT_DECREASE_PER_MANA * sim_res.spires_converted;
            m_twilight_level -= dec;
            m_twilight_level = std::clamp(m_twilight_level, 0.0f, TWILIGHT_MAX);
            record_twilight_event(-dec, "Spire Cleanse");
            Audio::play_sfx(SFX::spire_burn());
        }
        if (sim_res.refiners_processed > 0) {
            Audio::play_sfx(SFX::mana_converted());
        }

        // Active dark mana refiner pool gurgling sound with dynamic pitch variation
        bool has_active_refiners = false;
        for (const auto& fix : m_network.fixtures()) {
            if (fix.type == FixtureType::Refiner && fix.mana_state == ManaState::Dark && fix.is_root()) {
                has_active_refiners = true;
                break;
            }
        }
        if (has_active_refiners) {
            Audio::play_sfx(SFX::refiner_bubble());
        }
    }
}

void MainScene::sync_camera(float alpha) {
    m_camera.sync_render_position(m_player.center_x(alpha), m_player.center_y(alpha));
}

void MainScene::draw_world(std::vector<uint32_t>& pixel_buffer, float alpha) {
    float sub_tick_progress = std::clamp(m_sim_timer / SIM_TICK_RATE, 0.0f, 1.0f);

    draw_tiles_and_network(pixel_buffer, sub_tick_progress);
    m_enemy_manager.draw_corrupted_tiles(m_tiles.tile_size());
    m_enemy_manager.draw_enemies(pixel_buffer, alpha);
    m_player.draw(pixel_buffer, alpha, &m_tiles, &m_network);
    m_particle_system.draw(&m_camera);
}

void MainScene::trigger_vignette_surge(float duration) {
    m_vignette_surge_timer = duration;
}

void MainScene::trigger_tower_spawn_alert(float vignette_duration, float shake_intensity, float shake_duration) {
    trigger_vignette_surge(vignette_duration);
    m_camera.shake(shake_intensity, shake_duration);
}

void MainScene::draw_screen(std::vector<uint32_t>& pixel_buffer, float alpha) {
    draw_twilight(pixel_buffer, alpha);
    draw_vignette_surge();
    draw_hud();
    m_enemy_manager.draw_threat_indicators(m_camera);
}

void MainScene::draw_vignette_surge() {
    if (m_vignette_surge_timer <= 0.0f) return;

    float progress = std::clamp(1.0f - (m_vignette_surge_timer / VIGNETTE_SURGE_DURATION), 0.0f, 1.0f);
    constexpr float PI = 3.14159265358979323846f;
    float pulse_t = std::sin(progress * PI);
    float current_intensity = pulse_t * VIGNETTE_PEAK_INTENSITY;

    Draw::vignette(current_intensity, VIGNETTE_COLOR, VIGNETTE_INNER_RADIUS, VIGNETTE_OUTER_RADIUS, Layer::WorldOverlay);
}

void MainScene::draw_twilight(std::vector<uint32_t>& pixel_buffer, float alpha) {
    if (m_twilight_level <= 0.0f) return;

    int w = Game::WIDTH;
    int h = Game::HEIGHT;
    uint32_t twilight_rgb = 0x00130C1A;

    float max_alpha_float = m_twilight_level * 255.0f;
    uint8_t base_alpha = static_cast<uint8_t>(max_alpha_float);
    uint32_t twilight_color = (static_cast<uint32_t>(base_alpha) << 24) | twilight_rgb;

    std::fill(m_twilight_pixel_buffer.begin(), m_twilight_pixel_buffer.end(), twilight_color);

    int player_screen_x = m_camera.to_screen_x(m_player.center_x(alpha));
    int player_screen_y = m_camera.to_screen_y(m_player.center_y(alpha));
    int radius = static_cast<int>(std::round(m_player.wand_radius));
    int radius_sq = radius * radius;
    float inv_radius_sq = 1.0f / static_cast<float>(radius_sq);

    int min_x = std::clamp(player_screen_x - radius, 0, w);
    int max_x = std::clamp(player_screen_x + radius + 1, 0, w);
    int min_y = std::clamp(player_screen_y - radius, 0, h);
    int max_y = std::clamp(player_screen_y + radius + 1, 0, h);

    for (int y = min_y; y < max_y; ++y) {
        int dy = y - player_screen_y;
        int dy_sq = dy * dy;
        int row_offset = y * w;

        for (int x = min_x; x < max_x; ++x) {
            int dx = x - player_screen_x;
            int dist_sq = dx * dx + dy_sq;

            if (dist_sq < radius_sq) {
                float factor = static_cast<float>(dist_sq) * inv_radius_sq;
                uint8_t alpha_byte = static_cast<uint8_t>(max_alpha_float * factor);
                int idx = row_offset + x;

                m_twilight_pixel_buffer[idx] = (static_cast<uint32_t>(alpha_byte) << 24) | twilight_rgb;
            }
        }
    }

    Draw::blend_pixels(
        0, 0,
        m_twilight_pixel_buffer.data(),
        static_cast<uint32_t>(m_twilight_pixel_buffer.size() * sizeof(uint32_t)),
        w, h,
        Layer::WorldOverlay
    );
}

void MainScene::draw_hud() {
    int screen_width = Game::WIDTH;

    FixtureType sel_type = m_player.selected_fixture_type();
    const char* selected_name = fixture_glyph(sel_type);
    int cost = Player::fixture_cost(sel_type);

    const uint32_t text_color = 0xFF00CCCC;
    const uint32_t shadow_color = 0xFF003344;
    const FontData& font = Assets::Fonts::fant_8;
    const int line_h_padding = 4;
    int ly = line_h_padding;

    // \x03 = Heart icon, \x04 = Gem/Alloy icon
    Draw::text_shadow(
        6, ly,
        Draw::fmt("\x03 %d \x04 %d", m_player.state.hp, m_player.cursed_alloy()),
        text_color, shadow_color, 1, Layer::HUD_Text, &font
    );

    // \x0F = Twilight starburst icon, \x08 = Cleanse icon
    int twilight_pct = static_cast<int>(m_twilight_level * 100.0f);
    const char* icon = m_twilight_level >= 0.5f ? "\x08" : "\x0F";
    std::string_view twilight_str = Draw::fmt("%s %d%%", icon, twilight_pct);
    int twilight_width = Draw::text_width(twilight_str, 1, &font);
    Draw::text_shadow(
        screen_width / 2 - twilight_width / 2, ly,
        twilight_str,
        text_color, shadow_color, 1, Layer::HUD_Text, &font
    );

    // selected build fixture
    std::string_view build_str = Draw::fmt("build: %s (%d)", selected_name, cost);
    int build_width = Draw::text_width(build_str, 1, &font);
    Draw::text_shadow(
        screen_width - 6 - build_width, ly,
        build_str,
        text_color, shadow_color, 1, Layer::HUD_Text, &font
    );

    ly += font.size + line_h_padding;

    if constexpr (Debug::SHOW_SEED) {
        std::string_view seed_str = Draw::fmt(Random::is_custom_seeded() ? "seed: %u (custom)" : "seed: %u", Random::active_seed());
        int bottom_y = Game::HEIGHT - font.size - line_h_padding;
        Draw::text(
            6, bottom_y,
            seed_str,
            text_color, 1, Layer::HUD_Text, &font
        );
    }

    draw_victory_and_pause_overlays(screen_width, Game::HEIGHT, font);
}

void MainScene::draw_victory_and_pause_overlays(int screen_width, int screen_height, const FontData& font) {
    if (m_victory_achieved) {
        std::string_view win_str = "YOU WIN!";
        int win_w = Draw::text_width(win_str, 2, &font);
        Draw::text(
            screen_width / 2 - win_w / 2,
            screen_height / 2 - font.size,
            win_str,
            COLOR_VICTORY_TEXT, 2, Layer::HUD_Text, &font
        );
    } else if (m_paused) {
        std::string_view pause_str = "PAUSED";
        int pause_w = Draw::text_width(pause_str, 2, &font);
        Draw::text(
            screen_width / 2 - pause_w / 2,
            screen_height / 2 - font.size,
            pause_str,
            COLOR_PAUSE_TEXT, 2, Layer::HUD_Text, &font
        );
    } else if (m_twilight_level < VICTORY_TWILIGHT_THRESHOLD) {
        int remaining_sec = std::max(0, static_cast<int>(std::ceil(VICTORY_HOLD_DURATION_SEC - m_victory_hold_timer)));
        std::string_view count_str = Draw::fmt("TWILIGHT CLEARED IN: %ds", remaining_sec);
        int count_w = Draw::text_width(count_str, 1, &font);
        Draw::text(
            screen_width / 2 - count_w / 2,
            screen_height / 2 - font.size / 2,
            count_str,
            COLOR_VICTORY_TEXT, 1, Layer::HUD_Text, &font
        );
    }
}

bool MainScene::is_connectable_fixture(int gx, int gy) const {
    if (!m_network.in_bounds(gx, gy)) return false;
    FixtureType t = m_network.fixture(gx, gy).type;
    return t == FixtureType::Pipe || t == FixtureType::Seep || t == FixtureType::Refiner || t == FixtureType::Spire;
}



bool MainScene::connects_dark_mana(int gx, int gy) const {
    if (!m_network.in_bounds(gx, gy)) return false;
    const Fixture& f = m_network.fixture(gx, gy);
    return (f.type == FixtureType::Pipe || f.type == FixtureType::Seep || f.type == FixtureType::Refiner) && f.mana_state == ManaState::Dark;
}

bool MainScene::is_node_fixture(int gx, int gy) const {
    if (!m_network.in_bounds(gx, gy)) return false;
    FixtureType t = m_network.fixture(gx, gy).type;
    return t == FixtureType::Seep || t == FixtureType::Refiner || t == FixtureType::Spire;
}

void MainScene::draw_tiles_and_network(std::vector<uint32_t>& pixel_buffer, float progress) {
    int base_tile_size = m_tiles.tile_size();
    int min_tx = 0, max_tx = 0, min_ty = 0, max_ty = 0;
    m_camera.get_tile_bounds(
        m_tiles.width(), m_tiles.height(), base_tile_size,
        min_tx, max_tx, min_ty, max_ty
    );

    // Layer 0: Render Terrain Floor & Wall tiles
    for (int y = min_ty; y <= max_ty; ++y) {
        for (int x = min_tx; x <= max_tx; ++x) {
            Tile tile = m_tiles.tile(x, y);
            int world_x = x * base_tile_size;
            int world_y = y * base_tile_size;

            draw_terrain_tile(tile, world_x, world_y, base_tile_size);
        }
    }

    m_network.draw(
        min_tx, max_tx, min_ty, max_ty,
        m_player.transform, progress,
        m_paused ? nullptr : &m_particle_system, m_last_dt, SIM_TICK_RATE
    );
}

void MainScene::draw_terrain_tile(const Tile& tile, int world_x, int world_y, int tile_size) {
    if (tile.type == TileType::Empty) {
        return;
    }

    size_t frame_index = 0;
    switch (tile.type) {
        case TileType::Floor:  frame_index = 0; break;
        case TileType::Water:  frame_index = 2; break;
        case TileType::Stone:  frame_index = 3; break;
        case TileType::Dirt:   frame_index = 4; break;
        default:               frame_index = 0; break;
    }

    if (frame_index < 5) {
        const auto& frame = Assets::Images::tileset_frames[frame_index];
        Draw::sprite(
            static_cast<float>(world_x),
            static_cast<float>(world_y),
            Assets::Images::tileset + frame.offset,
            static_cast<uint32_t>(frame.len),
            static_cast<float>(tile_size),
            static_cast<float>(tile_size),
            Layer::Ground
        );
    }
}

} // namespace alx
