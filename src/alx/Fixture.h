#pragma once

#include <cstdint>

namespace alx {

enum class FixtureType : uint8_t {
    None = 0,
    Pipe,
    Refiner,
    Spire,
    Seep
};

enum class ManaState : uint8_t {
    None = 0,
    Dark,
    Light
};

namespace DirectionMask {
    constexpr uint8_t None  = 0;
    constexpr uint8_t North = 1 << 0;
    constexpr uint8_t East  = 1 << 1;
    constexpr uint8_t South = 1 << 2;
    constexpr uint8_t West  = 1 << 3;
}

struct Fixture {
    FixtureType type        = FixtureType::None;
    ManaState mana_state    = ManaState::None;
    bool is_powered         = false;
    uint8_t flow_in_mask    = 0;     // Incoming connection bitfield
    uint8_t flow_out_mask   = 0;    // Outgoing flow direction bitfield
    int8_t move_dx          = 0;           // Directional flow delta X (-1, 0, 1)
    int8_t move_dy          = 0;           // Directional flow delta Y (-1, 0, 1)
    int8_t out_dx           = 0;           // Outgoing flow delta X for corner rendering
    int8_t out_dy           = 0;           // Outgoing flow delta Y for corner rendering
    uint8_t process_timer   = 0;     // Processing / Stagnant tick timer
    uint8_t mana_ttl        = 0;     // Light Mana time-to-life TTL counter

    [[nodiscard]] constexpr bool is_empty() const noexcept { return type == FixtureType::None; }
};

} // namespace alx
