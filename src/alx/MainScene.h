#pragma once
#include <algorithm>
#include <string>
#include <vector>
#include "core/Draw.h"
#include "core/Scene.h"
#include "core/Input.h"
#include "assets/Fonts.h"
#include "Tiles.h"
#include "Network.h"
#include "Player.h"
#include "Action.h"
#include "EnemyManager.h"
#include "Layer.h"

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
    alx::Camera m_camera;
    float m_sim_timer = 0;
    const float SIM_TICK_RATE = 0.6f; // Speed of the mana flow
    bool m_paused = false;
    std::vector<uint32_t> m_twilight_pixel_buffer;

    // Level-specific progress stats
    float m_twilight_level = TWILIGHT_MAX;
    float m_wand_radius = 56.0f;
    int m_current_level_id = 1;

public:
    Camera& get_camera() override { return m_camera; }
    const Camera& get_camera() const override { return m_camera; }

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
                // seep to refiner
                {11, 8}, {12, 8}, {13, 8}, {14, 8}, {15, 8}, {15, 9}, {15, 10}, {15, 11},

                // refiner to spire
                {9, 8}, {9, 9}, {8, 9}, {7, 9}, {6, 9}, {6, 8}, {6, 7}
            };
        }

        update_camera_map_boundary();
        load_tiles_and_network(seeps, refiners, spires, pipes);
        m_enemy_manager.spawn_random_enemies(m_tiles, SpawnerConstants::DEFAULT_SPAWN_COUNT, m_player.center_x(1.0f), m_player.center_y(1.0f));
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

        update_tick_simulation(dt);

        m_camera.follow(m_player.center_x(1.0f), m_player.center_y(1.0f));
        m_camera.update(dt);

        m_player.update(dt, m_tiles, m_network, m_camera);
        m_enemy_manager.update(dt, &m_player, m_tiles, m_network);

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
        m_player.draw(pixel_buffer, alpha);
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
        int radius = static_cast<int>(std::round(m_player.wand_radius * m_camera.get_zoom()));
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
            Draw::fmt("alloy: %d", m_player.cursed_alloy()),
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
        float view_w = Game::WIDTH / m_camera.get_zoom();
        float view_h = Game::HEIGHT / m_camera.get_zoom();

        int min_tx = std::max(0, static_cast<int>(m_camera.get_x()) / base_tile_size);
        int max_tx = std::min(m_tiles.width() - 1, static_cast<int>(m_camera.get_x() + view_w) / base_tile_size + 1);
        int min_ty = std::max(0, static_cast<int>(m_camera.get_y()) / base_tile_size);
        int max_ty = std::min(m_tiles.height() - 1, static_cast<int>(m_camera.get_y() + view_h) / base_tile_size + 1);

        // Layer 0: Render Terrain Floor & Wall tiles
        for (int y = min_ty; y <= max_ty; ++y) {
            for (int x = min_tx; x <= max_tx; ++x) {
                Tile tile = m_tiles.tile(x, y);
                int world_x = x * base_tile_size;
                int world_y = y * base_tile_size;

                draw_terrain_tile(tile, world_x, world_y, base_tile_size);
            }
        }

        // Layer 1: Render Network Fixtures (Pipes, Refiners, Spires, Seeps, Mana Flow Orbs)
        for (int y = min_ty; y <= max_ty; ++y) {
            for (int x = min_tx; x <= max_tx; ++x) {
                const Fixture& fix = m_network.fixture(x, y);
                if (fix.is_empty()) continue;

                int world_x = x * base_tile_size;
                int world_y = y * base_tile_size;

                draw_fixture_bg(fix, x, y, world_x, world_y, base_tile_size);
                draw_fixture_mana(fix, x, y, world_x, world_y, base_tile_size, progress);
                draw_fixture_powered(fix, world_x, world_y, base_tile_size);
            }
        }
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

    void draw_fixture_bg(const Fixture& fix, int gx, int gy, int world_x, int world_y, int tile_size) {
        if (fix.type == FixtureType::Pipe) {
            uint32_t pipe_color = 0xFF4A4A60;

            int hub_size = 8;
            int offset = (tile_size - hub_size) / 2;
            int stub_len = offset;

            Draw::rect(world_x + offset, world_y + offset, hub_size, hub_size, pipe_color, true, 1, Layer::GroundFixture);

            if (is_connectable_fixture(gx, gy - 1)) {
                Draw::rect(world_x + offset, world_y, hub_size, stub_len, pipe_color, true, 1, Layer::GroundFixture);
            }
            if (is_connectable_fixture(gx, gy + 1)) {
                Draw::rect(world_x + offset, world_y + offset + hub_size, hub_size, stub_len, pipe_color, true, 1, Layer::GroundFixture);
            }
            if (is_connectable_fixture(gx - 1, gy)) {
                Draw::rect(world_x, world_y + offset, stub_len, hub_size, pipe_color, true, 1, Layer::GroundFixture);
            }
            if (is_connectable_fixture(gx + 1, gy)) {
                Draw::rect(world_x + offset + hub_size, world_y + offset, stub_len, hub_size, pipe_color, true, 1, Layer::GroundFixture);
            }
            return;
        }

        uint32_t color = 0xFF00FF66;
        int z_idx = Layer::GroundFixture;
        if (fix.type == FixtureType::Refiner) {
            color = 0xFF301C66;
            z_idx = Layer::WorldObj;
        } else if (fix.type == FixtureType::Spire) {
            color = 0xFF00FF66;
            z_idx = Layer::WorldObj;
        } else if (fix.type == FixtureType::Seep) {
            color = 0xFF00FF66;
            z_idx = Layer::GroundFixture;
        }

        Draw::rect(
            world_x,
            world_y,
            tile_size,
            tile_size,
            color,
            true,
            1,
            z_idx
        );
    }

    void draw_fixture_mana(const Fixture& fix, int gx, int gy, int world_x, int world_y, int tile_size, float progress) {
        if (fix.mana_state == ManaState::None) return;

        if (fix.type == FixtureType::Pipe) {
            draw_pipe_mana(fix, gx, gy, world_x, world_y, tile_size, progress);
            return;
        }

        uint32_t color = (fix.mana_state == ManaState::Light) ? 0xFF00FFFF : 0xFF6600FF;
        int size = tile_size / 2;
        int z_idx = (fix.type == FixtureType::Refiner || fix.type == FixtureType::Spire) ? Layer::WorldObj : Layer::GroundFixtureItem;
        Draw::rect(
            world_x + size / 2,
            world_y + size / 2,
            size,
            size,
            color,
            true,
            1,
            z_idx
        );
    }

    void draw_pipe_mana(const Fixture& fix, int gx, int gy, int world_x, int world_y, int tile_size, float progress) {
        int src_gx = gx - fix.move_dx;
        int src_gy = gy - fix.move_dy;

        int travel_dist = tile_size;
        if (is_node_fixture(src_gx, src_gy)) {
            travel_dist = tile_size / 2;
        }

        int anim_offset_x = static_cast<int>(-fix.move_dx * (1.0f - progress) * travel_dist);
        int anim_offset_y = static_cast<int>(-fix.move_dy * (1.0f - progress) * travel_dist);

        int render_x = world_x + anim_offset_x;
        int render_y = world_y + anim_offset_y;

        if (fix.mana_state == ManaState::Dark) {
            draw_tile_pipe_dark_mana(fix, gx, gy, render_x, render_y, world_x, world_y, tile_size, progress);
        } else if (fix.mana_state == ManaState::Light) {
            draw_tile_pipe_light_mana(fix, anim_offset_x, anim_offset_y, world_x, world_y, tile_size);
        }
    }

    void draw_tile_pipe_dark_mana(const Fixture& fix, int gx, int gy, int render_x, int render_y, int world_x, int world_y, int tile_size, float progress) {
        uint32_t liquid_color = 0xFF9900FF; // Glowing twilight violet liquid
        int hub_x = world_x + tile_size / 2;
        int hub_y = world_y + tile_size / 2;
        int half = tile_size / 2;
        int stream_w = 4;
        int offset = (tile_size - stream_w) / 2;

        int in_dx = fix.move_dx;
        int in_dy = fix.move_dy;
        int out_dx = fix.out_dx;
        int out_dy = fix.out_dy;

        if (in_dx != 0 || in_dy != 0) {
            if (out_dx == 0 && out_dy == 0) {
                m_network.downstream_dir(gx, gy, ManaState::Dark, out_dx, out_dy);
            }
            if (out_dx == 0 && out_dy == 0) {
                out_dx = in_dx;
                out_dy = in_dy;
            }

            bool is_corner = (in_dx != out_dx || in_dy != out_dy);

            if (!is_corner) {
                if (in_dx != 0) {
                    Draw::rect(render_x + offset, world_y + offset, stream_w, stream_w, liquid_color, true, 1, Layer::GroundFixtureItem);
                    if (is_connectable_fixture(gx - 1, gy) || in_dx == 1) {
                        Draw::rect(render_x, world_y + offset, offset, stream_w, liquid_color, true, 1, Layer::GroundFixtureItem);
                    }
                    if (is_connectable_fixture(gx + 1, gy) || in_dx == -1) {
                        Draw::rect(render_x + offset + stream_w, world_y + offset, offset, stream_w, liquid_color, true, 1, Layer::GroundFixtureItem);
                    }
                } else {
                    Draw::rect(world_x + offset, render_y + offset, stream_w, stream_w, liquid_color, true, 1, Layer::GroundFixtureItem);
                    if (is_connectable_fixture(gx, gy - 1) || in_dy == 1) {
                        Draw::rect(world_x + offset, render_y, stream_w, offset, liquid_color, true, 1, Layer::GroundFixtureItem);
                    }
                    if (is_connectable_fixture(gx, gy + 1) || in_dy == -1) {
                        Draw::rect(world_x + offset, render_y + offset + stream_w, stream_w, offset, liquid_color, true, 1, Layer::GroundFixtureItem);
                    }
                }
            } else {
                float head_s = progress * tile_size;
                int in_len = static_cast<int>(std::min(static_cast<float>(half), head_s));
                int out_len = static_cast<int>(std::max(0.0f, head_s - half));

                if (head_s >= half) {
                    Draw::rect(hub_x - stream_w / 2, hub_y - stream_w / 2, stream_w, stream_w, liquid_color, true, 1, Layer::GroundFixtureItem);
                }

                if (in_len > 0) {
                    if (in_dy != 0) {
                        int y0 = (in_dy == 1) ? (hub_y - half) : (hub_y + half - in_len);
                        Draw::rect(hub_x - stream_w / 2, y0, stream_w, in_len, liquid_color, true, 1, Layer::GroundFixtureItem);
                    } else {
                        int x0 = (in_dx == 1) ? (hub_x - half) : (hub_x + half - in_len);
                        Draw::rect(x0, hub_y - stream_w / 2, in_len, stream_w, liquid_color, true, 1, Layer::GroundFixtureItem);
                    }
                }

                if (out_len > 0) {
                    if (out_dy != 0) {
                        int y0 = (out_dy == 1) ? hub_y : (hub_y - out_len);
                        Draw::rect(hub_x - stream_w / 2, y0, stream_w, out_len, liquid_color, true, 1, Layer::GroundFixtureItem);
                    } else {
                        int x0 = (out_dx == 1) ? hub_x : (hub_x - out_len);
                        Draw::rect(x0, hub_y - stream_w / 2, out_len, stream_w, liquid_color, true, 1, Layer::GroundFixtureItem);
                    }
                }
            }
        } else {
            int stub_len = offset;
            Draw::rect(world_x + offset, world_y + offset, stream_w, stream_w, liquid_color, true, 1, Layer::GroundFixtureItem);

            if (connects_dark_mana(gx, gy - 1)) {
                Draw::rect(world_x + offset, world_y, stream_w, stub_len, liquid_color, true, 1, Layer::GroundFixtureItem);
            }
            if (connects_dark_mana(gx, gy + 1)) {
                Draw::rect(world_x + offset, world_y + offset + stream_w, stream_w, stub_len, liquid_color, true, 1, Layer::GroundFixtureItem);
            }
            if (connects_dark_mana(gx - 1, gy)) {
                Draw::rect(world_x, world_y + offset, stub_len, stream_w, liquid_color, true, 1, Layer::GroundFixtureItem);
            }
            if (connects_dark_mana(gx + 1, gy)) {
                Draw::rect(world_x + offset + stream_w, world_y + offset, stub_len, stream_w, liquid_color, true, 1, Layer::GroundFixtureItem);
            }
        }
    }

    void draw_tile_pipe_light_mana(const Fixture& fix, int anim_offset_x, int anim_offset_y, int world_x, int world_y, int tile_size) {
        uint32_t alpha = (fix.mana_ttl * 255) / Game::LIGHT_MANA_TIME_TO_LIFE_TICKS;
        if (alpha > 255) alpha = 255;

        uint32_t aura_color = (alpha << 24) | 0x0000FFFF;  // Cyan aura
        uint32_t core_color = (alpha << 24) | 0x00FFFFFF;  // Radiant white core

        int orb_size = 10;
        int offset = (tile_size - orb_size) / 2;

        int orb_x = world_x + offset + anim_offset_x;
        int orb_y = world_y + offset + anim_offset_y;

        Draw::rect(orb_x, orb_y, orb_size, orb_size, aura_color, true, 1, Layer::GroundFixtureItem);
        Draw::rect(orb_x + 2, orb_y + 2, orb_size - 4, orb_size - 4, core_color, true, 1, Layer::GroundFixtureItemFX);
    }

    void draw_fixture_powered(const Fixture& fix, int world_x, int world_y, int tile_size) {
        if (!fix.is_powered || (fix.type != FixtureType::Refiner && fix.type != FixtureType::Spire)) {
            return;
        }

        uint32_t color = 0xFFFFFF00;
        Draw::rect(world_x, world_y, tile_size, tile_size, color, false, 1, Layer::WorldObj);
    }

    Tiles& tiles() { return m_tiles; }
    const Tiles& tiles() const { return m_tiles; }
    Network& network() { return m_network; }
    const Network& network() const { return m_network; }
};

} // namespace alx
