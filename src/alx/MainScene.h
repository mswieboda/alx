#pragma once
#include <string>
#include <vector>
#include "core/Draw.h"
#include "core/helpers.h"
#include "core/Scene.h"
#include "core/Entity.h"
#include "core/Input.h"
#include "core/Audio.h"
#include "assets/Images.h"
#include "assets/Music.h"
#include "Grid.h"
#include "SFX.h"

namespace alx {

class MainScene : public Scene {
private:
    Grid m_grid{20, 15, 16};
    int m_score = 0;

public:
    void init(SceneManager& sm) override {
        background_color = 0xFF131313; // very dark gray from your boilerplate

        // Load the room data when the scene officially starts
        // Simple test pattern for our Cellar room setup:
        // Border walls, floor in the middle, and a twilight seep in the center
        int width = m_grid.getWidth();
        int height = m_grid.getHeight();

        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                Tile& tile = m_grid.getTile(x, y);

                // Set borders as walls
                if (x == 0 || x == width - 1 || y == 0 || y == height - 1) {
                    tile.type = TileType::Wall;
                } else {
                    tile.type = TileType::Floor;
                }
            }
        }

        // Drop a Twilight Seep resource node in the center
        Tile& seepTile = m_grid.getTile(width / 2, height / 2);
        seepTile.type = TileType::TwilightSeep;
        seepTile.manaState = ManaState::RawDark;
    }

    void update(SceneManager& sm, float dt) override {
        // Future: update entities, player movement, pipe flow logic here
    }

    // Direct primitive rendering loop for the grid
    void draw_custom(std::vector<uint32_t>& screen_buffer, float alpha = 1.0f) override {
        int tile_size = m_grid.getTileSize();

        for (int y = 0; y < m_grid.getHeight(); ++y) {
            for (int x = 0; x < m_grid.getWidth(); ++x) {
                const Tile& tile = m_grid.getTile(x, y);

                // Choose color based on TileType
                uint32_t color = 0xFF2A2A38; // Default Floor (desaturated dark purple-grey)
                bool fill = true;

                if (tile.type == TileType::Wall) {
                    color = 0xFF1C1C24; // Deep charcoal wall
                } else if (tile.type == TileType::TwilightSeep) {
                    color = 0xFF00FF66; // Sickly-green mana glow for the seep!
                } else {
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
        }
    }

    // Optional getters if needed by player collision later
    Grid& getGrid() { return m_grid; }
    const Grid& getGrid() const { return m_grid; }
};

} // namespace alx
