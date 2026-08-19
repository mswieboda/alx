#include "alx/MainScene.h"
#include <algorithm>
#include <cmath>
#include <string>
#include <utility>
#include "core/Audio.h"
#include "core/Draw.h"
#include "core/Input.h"
#include "core/Log.h"
#include "core/SceneManager.h"
#include "assets/Music.h"
#include "Game.h"
#include "Debug.h"
#include "Action.h"
#include "Layer.h"
#include "alx/SFX.h"
#include "alx/StartScene.h"

namespace alx {

void MainScene::init(SceneManager& sm) {
    background_color = 0xFF131313; // very dark gray

    m_twilight_overlay.init(Game::WIDTH, Game::HEIGHT);

    Draw::set_y_sort_mode(Draw::YSortMode::YPlusHeight);

    if (Audio::load_music_from_memory(Assets::Music::twilight_bg, Assets::Music::twilight_bg_len)) {
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
    m_twilight_level = std::clamp(lvl->initial_twilight, TWILIGHT_MIN, TWILIGHT_MAX);
    m_can_build = lvl->can_build;
    m_prompt_overlay.reset();
    m_prompt_overlay.reset_room_cooldowns();
    m_context_sensor.reset();
    m_victory_hold_timer = 0.0f;
    m_victory_sequence_timer = 0.0f;
    m_last_countdown_second = -1;
    m_victory_achieved = false;
    m_is_victory_screen = false;

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
    const float spawn_x = m_telemetry.is_headless() ? MainSceneTelemetry::OFFSCREEN_PLAYER_POS : spawn_x_px;
    const float spawn_y = m_telemetry.is_headless() ? MainSceneTelemetry::OFFSCREEN_PLAYER_POS : spawn_y_px;
#else
    const float spawn_x = spawn_x_px;
    const float spawn_y = spawn_y_px;
#endif // ALX_ENABLE_HEADLESS

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
    m_telemetry.reset(m_twilight_level);
    m_momentum_tracker.reset(m_twilight_level);
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

void MainScene::dump_telemetry_snapshot() {
    m_telemetry.update_telemetry_dump(
        0.0f,
        m_sim_tick_count,
        m_sim_elapsed_sec,
        m_time_scale,
        m_paused,
        m_twilight_level,
        m_enemy_manager,
        m_network,
        m_player,
        SIM_TICK_RATE,
        TWILIGHT_DECREASE_PER_MANA
    );
}

void MainScene::update(SceneManager& sm, float raw_dt) {
    if (m_is_victory_screen) {
        m_victory_menu.update_navigation();
        if (m_victory_menu.is_confirmed()) {
            switch (m_victory_menu.selected_item<VictoryMenuItem>()) {
                case VictoryMenuItem::PlayAgain: {
                    auto scene_ptr = std::make_unique<alx::MainScene>(1);
                    sm.change_scene(std::move(scene_ptr));
                    return;
                }
                case VictoryMenuItem::MainMenu: {
                    auto scene_ptr = std::make_unique<alx::StartScene>();
                    sm.change_scene(std::move(scene_ptr));
                    return;
                }
                case VictoryMenuItem::Count:
                    break;
            }
        }
        return;
    }

    if (m_is_game_over && m_game_over_fade_timer <= 0.0f) {
        m_game_over_menu.update_navigation();
        if (m_game_over_menu.is_confirmed()) {
            switch (m_game_over_menu.selected_item<GameOverItem>()) {
                case GameOverItem::Retry: {
                    auto scene_ptr = std::make_unique<alx::MainScene>(m_current_level_id);
                    sm.change_scene(std::move(scene_ptr));
                    return;
                }
                case GameOverItem::Quit: {
                    auto scene_ptr = std::make_unique<alx::StartScene>();
                    sm.change_scene(std::move(scene_ptr));
                    return;
                }
                case GameOverItem::Count:
                    break;
            }
        }
        return;
    }

    if constexpr (Debug::QUIT_ON_ESC) {
        if (Input::is_key_just_pressed(KeyCode::Escape)) {
            sm.m_is_quit = true;
            return;
        }
    }

    if (m_paused) {
        if (Action::is_just_pressed(Action::Cancel) || Action::is_just_pressed(Action::Menu)) {
            m_paused = false;
            Audio::resume_music();
            return;
        }

        m_pause_menu.update_navigation();

        if (m_pause_menu.is_confirmed()) {
            switch (m_pause_menu.selected_item<PauseMenuItem>()) {
                case PauseMenuItem::Resume:
                    m_paused = false;
                    Audio::resume_music();
                    return;
                case PauseMenuItem::Retry: {
                    auto scene_ptr = std::make_unique<alx::MainScene>(m_current_level_id);
                    sm.change_scene(std::move(scene_ptr));
                    return;
                }
                case PauseMenuItem::MainMenu: {
                    auto scene_ptr = std::make_unique<alx::StartScene>();
                    sm.change_scene(std::move(scene_ptr));
                    return;
                }
                case PauseMenuItem::Count:
                    break;
            }
        }
        return;
    }

    if (Action::is_just_pressed(Action::Menu)) {
        m_paused = true;
        m_pause_menu.set_selected_item(PauseMenuItem::Resume);
        Audio::pause_music();
        return;
    }

    update_victory_condition(raw_dt);
    update_game_over_fade(raw_dt);
    update_time_dilation_hotkeys();
    m_prompt_overlay.update(raw_dt);

    if (m_victory_achieved) {
        if (m_victory_sequence_timer > 0.0f) {
            m_victory_sequence_timer = std::max(0.0f, m_victory_sequence_timer - raw_dt);
            if (m_victory_sequence_timer <= 0.0f) {
                if (Levels::has_level(m_current_level_id + 1)) {
                    auto scene_ptr = std::make_unique<alx::MainScene>(m_current_level_id + 1);
                    sm.change_scene(std::move(scene_ptr));
                    return;
                } else {
                    m_is_victory_screen = true;
                    Audio::pause_music();
                    return;
                }
            }
        }
    }

    const float dt = raw_dt * m_time_scale;
    m_last_dt = dt;
    ++m_sim_tick_count;
    m_sim_elapsed_sec += dt;
    const float prev_twilight = m_twilight_level;

    m_camera.follow(m_player.center_x(1.0f), m_player.center_y(1.0f));
    m_camera.update(dt, m_player.facing_dx, m_player.facing_dy, m_player.input_buffer.was_moving);
    m_player.update(dt, m_tiles, m_network, m_camera, m_can_build, &m_enemy_manager.structures(), &m_particle_system, &m_prompt_overlay);
    m_context_sensor.update(dt, m_prompt_overlay, m_player, m_network, m_enemy_manager, m_tiles, m_can_build);

    if (!m_victory_achieved) {
        update_tick_simulation(dt);

#if ALX_ENABLE_HEADLESS
        if (m_telemetry.is_headless()) {
            update_headless_defense(raw_dt);
        }
#endif // ALX_ENABLE_HEADLESS

        const bool is_frenzy = (m_victory_hold_timer > 0.0f) && !m_victory_achieved;
        const float frenzy_multiplier = is_frenzy ? FrenzyConstants::get_frenzy_multiplier(m_current_level_id) : FrenzyConstants::DEFAULT_FREQ_MULTIPLIER;

        m_enemy_manager.update(dt, &m_player, m_tiles, m_network, &m_particle_system, m_twilight_level, is_frenzy, frenzy_multiplier);
        if (m_enemy_manager.consume_tower_spawned_event()) {
            trigger_tower_spawn_alert();
            Audio::play_sfx(SFX::dark_tower_spawn());
            m_prompt_overlay.try_show_cooldown(
                "ALERT: Dark Tower emerged!",
                PromptType::alert,
                PromptId::tower_emerged_alert,
                3.5f,
                false,
                15.0f
            );
        }

        if (m_enemy_manager.consume_mana_spark_fired_event()) {
            m_prompt_overlay.dismiss_if_matching(PromptId::mana_spark_hint);
        }


        float tw_inc = m_enemy_manager.consume_pending_twilight_increase();
        if (tw_inc > 0.0f) {
            m_twilight_level = std::clamp(m_twilight_level + tw_inc, TWILIGHT_MIN, TWILIGHT_MAX);
            record_twilight_event(tw_inc, "Tower/Fixture Corruption");
            Audio::play_sfx(SFX::twilight_pulse());
        }

        update_twilight_metrics(dt, prev_twilight);
    }

    m_twilight_overlay.update(dt);
    m_particle_system.update(dt);

    m_telemetry.update_telemetry_dump(
        raw_dt,
        m_sim_tick_count,
        m_sim_elapsed_sec,
        m_time_scale,
        m_paused,
        m_twilight_level,
        m_enemy_manager,
        m_network,
        m_player,
        SIM_TICK_RATE,
        TWILIGHT_DECREASE_PER_MANA
    );
}

void MainScene::update_victory_condition(float raw_dt) {
    if (m_twilight_level <= TWILIGHT_HOLD_THRESHOLD) {
        if (m_time_to_zero_twilight < 0.0f) {
            m_time_to_zero_twilight = m_sim_elapsed_sec;
        }
        m_victory_hold_timer += raw_dt;

        // Whole-second countdown audio ticking
        if (!m_victory_achieved) {
            int remaining_sec = static_cast<int>(std::ceil(VICTORY_HOLD_DURATION_SEC - m_victory_hold_timer));
            remaining_sec = std::clamp(remaining_sec, 1, static_cast<int>(VICTORY_HOLD_DURATION_SEC));
            if (m_last_countdown_second != remaining_sec) {
                m_last_countdown_second = remaining_sec;
                Audio::play_sfx(SFX::countdown_tick());
            }
        }

        if (m_victory_hold_timer >= VICTORY_HOLD_DURATION_SEC) {
            m_victory_hold_timer = VICTORY_HOLD_DURATION_SEC;
            if (!m_victory_achieved) {
                m_victory_achieved = true;
                m_victory_sequence_timer = VICTORY_SEQUENCE_DURATION;
                m_can_build = false;
                Audio::play_sfx(SFX::victory_chime());
                m_prompt_overlay.show("Twilight dissipated - Room Cleared!", PromptType::info, PromptId::room_purified_info, 4.0f);
            }
        }

    } else {
        if (m_victory_hold_timer > 0.0f) {
            m_victory_hold_timer = std::max(0.0f, m_victory_hold_timer - (raw_dt * VICTORY_HOLD_DRAIN_RATE));
            if (m_victory_hold_timer <= 0.0f && !m_victory_achieved) {
                m_time_to_zero_twilight = -1.0f;
                m_last_countdown_second = -1;
            }
        } else {
            m_last_countdown_second = -1;
        }
    }
}

void MainScene::update_game_over_fade(float raw_dt) {
    if (m_player.state.defeated) {
        if (m_player.state.defeat_timer <= 0.0f) {
            if (!m_is_game_over) {
                m_is_game_over = true;
                m_game_over_fade_timer = GAME_OVER_FADE_DURATION;
            } else if (m_game_over_fade_timer > 0.0f) {
                m_game_over_fade_timer -= raw_dt;
            }
        }
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

void MainScene::update_twilight_metrics(float dt, float prev_twilight) {
    if constexpr (ALX_ENABLE_DEBUG) {
        if (Action::is_just_pressed(Action::DebugEnemyWave)) {
            m_enemy_manager.spawn_enemy_wave(m_tiles, &m_network, -1, m_player.center_x(1.0f), m_player.center_y(1.0f), false);
        }

        if (Action::is_pressed(Action::DebugTwUp)) {
            m_twilight_level += 1 * dt;
            m_twilight_level = std::clamp(m_twilight_level, TWILIGHT_MIN, TWILIGHT_MAX);
            record_twilight_event(1.0f * dt, "Debug TwUp");
        } else if (Action::is_pressed(Action::DebugTwDown)) {
            m_twilight_level -= 1 * dt;
            m_twilight_level = std::clamp(m_twilight_level, TWILIGHT_MIN, TWILIGHT_MAX);
            record_twilight_event(-1.0f * dt, "Debug TwDown");
        }
    }

    m_telemetry.update_metrics(dt, m_twilight_level, prev_twilight);
    m_momentum_tracker.update(dt, m_twilight_level, prev_twilight);
}

void MainScene::update_headless_defense(float dt) {
    m_telemetry.update_headless_defense(dt, m_player_spawn, m_tiles, m_enemy_manager);
}

void MainScene::print_headless_summary_report(int64_t seed) {
    m_telemetry.print_headless_summary_report(
        seed,
        m_sim_tick_count,
        m_sim_elapsed_sec,
        m_twilight_level,
        m_time_to_zero_twilight,
        m_enemy_manager,
        m_network,
        SIM_TICK_RATE,
        TWILIGHT_DECREASE_PER_MANA
    );
}

void MainScene::update_tick_simulation(float dt) {
    if (m_paused) return;

    m_sim_timer += dt;

    if (m_sim_timer >= SIM_TICK_RATE) {
        m_sim_timer = 0.0f;

        NetworkSimResults sim_res = m_network.sim_tick();
        if (sim_res.spires_converted > 0) {
            float dec = TWILIGHT_DECREASE_PER_MANA * sim_res.spires_converted;
            m_twilight_level = std::clamp(m_twilight_level - dec, TWILIGHT_MIN, TWILIGHT_MAX);
            record_twilight_event(-dec, "Spire Cleanse");
            Audio::play_sfx(SFX::spire_burn());
            m_prompt_overlay.try_show_once("Spire energized - Twilight clearing", PromptType::info, PromptId::spire_linked_info, 3.0f);
        }
        if (sim_res.refiners_processed > 0) {
            Audio::play_sfx(SFX::mana_converted());
            m_prompt_overlay.try_show_cooldown("Refiner purifying Dark Mana", PromptType::info, PromptId::refiner_active_info, 2.5f, false, 15.0f);
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
    m_player.draw(pixel_buffer, alpha, m_can_build, &m_tiles, &m_network);
    m_particle_system.draw(&m_camera);

    if (m_victory_achieved && m_victory_sequence_timer > 0.0f) {
        draw_victory_shockwave(alpha);
    }
}

void MainScene::trigger_vignette_surge(float duration) {
    m_twilight_overlay.trigger_vignette_surge(duration);
}

void MainScene::trigger_tower_spawn_alert(float vignette_duration, float shake_intensity, float shake_duration) {
    trigger_vignette_surge(vignette_duration);
    m_camera.shake(shake_intensity, shake_duration);
}

void MainScene::draw_screen(std::vector<uint32_t>& pixel_buffer, float alpha) {
    m_twilight_overlay.draw(m_twilight_level, m_camera, m_player.center_x(alpha), m_player.center_y(alpha), m_player.wand_radius);
    m_twilight_overlay.draw_vignette_surge();
    m_enemy_manager.draw_threat_indicators(m_camera);

    // Brief white-hot screen flash at the beginning of the victory sequence (first 0.35s)
    if (m_victory_achieved && m_victory_sequence_timer > 0.0f) {
        float progress = 1.0f - (m_victory_sequence_timer / VICTORY_SEQUENCE_DURATION);
        float flash_progress = std::clamp(progress / 0.35f, 0.0f, 1.0f);
        if (flash_progress < 1.0f) {
            uint32_t flash_a = static_cast<uint32_t>((1.0f - flash_progress) * 90.0f);
            Draw::rect(0, 0, Game::WIDTH, Game::HEIGHT, (flash_a << 24) | (SHOCKWAVE_FLASH_COLOR & 0x00FFFFFF), true, 1, 95);
        }
    }

    const FixtureType sel_type = m_player.selected_fixture_type();
    const HUDState hud_state{
        .player_hp = m_player.state.hp,
        .player_alloy = m_player.cursed_alloy(),
        .selected_fixture = sel_type,
        .fixture_cost = Player::fixture_cost(sel_type),
        .twilight_level = m_twilight_level,
        .victory_hold_timer = m_victory_hold_timer,
        .victory_hold_duration = VICTORY_HOLD_DURATION_SEC,
        .twilight_hold_threshold = TWILIGHT_HOLD_THRESHOLD,
        .twilight_max = TWILIGHT_MAX,
        .can_build = m_can_build,
        .paused = m_paused,
        .victory = m_victory_achieved,
        .is_game_over = m_is_game_over,
        .game_over_fade_timer = m_game_over_fade_timer,
        .game_over_fade_duration = GAME_OVER_FADE_DURATION,
        .is_victory_screen = m_is_victory_screen,
        .momentum = m_momentum_tracker.state(),
    };

    const Menu& active_menu = m_paused
        ? m_pause_menu
        : (m_is_victory_screen ? m_victory_menu : m_game_over_menu);

    HUD::draw(hud_state, active_menu, Game::WIDTH, Game::HEIGHT);
    m_prompt_overlay.draw(Game::WIDTH, Game::HEIGHT);
}

void MainScene::draw_victory_shockwave(float alpha) {
    float progress = 1.0f - (m_victory_sequence_timer / VICTORY_SEQUENCE_DURATION);
    progress = std::clamp(progress, 0.0f, 1.0f);

    float center_x = static_cast<float>(m_tiles.width() * m_tiles.tile_size()) * 0.5f;
    float center_y = static_cast<float>(m_tiles.height() * m_tiles.tile_size()) * 0.5f;

    // Expand radial light ring outward across the room/viewport
    float max_radius = std::sqrt(center_x * center_x + center_y * center_y) + static_cast<float>(m_tiles.tile_size() * 2);
    float ring_radius = progress * max_radius;

    float thickness_f = std::lerp(SHOCKWAVE_MAX_RING_THICKNESS, SHOCKWAVE_MIN_RING_THICKNESS, progress);
    int thickness = std::max(1, static_cast<int>(std::round(thickness_f)));

    float fade = 1.0f - progress;
    uint32_t alpha_byte = static_cast<uint32_t>(std::clamp(fade * 230.0f, 0.0f, 255.0f));
    uint32_t primary_color = (alpha_byte << 24) | (SHOCKWAVE_COLOR_PRIMARY & 0x00FFFFFF);

    // Primary bright expanding shockwave ring
    Draw::circle(center_x, center_y, ring_radius, primary_color, false, thickness, Layer::WorldObjSpireTop);

    // Inner concentric harmonic pulse ring
    if (ring_radius > 16.0f) {
        uint32_t inner_alpha = static_cast<uint32_t>(alpha_byte * 0.5f);
        uint32_t inner_color = (inner_alpha << 24) | (SHOCKWAVE_COLOR_ACCENT & 0x00FFFFFF);
        Draw::circle(center_x, center_y, ring_radius * 0.85f, inner_color, false, std::max(1, thickness - 1), Layer::WorldObjSpireTop);
    }
}

void MainScene::draw_tiles_and_network(std::vector<uint32_t>& pixel_buffer, float progress) {
    int min_tx = 0, max_tx = 0, min_ty = 0, max_ty = 0;
    m_camera.get_tile_bounds(
        m_tiles.width(), m_tiles.height(), m_tiles.tile_size(),
        min_tx, max_tx, min_ty, max_ty
    );

    m_tiles.draw(min_tx, max_tx, min_ty, max_ty);
    m_network.draw(
        min_tx, max_tx, min_ty, max_ty,
        m_player.transform, progress,
        m_paused ? nullptr : &m_particle_system, m_last_dt, SIM_TICK_RATE
    );
}

} // namespace alx
