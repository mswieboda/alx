#pragma once

#include <cstdint>

namespace alx {

struct CorruptedDarkTowerTile {
    int tile_x{0};
    int tile_y{0};
    bool is_occupied{false};
    float last_spawn_time{0.0f};

    [[nodiscard]] constexpr bool is_available() const noexcept {
        return !is_occupied;
    }
};

} // namespace alx
