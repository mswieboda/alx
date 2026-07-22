#pragma once
#include <string>
#include <vector>
#include "core/Draw.h"
#include "core/Scene.h"
#include "core/Input.h"
#include "Grid.h"
#include "Player.h"

namespace alx {

class MainScene : public Scene {
private:
    Grid m_grid{20, 15, 32};
    alx::Player m_player;
    float m_sim_timer = 0;
    const float SIM_TICK_RATE = 0.6f; // Speed of the mana flow

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

        // Drop a Twilight Seep resource node in the center
        Tile& seep_tile = m_grid.get_tile(width / 2, height / 2);
        seep_tile.type = TileType::Seep;
        seep_tile.mana_state = ManaState::Dark;

        // Drop a Refiner node near the center
        Tile& refiner_tile = m_grid.get_tile(width / 2 - 5, height / 2 - 3);
        refiner_tile.type = TileType::Refiner;
        // seepTile.mana_state = ManaState::Dark;
    }

    void update(SceneManager& sm, float dt) override {
        update_tick_simulation(dt);

        // --- PLAYER ---
        m_player.update(dt, m_grid);
    }

    void update_tick_simulation(float dt) {
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
        } else if (tile.type == TileType::Seep) {
            color = 0xFF00FF66; // Sickly-green mana glow for the seep!
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
            color = 0xFFFFFFFF; // pure white light
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
