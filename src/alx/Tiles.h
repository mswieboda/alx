#pragma once

#include <vector>
#include <algorithm>
#include <cmath>
#include "GridPos.h"
#include "Tile.h"
#include "Game.h"
#include "core/Draw.h"

namespace alx {

class Camera; // Forward declaration

class Tiles {
private:
    int m_width = 20;
    int m_height = 15;
    int m_tile_size = Game::TILE_SIZE;
    std::vector<Tile> m_tiles;

public:
    Tiles(int width = 20, int height = 15, int tile_size = Game::TILE_SIZE)
        : m_width(width), m_height(height), m_tile_size(tile_size)
    {
        m_tiles.resize(m_width * m_height, Tile{ TileType::Empty });
    }

    void resize(int width, int height) {
        m_width = width;
        m_height = height;
        m_tiles.assign(m_width * m_height, Tile{ TileType::Empty });
    }

    [[nodiscard]] bool in_bounds(GridPos pos) const noexcept {
        return pos.x >= 0 && pos.x < m_width && pos.y >= 0 && pos.y < m_height;
    }

    [[nodiscard]] bool in_bounds(int x, int y) const noexcept {
        return x >= 0 && x < m_width && y >= 0 && y < m_height;
    }

    [[nodiscard]] Tile& get_tile(GridPos pos) noexcept {
        return m_tiles[pos.to_index(m_width)];
    }

    [[nodiscard]] const Tile& get_tile(GridPos pos) const noexcept {
        return m_tiles[pos.to_index(m_width)];
    }

    [[nodiscard]] Tile& get_tile(int x, int y) noexcept {
        return m_tiles[y * m_width + x];
    }

    [[nodiscard]] const Tile& get_tile(int x, int y) const noexcept {
        return m_tiles[y * m_width + x];
    }

    void set_tile(GridPos pos, TileType type) noexcept {
        if (in_bounds(pos)) {
            get_tile(pos).type = type;
        }
    }

    void set_tile(int x, int y, TileType type) noexcept {
        if (in_bounds(x, y)) {
            get_tile(x, y).type = type;
        }
    }

    // Terrain Collision Checks
    [[nodiscard]] bool is_wall(GridPos pos) const noexcept {
        if (!in_bounds(pos)) return true;
        return get_tile(pos).type == TileType::Wall;
    }

    [[nodiscard]] bool is_wall(int x, int y) const noexcept {
        if (!in_bounds(x, y)) return true;
        return get_tile(x, y).type == TileType::Wall;
    }

    [[nodiscard]] bool is_floor(GridPos pos) const noexcept {
        if (!in_bounds(pos)) return false;
        return get_tile(pos).type == TileType::Floor;
    }

    [[nodiscard]] bool is_floor(int x, int y) const noexcept {
        if (!in_bounds(x, y)) return false;
        return get_tile(x, y).type == TileType::Floor;
    }

    [[nodiscard]] int get_width() const noexcept { return m_width; }
    [[nodiscard]] int get_height() const noexcept { return m_height; }
    [[nodiscard]] int get_tile_size() const noexcept { return m_tile_size; }
};

} // namespace alx
