#pragma once
#include <algorithm>
#include <string>
#include <vector>
#include "core/Draw.h"
#include "core/Scene.h"
#include "core/Input.h"
#include "assets/Fonts.h"
#include "Game.h"
#include "Debug.h"
#include "Tiles.h"
#include "Network.h"
#include "Player.h"
#include "Action.h"
#include "EnemyManager.h"
#include "Random.h"
#include "Layer.h"
#include "ParticleSystem.h"
#include "ParticleEmitters.h"

namespace alx {

#include "alx/Camera.h"

class MainScene : public Scene {
private:
    // --- CONSTANTS ---
    static constexpr float TWILIGHT_MAX = 0.9f;
    static constexpr float TWILIGHT_DECREASE_PER_MANA = 0.005f;

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

public:
    Camera& camera() override { return m_camera; }
    const Camera& camera() const override { return m_camera; }

    void init(SceneManager& sm) override {
        background_color = 0xFF131313; // very dark gray

        if (m_twilight_pixel_buffer.size() != static_cast<size_t>(Game::WIDTH * Game::HEIGHT)) {
            m_twilight_pixel_buffer.resize(Game::WIDTH * Game::HEIGHT);
        }

        Draw::set_y_sort_mode(Draw::YSortMode::YPlusHeight);

        load_level(m_current_level_id);
    }

    void load_level(int level_id) {
        m_current_level_id = level_id;
        m_twilight_level = TWILIGHT_MAX;

        std::vector<std::pair<int, int>> seeps;
        std::vector<std::pair<int, int>> refiners;
        std::vector<std::pair<int, int>> spires;
        std::vector<std::pair<int, int>> pipes;

        if (level_id == 1) {
            m_tiles = Tiles(60, 30);
            m_network = Network(60, 30);
            m_player = Player(9 * m_tiles.tile_size(), 9 * m_tiles.tile_size());
            m_twilight_level = 0.75f;

            seeps = { {15, 12} };
            refiners = { {10, 8} };
            spires = { {6, 6} };

            pipes = {
                // seep top center port (16,12) to refiner east port (12,9)
                {16, 11}, {16, 10}, {16, 9}, {15, 9}, {14, 9}, {13, 9},

                // refiner west port (10,9) to spire south port (7,8)
                {9, 9}, {8, 9}, {7, 9}
            };
        }

        update_camera_map_boundary();
        load_tiles_and_network(seeps, refiners, spires, pipes);
        m_enemy_manager.clear();
    }

    void load_tiles_and_network(
        const std::vector<std::pair<int, int>>& seeps,
        const std::vector<std::pair<int, int>>& refiners,
        const std::vector<std::pair<int, int>>& spires,
        const std::vector<std::pair<int, int>>& pipes
    ) {
        int width = m_tiles.width();
        int height = m_tiles.height();

        // 1. Static Floor & Wall initialization
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                if (x == 0 || x == width - 1 || y == 0 || y == height - 1) {
                    m_tiles.set_tile(x, y, TileType::Empty);
                } else if (x == 1 || x == width - 2 || y == 1 || y == height - 2) {
                    m_tiles.set_tile(x, y, TileType::Wall);
                } else {
                    m_tiles.set_tile(x, y, TileType::Floor);
                }
            }
        }

        // 2. Clear & populate Network fixtures
        m_network.clear();

        for (auto [x, y] : seeps) {
            m_network.place_fixture(GridPos{ static_cast<int16_t>(x), static_cast<int16_t>(y) }, FixtureType::Seep);
        }
        for (auto [x, y] : refiners) {
            m_network.place_fixture(GridPos{ static_cast<int16_t>(x), static_cast<int16_t>(y) }, FixtureType::Refiner);
        }
        for (auto [x, y] : spires) {
            m_network.place_fixture(GridPos{ static_cast<int16_t>(x), static_cast<int16_t>(y) }, FixtureType::Spire);
        }
        for (auto [x, y] : pipes) {
            m_network.place_fixture(GridPos{ static_cast<int16_t>(x), static_cast<int16_t>(y) }, FixtureType::Pipe);
        }
    }

    void update_camera_map_boundary() {
        int tile_size = m_tiles.tile_size();
        int bound_width = m_tiles.width() * tile_size;
        int bound_height = (m_tiles.height() * tile_size);

        m_camera.set_limits(0, 0, bound_width, bound_height);
    }

    void update(SceneManager& sm, float dt) override {
        if (Action::is_just_pressed(Action::Menu)) {
            m_paused = !m_paused;
        }

        m_last_dt = dt;
        update_tick_simulation(dt);

        m_camera.follow(m_player.center_x(1.0f), m_player.center_y(1.0f));
        m_camera.update(dt);

        m_player.update(dt, m_tiles, m_network, m_camera);

        if (m_player.state.defeated && m_player.state.defeat_timer <= 0.0f) {
            m_player.state.hp = m_player.state.max_hp;
            m_player.state.defeated = false;
            m_player.state.iframe_timer = Player::State::IFRAME_DURATION;

            float spawn_x = 9.0f * m_tiles.tile_size();
            float spawn_y = 9.0f * m_tiles.tile_size();
            m_player.transform.x = spawn_x;
            m_player.transform.y = spawn_y;
            m_player.sync_prev_transforms();
        }

        m_enemy_manager.update(dt, &m_player, m_tiles, m_network, &m_particle_system);
        m_particle_system.update(dt);

        if (m_player.is_attacking()) {
            Collision::Circle hit_circle = m_player.attack_hit_circle(1.0f);
            float pcx = m_player.center_x(1.0f);
            float pcy = m_player.center_y(1.0f);

            float dir_x = hit_circle.cx - pcx;
            float dir_y = hit_circle.cy - pcy;
            float len = std::sqrt(dir_x * dir_x + dir_y * dir_y);
            if (len > 0.001f) {
                dir_x /= len;
                dir_y /= len;
            } else {
                dir_x = 0.0f;
                dir_y = 1.0f;
            }

            float curr_tip_x = hit_circle.cx + dir_x * (hit_circle.radius - 1.0f);
            float curr_tip_y = hit_circle.cy + dir_y * (hit_circle.radius - 1.0f);

            if (!m_slash_was_attacking) {
                m_slash_prev_tip_x = curr_tip_x;
                m_slash_prev_tip_y = curr_tip_y;
                m_slash_was_attacking = true;
            }

            int player_sort_y = static_cast<int>(m_player.transform.y + m_player.transform.height);
            ParticleEmitters::spawn_sword_slash_trail(
                m_particle_system, m_slash_prev_tip_x, m_slash_prev_tip_y, curr_tip_x, curr_tip_y,
                m_player.swing_progress_curr, 16, Layer::WorldObj, player_sort_y
            );

            m_slash_prev_tip_x = curr_tip_x;
            m_slash_prev_tip_y = curr_tip_y;
        } else {
            m_slash_was_attacking = false;
        }

        float tw_inc = m_enemy_manager.consume_pending_twilight_increase();
        if (tw_inc > 0.0f) {
            m_twilight_level = std::clamp(m_twilight_level + tw_inc, 0.0f, TWILIGHT_MAX);
        }

        if (Action::is_just_pressed(Action::DebugEnemyWave)) {
            m_enemy_manager.spawn_enemy_wave(m_tiles, &m_network, -1, m_player.center_x(1.0f), m_player.center_y(1.0f), false);
        }

        if (Action::is_pressed(Action::DebugTwUp)) {
            m_twilight_level += 1 * dt;
            m_twilight_level = std::clamp(m_twilight_level, 0.0f, TWILIGHT_MAX);
        } else if (Action::is_pressed(Action::DebugTwDown)) {
            m_twilight_level -= 1 * dt;
            m_twilight_level = std::clamp(m_twilight_level, 0.0f, TWILIGHT_MAX);
        }
    }

    void update_tick_simulation(float dt) {
        if (m_paused) return;

        m_sim_timer += dt;

        if (m_sim_timer >= SIM_TICK_RATE) {
            m_sim_timer = 0.0f;

            NetworkSimResults sim_res = m_network.sim_tick();
            if (sim_res.spires_converted > 0) {
                m_twilight_level -= TWILIGHT_DECREASE_PER_MANA * sim_res.spires_converted;
                m_twilight_level = std::clamp(m_twilight_level, 0.0f, TWILIGHT_MAX);
            }
        }
    }

    void sync_camera(float alpha) override {
        m_camera.sync_render_position(m_player.center_x(alpha), m_player.center_y(alpha));
    }

    void draw_world(std::vector<uint32_t>& pixel_buffer, float alpha) override {
        float sub_tick_progress = std::clamp(m_sim_timer / SIM_TICK_RATE, 0.0f, 1.0f);

        draw_tiles_and_network(pixel_buffer, sub_tick_progress);
        m_enemy_manager.draw_enemies(pixel_buffer, alpha);
        m_player.draw(pixel_buffer, alpha, &m_tiles, &m_network);
        m_particle_system.draw(&m_camera);
    }

    void draw_screen(std::vector<uint32_t>& pixel_buffer, float alpha) override {
        draw_twilight(pixel_buffer, alpha);
        draw_hud();
        m_enemy_manager.draw_threat_indicators(m_camera);
    }

    void draw_twilight(std::vector<uint32_t>& pixel_buffer, float alpha) {
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
        int radius = static_cast<int>(std::round(m_player.wand_radius * m_camera.zoom));
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

    void draw_hud() {
        int screen_width = Game::WIDTH;

        const char* selected_name = "pipe";
        FixtureType sel_type = m_player.selected_fixture_type();
        int cost = Player::fixture_cost(sel_type);
        if (sel_type == FixtureType::Refiner) {
            selected_name = "refiner";
        } else if (sel_type == FixtureType::Spire) {
            selected_name = "spire";
        }

        const FontData& font = Assets::Fonts::fant_8;
        const int line_h_padding = 4;
        int ly = line_h_padding;

        Draw::text(
            6, ly,
            Draw::fmt("hp: %d/%d  alloy: %d", m_player.state.hp, m_player.state.max_hp, m_player.cursed_alloy()),
            0xFF00CCCC, 1, Layer::HUD_Text, &font
        );

        int twilight_pct = static_cast<int>(m_twilight_level * 100.0f);
        std::string_view twilight_str = Draw::fmt("tw: %d%%", twilight_pct);
        int twilight_width = Draw::text_width(twilight_str, 1, &font);
        Draw::text(
            screen_width / 2 - twilight_width / 2, ly,
            twilight_str,
            0xFF00CCCC, 1, Layer::HUD_Text, &font
        );

        std::string_view build_str = Draw::fmt("%s (%d)", selected_name, cost);
        int build_width = Draw::text_width(build_str, 1, &font);
        Draw::text(
            screen_width - 6 - build_width, ly,
            build_str,
            0xFF00CCCC, 1, Layer::HUD_Text, &font
        );

        ly += font.size;
        const int rect_height = ly + line_h_padding;
        Draw::rect(0, 0, screen_width, rect_height, 0xAA101019, true, 1, Layer::HUD_BG);

        if constexpr (Debug::SHOW_SEED) {
            std::string_view seed_str = Draw::fmt(Random::is_custom_seeded() ? "seed: %u (custom)" : "seed: %u", Random::active_seed());
            int bottom_y = Game::HEIGHT - font.size - line_h_padding;
            Draw::text(
                6, bottom_y,
                seed_str,
                0x9900CCCC, 1, Layer::HUD_Text, &font
            );
        }
    }

    bool is_connectable_fixture(int gx, int gy) const {
        if (!m_network.in_bounds(gx, gy)) return false;
        FixtureType t = m_network.fixture(gx, gy).type;
        return t == FixtureType::Pipe || t == FixtureType::Seep || t == FixtureType::Refiner || t == FixtureType::Spire;
    }

    bool connects_dark_mana(int gx, int gy) const {
        if (!m_network.in_bounds(gx, gy)) return false;
        const Fixture& f = m_network.fixture(gx, gy);
        return (f.type == FixtureType::Pipe || f.type == FixtureType::Seep || f.type == FixtureType::Refiner) && f.mana_state == ManaState::Dark;
    }

    bool is_node_fixture(int gx, int gy) const {
        if (!m_network.in_bounds(gx, gy)) return false;
        FixtureType t = m_network.fixture(gx, gy).type;
        return t == FixtureType::Seep || t == FixtureType::Refiner || t == FixtureType::Spire;
    }

    void draw_tiles_and_network(std::vector<uint32_t>& pixel_buffer, float progress) {
        int base_tile_size = m_tiles.tile_size();
        float view_w = Game::WIDTH / m_camera.zoom;
        float view_h = Game::HEIGHT / m_camera.zoom;

        int min_tx = std::max(0, static_cast<int>(m_camera.x) / base_tile_size);
        int max_tx = std::min(m_tiles.width() - 1, static_cast<int>(m_camera.x + view_w) / base_tile_size + 1);
        int min_ty = std::max(0, static_cast<int>(m_camera.y) / base_tile_size);
        int max_ty = std::min(m_tiles.height() - 1, static_cast<int>(m_camera.y + view_h) / base_tile_size + 1);

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
            m_particle_system, m_last_dt, SIM_TICK_RATE
        );
    }

    void draw_terrain_tile(const Tile& tile, int world_x, int world_y, int tile_size) {
        uint32_t color = 0xFF2A2A38;
        bool fill = true;

        if (tile.type == TileType::Wall) {
            color = 0xFF1C1C24;
        } else if (tile.type == TileType::Floor) {
            fill = false;
        } else {
            color = 0x00FF00FF;
        }

        Draw::rect(
            world_x,
            world_y,
            tile_size,
            tile_size,
            color,
            fill,
            1,
            Layer::Ground
        );
    }

    Tiles& tiles() { return m_tiles; }
    const Tiles& tiles() const { return m_tiles; }
    Network& network() { return m_network; }
    const Network& network() const { return m_network; }
};

} // namespace alx
