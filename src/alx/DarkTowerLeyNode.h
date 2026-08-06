#pragma once

#include <cstdint>

namespace alx {

struct DarkTowerLeyNode {
    int tile_x{0};
    int tile_y{0};
    bool is_occupied{false};
    float last_spawn_time{0.0f};
};

} // namespace alx
