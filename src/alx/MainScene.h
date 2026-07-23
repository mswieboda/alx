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
    Grid m_grid{20, 15, 32};
    Player m_player;
    float m_sim_timer = 0;
    const float SIM_TICK_RATE = 0.6f; // Speed of the mana flow
    bool m_paused = false;

public:
    MainScene() :
        m_player(128, 128) // initial x, y
    {}

    void init(SceneManager& sm) override {
        background_color = 0xFF131313; // very dark gray from your boilerplate

        // --- Tiles ---
        // Load the room data when the scene officially starts
        // Simple test pattern for our Cellar room setup:
        // Border walls, floor in the middle, and a twilight seep in the center
        int width = m_grid.get_width();
        int height = m_grid.get_height();

        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                Tile& tile = m_grid.get_tile(x, y);

                // Set borders as walls
                if (x == 0 || x == width - 1 || y == 0 || y == height - 1) {
                    tile.type = TileType::Wall;
                } else {
                    tile.type = TileType::Floor;
                }
            }
        }

        // Drop a Twilight Seep resource node
        Tile& seep_tile = m_grid.get_tile(10, 7);
        seep_tile.type = TileType::Seep;
        seep_tile.mana_state = ManaState::Dark;

        // Drop a Refiner node
        Tile& refiner_tile = m_grid.get_tile(5, 4);
        refiner_tile.type = TileType::Refiner;

        // Drop a LightSpire node
        // Tile& spire_tile = m_grid.get_tile(1, 2);
        // spire_tile.type = TileType::LightSpire;

        // Place initial test pipes (matching debug layout)
        std::vector<std::pair<int, int>> pipes = {
            // Input pipeline (Seep to Refiner)
            {1, 3}, {1, 4}, {2, 4}, {3, 4}, {4, 4},
            // Output pipeline (Refiner to LightSpire)
            {6, 4}, {6, 5}, {6, 6}, {6, 7}, {6, 8}, {6, 9},
            {7, 9}, {8, 9}, {8, 8}, {8, 7}
        };

        for (auto [px, py] : pipes) {
            m_grid.get_tile(px, py).type = TileType::Pipe;
        }
    }

    void update(SceneManager& sm, float dt) override {
        if (Action::is_just_pressed(Action::Menu)) {
            m_paused = !m_paused;
        }

        update_tick_simulation(dt);


        // --- PLAYER ---
        m_player.update(dt, m_grid);
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
    void draw_custom(std::vector<uint32_t>& screen_buffer, float alpha) override {
        draw_tiles(screen_buffer);
        m_player.draw(screen_buffer, alpha);
        draw_hud();
    }

    void draw_hud() {
        // Draw top HUD background bar (height 34px for 2-line display)
        Draw::rect(0, 0, m_grid.get_width() * m_grid.get_tile_size(), 34, 0xCC101018, true, 1, 99);

        // Build status string
        const char* selected_name = "Pipe";
        int cost = Grid::get_tile_cost(m_player.get_selected_build_type());
        if (m_player.get_selected_build_type() == TileType::Refiner) {
            selected_name = "Refiner";
        } else if (m_player.get_selected_build_type() == TileType::LightSpire) {
            selected_name = "LightSpire";
        }

        // Line 1: ALLOY & BUILD Status
        Draw::text(
            6, 4,
            Draw::fmt("ALLOY: %d  |  BUILD: %s (%d)", m_player.get_cursed_alloy(), selected_name, cost),
            0xFF00CCCC, 1, 100, &Assets::Fonts::mini
        );

        // Line 2: Concise Controls Legend
        Draw::text(
            6, 18,
            "[J] Build  [K] Clear  [Q/E] Cycle  [Enter] Menu",
            0xFF003333, 1, 100, &Assets::Fonts::mini
        );
    }

    void draw_tiles(std::vector<uint32_t>& screen_buffer) {
        int tile_size = m_grid.get_tile_size();

        for (int y = 0; y < m_grid.get_height(); ++y) {
            for (int x = 0; x < m_grid.get_width(); ++x) {
                Tile tile = m_grid.get_tile(x, y);
                draw_tile_bg(tile, x, y, tile_size);
                draw_tile_mana(tile, x, y, tile_size);
                draw_tile_powered(tile, x, y, tile_size);
            }
        }
    }

    void draw_tile_bg(const Tile& tile, int x, int y, int tile_size) {
        // Choose color based on TileType
        uint32_t color = 0xFF2A2A38; // Default Floor (desaturated dark purple-grey)
        bool fill = true;

        if (tile.type == TileType::Wall) {
            color = 0xFF1C1C24; // Deep charcoal wall
        } else if (tile.type == TileType::Seep || tile.type == TileType::LightSpire) {
            color = 0xFF00FF66; // Sickly-green mana glow for seep/spire!
        } else if (tile.type == TileType::Pipe) {
            color = 0xFF00CCCC; // Pipe, cyan for now?
        } else if (tile.type == TileType::Refiner) {
            color = 0xFF301C66; // Refiner, purplish for now?
        } else {
            // Floor grid for now
            fill = false;
        }

        Draw::rect(
            x * tile_size, // x
            y * tile_size, // y
            tile_size, // width
            tile_size, // height
            color,
            fill, // filled
            1, // thickness (unused if filled)
            0 // z-index
        );
    }

    void draw_tile_mana(const Tile& tile, int x, int y, int tile_size) {
        if (!Grid::has_mana_glow(tile) || tile.mana_state == ManaState::None) {
            return;
        }

        uint32_t color = 0xFF6600FF; // Dark mana glow

        if (tile.mana_state == ManaState::Light) {
            uint32_t alpha = 0xFF;
            if (tile.mana_ttl > 0 && tile.mana_ttl <= 6) {
                alpha = (tile.mana_ttl * 255) / 6;
            }
            color = (alpha << 24) | 0x00FFFFFF; // Fading white light glow
        }

        int size = tile_size / 2;
        bool fill = true;

        Draw::rect(
            x * tile_size + size / 2, // x
            y * tile_size + size / 2, // y
            size, // width
            size, // height
            color,
            fill, // filled
            1, // thickness (unused if filled)
            0 // z-index
        );
    }

    void draw_tile_powered(const Tile& tile, int x, int y, int tile_size) {
        if (!Grid::has_power_glow(tile) || !tile.is_powered) {
            return;
        }

        uint32_t color = 0xFFFFFF00; // Yellow glow for powered on!
        bool fill = false;

        Draw::rect(
            x * tile_size, // x
            y * tile_size, // y
            tile_size, // width
            tile_size, // height
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
