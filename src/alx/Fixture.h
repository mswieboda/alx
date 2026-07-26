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

enum class FixtureFlag : uint8_t {
    None        = 0,
    Powered     = 1 << 0,
    Active      = 1 << 1,
    DirtySprite = 1 << 2
};

struct Fixture {
    FixtureType type        = FixtureType::None;
    ManaState mana_state    = ManaState::None;
    uint8_t flow_in_mask    = 0;     // Incoming connection bitfield
    uint8_t flow_out_mask   = 0;    // Outgoing flow direction bitfield
    int8_t move_dx          = 0;           // Directional flow delta X (-1, 0, 1)
    int8_t move_dy          = 0;           // Directional flow delta Y (-1, 0, 1)
    uint8_t process_timer   = 0;     // Processing / Stagnant tick timer
    uint8_t flags           = 0;     // Bit 0: Powered, Bit 1: Active, Bit 2: DirtySprite

    [[nodiscard]] constexpr bool is_empty() const noexcept { return type == FixtureType::None; }
    [[nodiscard]] constexpr bool is_powered() const noexcept { return (flags & static_cast<uint8_t>(FixtureFlag::Powered)) != 0; }

    constexpr void set_powered(bool powered) noexcept {
        if (powered) {
            flags |= static_cast<uint8_t>(FixtureFlag::Powered);
        } else {
            flags &= ~static_cast<uint8_t>(FixtureFlag::Powered);
        }
    }
};

static_assert(sizeof(Fixture) == 8, "Fixture struct must remain 8 bytes!");

} // namespace alx
