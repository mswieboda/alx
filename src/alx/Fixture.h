#pragma once
#include <cstdint>
#include "core/Collision.h"

namespace alx {

struct FixtureConstants {
    static constexpr float GROUND_WIDTH_RATIO  = 1.00f; // 100% tile width (16.0px for 16px tile)
    static constexpr float GROUND_HEIGHT_RATIO = 0.50f; // 50% tile height (8.0px for 16px tile)
    static constexpr float GROUND_OFFSET_Y_RATIO = 0.50f; // Sits in bottom 50% of tile (y + 8.0px)
};

struct FixtureHPConstants {
    static constexpr int PIPE_MAX_HP = 10;
    static constexpr int REFINER_MAX_HP = 30;
    static constexpr int SPIRE_MAX_HP = 30;
};

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
    uint8_t last_dir_idx    = 3;     // Round-robin direction memory (0=N, 1=S, 2=W, 3=E)
    int hp                  = 0;     // Current HP pool
    int max_hp              = 0;     // Max HP pool

    [[nodiscard]] constexpr bool is_empty() const noexcept { return type == FixtureType::None; }
};

inline Collision::AABB fixture_ground_aabb(int tx, int ty, float tile_size) {
    float w = tile_size * FixtureConstants::GROUND_WIDTH_RATIO;
    float h = tile_size * FixtureConstants::GROUND_HEIGHT_RATIO;
    float x = static_cast<float>(tx) * tile_size + (tile_size - w) / 2.0f;
    float y = static_cast<float>(ty) * tile_size + (tile_size * FixtureConstants::GROUND_OFFSET_Y_RATIO);
    return Collision::AABB{ x, y, w, h };
}


} // namespace alx
