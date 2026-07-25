#pragma once
#include <string>
#include <vector>
#include "core/Draw.h"
#include "core/Scene.h"
#include "core/Input.h"
#include "assets/Fonts.h"
#include "Grid.h"
#include "Player.h"
#include "Action.h"

namespace alx {

class MainScene : public Scene {
private:
    Grid m_grid;
    Player m_player;
    float m_sim_timer = 0;
    const float SIM_TICK_RATE = 0.6f; // Speed of the mana flow
    bool m_paused = false;

    // Level-specific progress stats
    float m_twilight_level = 0.8f;
    float m_wand_radius = 56.0f;
    int m_current_level_id = 1;

public:
    void init(SceneManager& sm) override {
        background_color = 0xFF131313; // very dark gray

        // --- CAMERA ---
        // Target tracking
        camera.follow(&m_player.center_x, &m_player.center_y);

        // --- LEVEL ---
        load_level(m_current_level_id);
    }

    void load_level(int level_id) {
        m_current_level_id = level_id;
        m_twilight_level = 0.75f; // Reset darkness for new room

        // temp data for specific initial level spawns
        std::vector<std::pair<int, int>> seeps;
        std::vector<std::pair<int, int>> refiners;
        std::vector<std::pair<int, int>> spires;
        std::vector<std::pair<int, int>> pipes;

        if (level_id == 1) {
            m_grid = Grid(40, 25);
            m_player = Player(300, 300);
            m_twilight_level = 0.5f;

            // Populate Level 1 coordinates
            seeps.push_back({15, 12});
            refiners.push_back({10, 8});

            // Input pipeline (Seep to Refiner)
            pipes = {
                {11, 8}, {12, 8}, {13, 8}, {14, 8}, {15, 8}, {15, 9}, {15, 10}, {15, 11}
            };
        }

        update_camera_map_boundary();

        load_tiles(seeps, refiners, spires, pipes);
    }

    void load_tiles(
        const std::vector<std::pair<int, int>>& seeps,
        const std::vector<std::pair<int, int>>& refiners,
        const std::vector<std::pair<int, int>>& spires,
        const std::vector<std::pair<int, int>>& pipes
    ) {
        // --- Base Grid Initialization (Walls & Floors) ---
        int width = m_grid.get_width();
        int height = m_grid.get_height();

        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                Tile& tile = m_grid.get_tile(x, y);

                if (x == 0 || x == width - 1 || y == 0 || y == height - 1) {
                    tile.type = TileType::Empty;
                } else if (x == 1 || x == width - 2 || y == 1 || y == height - 2) {
                    tile.type = TileType::Wall;
                } else {
                    tile.type = TileType::Floor;
                }
            }
        }

        // --- Apply initial level spawns ---
        // Seeps
        for (auto [x, y] : seeps) {
            Tile& tile = m_grid.get_tile(x, y);
            tile.type = TileType::Seep;
            tile.mana_state = ManaState::Dark;
        }

        // Refiners
        for (auto [x, y] : refiners) {
            m_grid.get_tile(x, y).type = TileType::Refiner;
        }

        // Light Spires
        for (auto [x, y] : spires) {
            m_grid.get_tile(x, y).type = TileType::LightSpire;
        }

        // Pipes
        for (auto [x, y] : pipes) {
            m_grid.get_tile(x, y).type = TileType::Pipe;
        }
    }

    void update_camera_map_boundary() {
        int tile_size = m_grid.get_tile_size();
        int bound_width = m_grid.get_width() * tile_size;
        int bound_height = (m_grid.get_height() * tile_size);

        camera.set_limits(0, 0, bound_width, bound_height);
    }

    void update(SceneManager& sm, float dt) override {
        if (Action::is_just_pressed(Action::Menu)) {
            m_paused = !m_paused;
        }

        update_tick_simulation(dt);

        // --- PLAYER ---
        m_player.update(dt, m_grid);

        // --- CAMERA ---
        camera.update();
    }

    void update_tick_simulation(float dt) {
        if (m_paused) return;

        m_sim_timer += dt;

        if (m_sim_timer >= SIM_TICK_RATE) {
            m_sim_timer = 0.0f;

            // --- GRID ---
            m_grid.tick_simulation(); // Advance factory items/fluid by one step
        }
    }

    // Direct primitive rendering loop for the grid
    void draw_custom(std::vector<uint32_t>& pixel_buffer, float alpha) override {
        float sub_tick_progress = std::clamp(m_sim_timer / SIM_TICK_RATE, 0.0f, 1.0f);

        draw_tiles(pixel_buffer, sub_tick_progress);
        m_player.draw(pixel_buffer, alpha, camera);

        draw_twilight(pixel_buffer);

        draw_hud();
    }

    void draw_twilight(std::vector<uint32_t>& pixel_buffer) {
        if (m_twilight_level > 0.0f) {

        }
    }

    void draw_hud() {
        int screen_width = Game::WIDTH;

        // Draw top HUD background bar (height 34px for 2-line display)
        Draw::rect(0, 0, screen_width, 34, 0xAA101019, true, 1, 99);

        // Build status string
        const char* selected_name = "Pipe";
        int cost = Grid::get_tile_cost(m_player.get_selected_build_type());
        if (m_player.get_selected_build_type() == TileType::Refiner) {
            selected_name = "Refiner";
        } else if (m_player.get_selected_build_type() == TileType::LightSpire) {
            selected_name = "LightSpire";
        }

        // Line 1: (left) ALLOY
        Draw::text(
            6, 4,
            Draw::fmt("ALLOY: %d", m_player.get_cursed_alloy()),
            0xFF00CCCC, 1, 100, &Assets::Fonts::mini
        );

        // Line 1: (center) TWILIGHT percentage
        int twilight_pct = static_cast<int>(m_twilight_level * 100.0f);
        std::string_view twilight_str = Draw::fmt("TWILIGHT: %d%%", twilight_pct);
        int twilight_width = Draw::text_width(twilight_str, 1, &Assets::Fonts::mini);
        Draw::text(
            screen_width / 2 - twilight_width / 2, 4,
            twilight_str,
            0xFF00CCCC, 1, 100, &Assets::Fonts::mini
        );

        // Line 1: (right) BUILD Status
        std::string_view build_str = Draw::fmt("BUILD: %s (%d)", selected_name, cost);
        int build_width = Draw::text_width(build_str, 1, &Assets::Fonts::mini);
        Draw::text(
            screen_width - 6 - build_width, 4,
            build_str,
            0xFF00CCCC, 1, 100, &Assets::Fonts::mini
        );

        // Line 2: Concise Controls Legend
        Draw::text(
            6, 18,
            "[J] Build  [K] Clear  [Q/E] Cycle  [Enter] Menu",
            0xFF003333, 1, 100, &Assets::Fonts::mini
        );
    }

    bool is_connectable_tile(int gx, int gy) const {
        if (!m_grid.is_in_bounds(gx, gy)) return false;
        TileType t = m_grid.get_tile(gx, gy).type;
        return t == TileType::Pipe || t == TileType::Seep || t == TileType::Refiner || t == TileType::LightSpire;
    }

    bool connects_dark_mana(int gx, int gy) const {
        if (!m_grid.is_in_bounds(gx, gy)) return false;
        Tile t = m_grid.get_tile(gx, gy);
        return (t.type == TileType::Seep) || (t.type == TileType::Refiner) || (t.type == TileType::Pipe && t.mana_state == ManaState::Dark);
    }

    void draw_tiles(std::vector<uint32_t>& pixel_buffer, float progress) {
        int tile_size = m_grid.get_tile_size();

        int min_tx = std::max(0, static_cast<int>(camera.get_x()) / tile_size);
        int max_tx = std::min(m_grid.get_width() - 1, static_cast<int>(camera.get_x() + Game::WIDTH) / tile_size + 1);
        int min_ty = std::max(0, static_cast<int>(camera.get_y()) / tile_size);
        int max_ty = std::min(m_grid.get_height() - 1, static_cast<int>(camera.get_y() + Game::HEIGHT) / tile_size + 1);

        for (int y = min_ty; y <= max_ty; ++y) {
            for (int x = min_tx; x <= max_tx; ++x) {
                Tile tile = m_grid.get_tile(x, y);
                int screen_x = camera.to_screen_x(x * tile_size);
                int screen_y = camera.to_screen_y(y * tile_size);

                draw_tile_bg(tile, x, y, screen_x, screen_y, tile_size);
                draw_tile_mana(tile, x, y, screen_x, screen_y, tile_size, progress);
                draw_tile_powered(tile, screen_x, screen_y, tile_size);
            }
        }
    }

    void draw_tile_bg(const Tile& tile, int gx, int gy, int screen_x, int screen_y, int tile_size) {
        if (tile.type == TileType::Pipe) {
            uint32_t pipe_color = 0xFF4A4A60; // Cool metallic conduit pipe casing

            // Render procedural skinny pipe (6px width centered in 32px tile)
            int hub_size = 8;
            int offset = (tile_size - hub_size) / 2; // 12
            int stub_len = offset; // 12

            Draw::rect(screen_x + offset, screen_y + offset, hub_size, hub_size, pipe_color, true, 1, 0);

            // North stub
            if (is_connectable_tile(gx, gy - 1)) {
                Draw::rect(screen_x + offset, screen_y, hub_size, stub_len, pipe_color, true, 1, 0);
            }
            // South stub
            if (is_connectable_tile(gx, gy + 1)) {
                Draw::rect(screen_x + offset, screen_y + offset + hub_size, hub_size, stub_len, pipe_color, true, 1, 0);
            }
            // West stub
            if (is_connectable_tile(gx - 1, gy)) {
                Draw::rect(screen_x, screen_y + offset, stub_len, hub_size, pipe_color, true, 1, 0);
            }
            // East stub
            if (is_connectable_tile(gx + 1, gy)) {
                Draw::rect(screen_x + offset + hub_size, screen_y + offset, stub_len, hub_size, pipe_color, true, 1, 0);
            }
            return;
        }

        // Choose color based on TileType
        uint32_t color = 0xFF2A2A38; // Default Floor (desaturated dark purple-grey)
        bool fill = true;

        if (tile.type == TileType::Wall) {
            color = 0xFF1C1C24; // Deep charcoal wall
        } else if (tile.type == TileType::Seep || tile.type == TileType::LightSpire) {
            color = 0xFF00FF66; // Sickly-green mana glow for seep/spire!
        } else if (tile.type == TileType::Refiner) {
            color = 0xFF301C66; // Refiner, purplish for now?
        } else if (tile.type == TileType::Floor) {
            fill = false; // Lined grid
        } else {
            color = 0x00FF00FF; // transparent
        }

        Draw::rect(
            screen_x,
            screen_y,
            tile_size,
            tile_size,
            color,
            fill,
            1,
            0
        );
    }

    bool is_node_tile(int gx, int gy) const {
        if (!m_grid.is_in_bounds(gx, gy)) return false;
        TileType t = m_grid.get_tile(gx, gy).type;
        return t == TileType::Seep || t == TileType::Refiner || t == TileType::LightSpire;
    }

    void draw_tile_mana(const Tile& tile, int gx, int gy, int screen_x, int screen_y, int tile_size, float progress) {
        if (!Grid::has_mana_glow(tile) || tile.mana_state == ManaState::None) {
            return;
        }

        if (tile.type == TileType::Pipe) {
            draw_tile_pipe_mana(tile, gx, gy, screen_x, screen_y, tile_size, progress);
            return;
        }

        // Generic mana glow for Seep / Refiner / Spire centers
        uint32_t color = 0xFF6600FF;
        if (tile.mana_state == ManaState::Light) {
            color = 0xFF00FFFF;
        }

        int size = tile_size / 2;
        Draw::rect(
            screen_x + size / 2,
            screen_y + size / 2,
            size,
            size,
            color,
            true,
            1,
            1
        );
    }

    void draw_tile_pipe_mana(const Tile& tile, int gx, int gy, int screen_x, int screen_y, int tile_size, float progress) {
        int src_gx = gx - tile.move_dx;
        int src_gy = gy - tile.move_dy;

        int travel_dist = tile_size;
        if (is_node_tile(src_gx, src_gy)) {
            travel_dist = tile_size / 2;
        }

        int anim_offset_x = static_cast<int>(-tile.move_dx * (1.0f - progress) * travel_dist);
        int anim_offset_y = static_cast<int>(-tile.move_dy * (1.0f - progress) * travel_dist);

        int render_x = screen_x + anim_offset_x;
        int render_y = screen_y + anim_offset_y;

        if (tile.mana_state == ManaState::Dark) {
            draw_tile_pipe_dark_mana(tile, gx, gy, render_x, render_y, screen_x, screen_y, tile_size, progress);
        } else if (tile.mana_state == ManaState::Light) {
            draw_tile_pipe_light_mana(tile, anim_offset_x, anim_offset_y, screen_x, screen_y, tile_size);
        }
    }

    void draw_tile_pipe_dark_mana(const Tile& tile, int gx, int gy, int render_x, int render_y, int screen_x, int screen_y, int tile_size, float progress) {
        uint32_t liquid_color = 0xFF9900FF; // Glowing twilight violet liquid
        int hub_x = screen_x + tile_size / 2;
        int hub_y = screen_y + tile_size / 2;
        int half = tile_size / 2; // 16 — distance from hub center to tile edge
        int stream_w = 4;
        int offset = (tile_size - stream_w) / 2; // 14

        // Use tile's stored movement vectors (zeroed by simulation on backpressure)
        int in_dx = tile.move_dx;
        int in_dy = tile.move_dy;
        int out_dx = tile.out_dx;
        int out_dy = tile.out_dy;

        // Only animate if the packet actually moved this tick
        if (in_dx != 0 || in_dy != 0) {
            // Determine exit direction for rendering:
            // On first arrival at a corner, out_dx/out_dy may still be 0
            // because simulation sets them on the source tile, not destination.
            // Query the distance field as a rendering-only fallback.
            if (out_dx == 0 && out_dy == 0) {
                m_grid.get_downstream_dir(gx, gy, ManaState::Dark, out_dx, out_dy);
            }
            if (out_dx == 0 && out_dy == 0) {
                out_dx = in_dx;
                out_dy = in_dy;
            }

            bool is_corner = (in_dx != out_dx || in_dy != out_dy);

            if (!is_corner) {
                // --- Straight pipe: full tile-length slug sliding along entry direction ---
                // The slug spans the full tile width and glides from previous position.
                // anim_offset already computed above handles the slide.
                if (in_dx != 0) {
                    // Horizontal movement
                    Draw::rect(render_x + offset, screen_y + offset, stream_w, stream_w, liquid_color, true, 1, 1);
                    // Fill full horizontal extent of pipe within this tile
                    if (is_connectable_tile(gx - 1, gy) || in_dx == 1) {
                        Draw::rect(render_x, screen_y + offset, offset, stream_w, liquid_color, true, 1, 1);
                    }
                    if (is_connectable_tile(gx + 1, gy) || in_dx == -1) {
                        Draw::rect(render_x + offset + stream_w, screen_y + offset, offset, stream_w, liquid_color, true, 1, 1);
                    }
                } else {
                    // Vertical movement
                    Draw::rect(screen_x + offset, render_y + offset, stream_w, stream_w, liquid_color, true, 1, 1);
                    if (is_connectable_tile(gx, gy - 1) || in_dy == 1) {
                        Draw::rect(screen_x + offset, render_y, stream_w, offset, liquid_color, true, 1, 1);
                    }
                    if (is_connectable_tile(gx, gy + 1) || in_dy == -1) {
                        Draw::rect(screen_x + offset, render_y + offset + stream_w, stream_w, offset, liquid_color, true, 1, 1);
                    }
                }
            } else {
                // --- Corner L-bend: slug head flows along L-path ---
                // L-path: entry edge → hub center (half) → exit edge (half) = tile_size total
                // Head position along L-path: 0 at progress=0, tile_size at progress=1
                float head_s = progress * tile_size;
                int in_len = static_cast<int>(std::min(static_cast<float>(half), head_s));
                int out_len = static_cast<int>(std::max(0.0f, head_s - half));

                // Draw hub center square once head reaches it (progress >= 0.5)
                if (head_s >= half) {
                    Draw::rect(hub_x - stream_w / 2, hub_y - stream_w / 2, stream_w, stream_w, liquid_color, true, 1, 1);
                }

                // Incoming arm (grows from entry edge toward hub center)
                if (in_len > 0) {
                    if (in_dy != 0) {
                        // Vertical incoming: anchor at entry edge
                        int y0 = (in_dy == 1) ? (hub_y - half) : (hub_y + half - in_len);
                        Draw::rect(hub_x - stream_w / 2, y0, stream_w, in_len, liquid_color, true, 1, 1);
                    } else {
                        // Horizontal incoming: anchor at entry edge
                        int x0 = (in_dx == 1) ? (hub_x - half) : (hub_x + half - in_len);
                        Draw::rect(x0, hub_y - stream_w / 2, in_len, stream_w, liquid_color, true, 1, 1);
                    }
                }

                // Outgoing arm (from hub center toward exit edge, expanding)
                if (out_len > 0) {
                    if (out_dy != 0) {
                        // Vertical outgoing
                        int y0 = (out_dy == 1) ? hub_y : (hub_y - out_len);
                        Draw::rect(hub_x - stream_w / 2, y0, stream_w, out_len, liquid_color, true, 1, 1);
                    } else {
                        // Horizontal outgoing
                        int x0 = (out_dx == 1) ? hub_x : (hub_x - out_len);
                        Draw::rect(x0, hub_y - stream_w / 2, out_len, stream_w, liquid_color, true, 1, 1);
                    }
                }
            }
        } else {
            // Stationary / backed-up Dark Mana pipe: fill connected pipe stubs
            int stub_len = offset; // 14

            Draw::rect(screen_x + offset, screen_y + offset, stream_w, stream_w, liquid_color, true, 1, 1);

            if (connects_dark_mana(gx, gy - 1)) {
                Draw::rect(screen_x + offset, screen_y, stream_w, stub_len, liquid_color, true, 1, 1);
            }
            if (connects_dark_mana(gx, gy + 1)) {
                Draw::rect(screen_x + offset, screen_y + offset + stream_w, stream_w, stub_len, liquid_color, true, 1, 1);
            }
            if (connects_dark_mana(gx - 1, gy)) {
                Draw::rect(screen_x, screen_y + offset, stub_len, stream_w, liquid_color, true, 1, 1);
            }
            if (connects_dark_mana(gx + 1, gy)) {
                Draw::rect(screen_x + offset + stream_w, screen_y + offset, stub_len, stream_w, liquid_color, true, 1, 1);
            }
        }
    }

    void draw_tile_pipe_light_mana(const Tile& tile, int anim_offset_x, int anim_offset_y, int screen_x, int screen_y, int tile_size) {
        // Radiant Light Mana Orb / Diamond Pulse
        uint32_t alpha = (tile.mana_ttl * 255) / Game::LIGHT_MANA_TIME_TO_LIFE_TICKS;
        if (alpha > 255) alpha = 255;

        uint32_t aura_color = (alpha << 24) | 0x0000FFFF;  // Cyan aura
        uint32_t core_color = (alpha << 24) | 0x00FFFFFF;  // Radiant white core

        int orb_size = 10;
        int offset = (tile_size - orb_size) / 2; // 11

        int orb_x = screen_x + offset + anim_offset_x;
        int orb_y = screen_y + offset + anim_offset_y;

        // Outer cyan aura
        Draw::rect(orb_x, orb_y, orb_size, orb_size, aura_color, true, 1, 1);
        // Inner white core
        Draw::rect(orb_x + 2, orb_y + 2, orb_size - 4, orb_size - 4, core_color, true, 1, 2);
    }

    void draw_tile_powered(const Tile& tile, int screen_x, int screen_y, int tile_size) {
        if (!Grid::has_power_glow(tile) || !tile.is_powered) {
            return;
        }

        uint32_t color = 0xFFFFFF00; // Yellow glow for powered on!
        bool fill = false;

        Draw::rect(
            screen_x,
            screen_y,
            tile_size,
            tile_size,
            color,
            fill, // filled
            1, // thickness
            0 // z-index
        );
    }

    // Optional getters if needed by player collision later
    Grid& get_grid() { return m_grid; }
    const Grid& get_grid() const { return m_grid; }
};

} // namespace alx
