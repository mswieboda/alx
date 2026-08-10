#pragma once

#include <vector>
#include "GridPos.h"
#include "Tile.h"
#include "Game.h"

namespace alx {

class Tiles {
private:
    int m_width = 20;
    int m_height = 15;
    int m_tile_size = Game::TILE_SIZE;
    std::vector<Tile> m_tiles;

public:
    Tiles(int width = 20, int height = 15, int tile_size = Game::TILE_SIZE);

    void resize(int width, int height);

    [[nodiscard]] bool in_bounds(GridPos pos) const noexcept;
    [[nodiscard]] bool in_bounds(int x, int y) const noexcept;

    [[nodiscard]] Tile& tile(GridPos pos) noexcept;
    [[nodiscard]] const Tile& tile(GridPos pos) const noexcept;
    [[nodiscard]] Tile& tile(int x, int y) noexcept;
    [[nodiscard]] const Tile& tile(int x, int y) const noexcept;

    void set_tile(GridPos pos, TileType type) noexcept;
    void set_tile(int x, int y, TileType type) noexcept;

    [[nodiscard]] bool is_wall(GridPos pos) const noexcept;
    [[nodiscard]] bool is_wall(int x, int y) const noexcept;
    [[nodiscard]] bool is_floor(GridPos pos) const noexcept;
    [[nodiscard]] bool is_floor(int x, int y) const noexcept;

    [[nodiscard]] int width() const noexcept { return m_width; }
    [[nodiscard]] int height() const noexcept { return m_height; }
    [[nodiscard]] int tile_size() const noexcept { return m_tile_size; }

    [[nodiscard]] float world_width() const noexcept { return static_cast<float>(m_width * m_tile_size); }
    [[nodiscard]] float world_height() const noexcept { return static_cast<float>(m_height * m_tile_size); }
};

} // namespace alx
