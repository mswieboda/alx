#include "Tiles.h"

namespace alx {

Tiles::Tiles(int width, int height, int tile_size)
    : m_width(width), m_height(height), m_tile_size(tile_size)
{
    m_tiles.resize(m_width * m_height, Tile{ TileType::Empty });
}

void Tiles::resize(int width, int height) {
    m_width = width;
    m_height = height;
    m_tiles.assign(m_width * m_height, Tile{ TileType::Empty });
}

bool Tiles::in_bounds(GridPos pos) const noexcept {
    return pos.x >= 0 && pos.x < m_width && pos.y >= 0 && pos.y < m_height;
}

bool Tiles::in_bounds(int x, int y) const noexcept {
    return x >= 0 && x < m_width && y >= 0 && y < m_height;
}

Tile& Tiles::tile(GridPos pos) noexcept {
    return m_tiles[pos.to_index(m_width)];
}

const Tile& Tiles::tile(GridPos pos) const noexcept {
    return m_tiles[pos.to_index(m_width)];
}

Tile& Tiles::tile(int x, int y) noexcept {
    return m_tiles[y * m_width + x];
}

const Tile& Tiles::tile(int x, int y) const noexcept {
    return m_tiles[y * m_width + x];
}

void Tiles::set_tile(GridPos pos, TileType type) noexcept {
    if (in_bounds(pos)) {
        tile(pos).type = type;
    }
}

void Tiles::set_tile(int x, int y, TileType type) noexcept {
    if (in_bounds(x, y)) {
        tile(x, y).type = type;
    }
}

bool Tiles::is_wall(GridPos pos) const noexcept {
    if (!in_bounds(pos)) return true;
    return tile(pos).type == TileType::Wall;
}

bool Tiles::is_wall(int x, int y) const noexcept {
    if (!in_bounds(x, y)) return true;
    return tile(x, y).type == TileType::Wall;
}

bool Tiles::is_floor(GridPos pos) const noexcept {
    if (!in_bounds(pos)) return false;
    return tile(pos).type == TileType::Floor;
}

bool Tiles::is_floor(int x, int y) const noexcept {
    if (!in_bounds(x, y)) return false;
    return tile(x, y).type == TileType::Floor;
}

} // namespace alx
