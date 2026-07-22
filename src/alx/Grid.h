#pragma once

#include <vector>
#include <cstdint>

namespace alx {

// Enum representing the different types of grid elements in Aetherlux
enum class TileType : uint8_t {
    Empty = 0,
    Floor,
    Wall,
    Seep,       // Raw resource node (dark purple-black mana source)
    Pipe,       // Player-laid infrastructure pipe
    Refiner     // Conversion station (turns dark mana into cyan/gold light)
};

// State for pipes or nodes to handle power/mana routing logic later
enum class ManaState : uint8_t {
    None = 0,
    RawDark,       // Volatile dark twilight mana
    PurifiedLight  // Stable light energy
};

struct Tile {
    TileType type = TileType::Empty;
    ManaState manaState = ManaState::None;
    bool isPowered = false;
};

class Grid {
public:
    Grid(int width, int height, int tileSize)
        : m_width(width), m_height(height), m_tileSize(tileSize) {
        m_tiles.resize(m_width * m_height);
    }

    // Coordinate indexing helper
    Tile& get_tile(int x, int y) {
        return m_tiles[y * m_width + x];
    }

    const Tile& get_tile(int x, int y) const {
        return m_tiles[y * m_width + x];
    }

    bool is_walkable(int x, int y) const {
        if (x < 0 || x >= m_width || y < 0 || y >= m_height) return false;
        TileType t = get_tile(x, y).type;
        // Player can walk on floors, around pipes, and near seeps; walls block movement
        return t != TileType::Wall && t != TileType::Empty;
    }

    int get_width() const { return m_width; }
    int get_height() const { return m_height; }
    int get_tile_size() const { return m_tileSize; }

    void toggle_conduit(int tx, int ty) {
        // Bounds check
        if (tx < 0 || tx >= m_width || ty < 0 || ty >= m_height) {
            return;
        }

        // Grab the tile reference
        Tile& tile = get_tile(tx, ty);

        // Toggle logic
        if (tile.type == TileType::Floor || tile.type == TileType::Empty) {
            tile.type = TileType::Pipe;
        } else if (tile.type == TileType::Pipe) {
            // Pick it back up
            tile.type = TileType::Floor;
            tile.manaState = ManaState::None; // Reset state just in case
            tile.isPowered = false;
        }
    }

private:
    int m_width;
    int m_height;
    int m_tileSize; // e.g., 16 pixels per tile for GBA aesthetic
    std::vector<Tile> m_tiles;
};

} // namespace alx
