#include "alx/Tiles.h"
#include "alx/Camera.h"
#include "core/Draw.h"
#include "assets/Images.h"
#include "alx/Layer.h"

namespace alx {

namespace {
void draw_terrain_tile(const Tile& tile, int world_x, int world_y, int tile_size) {
    if (tile.type == TileType::Empty) {
        return;
    }

    size_t frame_index = 0;
    switch (tile.type) {
        case TileType::Floor:  frame_index = 0; break;
        case TileType::Water:  frame_index = 2; break;
        case TileType::Stone:  frame_index = 3; break;
        case TileType::Dirt:   frame_index = 4; break;
        default:               frame_index = 0; break;
    }

    if (frame_index < 5) {
        const auto& frame = Assets::Images::tileset_frames[frame_index];
        Draw::sprite(
            static_cast<float>(world_x),
            static_cast<float>(world_y),
            Assets::Images::tileset + frame.offset,
            static_cast<uint32_t>(frame.len),
            static_cast<float>(tile_size),
            static_cast<float>(tile_size),
            Layer::Ground
        );
    }
}
} // namespace

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

void Tiles::draw(int min_tx, int max_tx, int min_ty, int max_ty) const {
    for (int y = min_ty; y <= max_ty; ++y) {
        for (int x = min_tx; x <= max_tx; ++x) {
            const Tile& t = tile(x, y);
            const int world_x = x * m_tile_size;
            const int world_y = y * m_tile_size;
            draw_terrain_tile(t, world_x, world_y, m_tile_size);
        }
    }
}

void Tiles::draw(const Camera& camera) const {
    int min_tx = 0, max_tx = 0, min_ty = 0, max_ty = 0;
    camera.get_tile_bounds(m_width, m_height, m_tile_size, min_tx, max_tx, min_ty, max_ty);
    draw(min_tx, max_tx, min_ty, max_ty);
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
    const TileType t = tile(pos).type;
    return t == TileType::Water;
}

bool Tiles::is_wall(int x, int y) const noexcept {
    if (!in_bounds(x, y)) return true;
    const TileType t = tile(x, y).type;
    return t == TileType::Water;
}

bool Tiles::is_floor(GridPos pos) const noexcept {
    if (!in_bounds(pos)) return false;
    const TileType t = tile(pos).type;
    return t == TileType::Floor || t == TileType::Stone || t == TileType::Dirt;
}

bool Tiles::is_floor(int x, int y) const noexcept {
    if (!in_bounds(x, y)) return false;
    const TileType t = tile(x, y).type;
    return t == TileType::Floor || t == TileType::Stone || t == TileType::Dirt;
}

} // namespace alx
