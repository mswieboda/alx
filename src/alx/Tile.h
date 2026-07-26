#pragma once

#include <cstdint>

namespace alx {

enum class TileType : uint8_t {
    Empty = 0,
    Floor,
    Wall
};

struct Tile {
    TileType type = TileType::Empty;
};

} // namespace alx
