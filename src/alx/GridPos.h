#pragma once

#include <cstdint>
#include <compare>

namespace alx {

struct GridPos {
    int16_t x = 0;
    int16_t y = 0;

    constexpr GridPos() noexcept = default;
    constexpr GridPos(int16_t x_, int16_t y_) noexcept : x(x_), y(y_) {}
    constexpr GridPos(int32_t x_, int32_t y_) noexcept
        : x(static_cast<int16_t>(x_)), y(static_cast<int16_t>(y_)) {}

    constexpr bool operator==(const GridPos& other) const noexcept = default;
    constexpr auto operator<=>(const GridPos& other) const noexcept = default;

    [[nodiscard]] constexpr int32_t to_index(int32_t map_width) const noexcept {
        return static_cast<int32_t>(y) * map_width + static_cast<int32_t>(x);
    }

    [[nodiscard]] static constexpr GridPos from_index(int32_t index, int32_t map_width) noexcept {
        return GridPos{ static_cast<int16_t>(index % map_width), static_cast<int16_t>(index / map_width) };
    }

    [[nodiscard]] constexpr GridPos north() const noexcept { return GridPos{ x, static_cast<int16_t>(y - 1) }; }
    [[nodiscard]] constexpr GridPos south() const noexcept { return GridPos{ x, static_cast<int16_t>(y + 1) }; }
    [[nodiscard]] constexpr GridPos west()  const noexcept { return GridPos{ static_cast<int16_t>(x - 1), y }; }
    [[nodiscard]] constexpr GridPos east()  const noexcept { return GridPos{ static_cast<int16_t>(x + 1), y }; }

    [[nodiscard]] constexpr int32_t manhattan_dist(GridPos other) const noexcept {
        int32_t dx = static_cast<int32_t>(x) - static_cast<int32_t>(other.x);
        int32_t dy = static_cast<int32_t>(y) - static_cast<int32_t>(other.y);
        return (dx < 0 ? -dx : dx) + (dy < 0 ? -dy : dy);
    }
};

static_assert(sizeof(GridPos) == 4, "GridPos must be 4 bytes!");

} // namespace alx
