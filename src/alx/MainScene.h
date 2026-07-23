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
    Grid m_grid{40, 25, 32};
    Player m_player;
    float m_sim_timer = 0;
    const float SIM_TICK_RATE = 0.6f; // Speed of the mana flow
    bool m_paused = false;

public:
    MainScene() :
        m_player(300, 300) // initial x, y
    {}

    void init(SceneManager& sm) override {
        background_color = 0xFF131313; // very dark gray

        // --- CAMERA ---
        // Target tracking
        camera.follow(&m_player.center_x, &m_player.center_y);

        // Map boundary limits
        int tile_size = m_grid.get_tile_size();
        int bound_width = m_grid.get_width() * tile_size;
        int bound_height = (m_grid.get_height() * tile_size);

        camera.set_limits(0, 0, bound_width, bound_height);

        // --- Tiles ---
        // Load the room data when the scene officially starts
        int width = m_grid.get_width();
        int height = m_grid.get_height();

        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                Tile& tile = m_grid.get_tile(x, y);

                if (x == 0 || x == width - 1 || y == 0 || y == height - 1) {
                    // Set borders as empty
                    tile.type = TileType::Empty;
                } else if (x == 1 || x == width - 2 || y == 1 || y == height - 2) {
                    // Set inner borders as walls
                    tile.type = TileType::Wall;
                } else {
                    tile.type = TileType::Floor;
                }
            }
        }

        // Drop a Twilight Seep resource node
        Tile& seep_tile = m_grid.get_tile(15, 12);
        seep_tile.type = TileType::Seep;
        seep_tile.mana_state = ManaState::Dark;

        // Drop a Refiner node
        Tile& refiner_tile = m_grid.get_tile(10, 8);
        refiner_tile.type = TileType::Refiner;

        // Place initial test pipes
        std::vector<std::pair<int, int>> pipes = {
            // Input pipeline (Seep to Refiner)
            {11, 8}, {12, 8}, {13, 8}, {14, 8}, {15, 8}, {15, 9}, {15, 10}, {15, 11}
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
    void draw_custom(std::vector<uint32_t>& screen_buffer, float alpha) override {
        draw_tiles(screen_buffer);
        m_player.draw(screen_buffer, alpha, camera);
        draw_hud();
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

        // Line 1: ALLOY (left) & BUILD Status (right-aligned to far right of screen)
        Draw::text(
            6, 4,
            Draw::fmt("ALLOY: %d", m_player.get_cursed_alloy()),
            0xFF00CCCC, 1, 100, &Assets::Fonts::mini
        );

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

    void draw_tiles(std::vector<uint32_t>& screen_buffer) {
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

                draw_tile_bg(tile, screen_x, screen_y, tile_size);
                draw_tile_mana(tile, screen_x, screen_y, tile_size);
                draw_tile_powered(tile, screen_x, screen_y, tile_size);
            }
        }
    }

    void draw_tile_bg(const Tile& tile, int screen_x, int screen_y, int tile_size) {
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
        } else if (tile.type == TileType::Floor) {
            // Floor is a lined grid for now
            fill = false;
        } else {
            // Empty
            color = 0x00FF00FF; // transparent, whatever the scene background is
        }

        Draw::rect(
            screen_x,
            screen_y,
            tile_size,
            tile_size,
            color,
            fill, // filled
            1, // thickness (unused if filled)
            0 // z-index
        );
    }

    void draw_tile_mana(const Tile& tile, int screen_x, int screen_y, int tile_size) {
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
            screen_x + size / 2,
            screen_y + size / 2,
            size,
            size,
            color,
            fill, // filled
            1, // thickness (unused if filled)
            0 // z-index
        );
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
