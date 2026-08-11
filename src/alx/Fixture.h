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
    static constexpr int WALL_MAX_HP = 15;
    static constexpr int REFINER_MAX_HP = 30;
    static constexpr int SPIRE_MAX_HP = 30;
};

enum class FixtureType : uint8_t {
    None = 0,
    Pipe,
    Wall,
    Refiner,
    Spire,
    Seep
};

inline constexpr const char* fixture_glyph(FixtureType type) noexcept {
    switch (type) {
        case FixtureType::Pipe:    return "\x1D";
        case FixtureType::Wall:    return "\x16";
        case FixtureType::Refiner: return "\x0A";
        case FixtureType::Spire:   return "\x7F";
        default:                   return "";
    }
}

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

    // Convert (dx, dy) direction to DirectionMask bit
    inline constexpr uint8_t from_delta(int dx, int dy) {
        if (dy == -1) return North;
        if (dx ==  1) return East;
        if (dy ==  1) return South;
        if (dx == -1) return West;
        return None;
    }

    // Extract first set direction from mask into (dx, dy)
    inline void to_delta(uint8_t mask, int& dx, int& dy) {
        dx = 0; dy = 0;
        if (mask & North) { dy = -1; return; }
        if (mask & East)  { dx =  1; return; }
        if (mask & South) { dy =  1; return; }
        if (mask & West)  { dx = -1; return; }
    }
}

struct MultiTileFootprint {
    int width  = 1;
    int height = 1;

    [[nodiscard]] constexpr int pixel_width(int tile_size) const noexcept { return width * tile_size; }
    [[nodiscard]] constexpr int pixel_height(int tile_size) const noexcept { return height * tile_size; }
};

inline constexpr MultiTileFootprint get_fixture_footprint(FixtureType type) noexcept {
    switch (type) {
        case FixtureType::Refiner: return { 3, 3 };
        case FixtureType::Spire:   return { 2, 3 };
        case FixtureType::Seep:    return { 3, 2 };
        default:                   return { 1, 1 };
    }
}

struct PortLocation {
    int8_t off_x;      // Tile offset relative to building root
    int8_t off_y;
    int8_t face_dx;    // Connection direction facing outwards (-1, 0, 1)
    int8_t face_dy;    // Connection direction facing outwards (-1, 0, 1)
};

int get_fixture_ports(FixtureType type, PortLocation out_ports[4]) noexcept;
bool is_fixture_port(FixtureType type, int8_t off_x, int8_t off_y, int8_t face_dx, int8_t face_dy) noexcept;
int max_fixture_footprint_dimension() noexcept;

struct Fixture {
    FixtureType type        = FixtureType::None;
    ManaState mana_state    = ManaState::None;
    bool is_powered         = false;
    bool is_draining        = false;    // True when pipe is receding/draining downstream
    bool is_stepping        = false;    // True when mana orb is stepping into a new tile on this tick
    uint8_t flow_in_mask    = 0;     // Incoming connection bitfield
    uint8_t flow_out_mask   = 0;    // Outgoing flow direction bitfield
    int8_t move_dx          = 0;           // Directional flow delta X (-1, 0, 1)
    int8_t move_dy          = 0;           // Directional flow delta Y (-1, 0, 1)
    uint8_t process_timer   = 0;     // Processing / Stagnant tick timer
    uint8_t mana_ttl        = 0;     // Light Mana time-to-life TTL counter
    uint8_t last_in_dir_idx = 3;     // Intake round-robin direction memory (0=N, 1=S, 2=W, 3=E)
    uint8_t last_out_dir_idx= 3;     // Output round-robin direction memory (0=N, 1=S, 2=W, 3=E)
    int8_t root_offset_x    = 0;     // Multi-tile footprint X offset from root origin
    int8_t root_offset_y    = 0;     // Multi-tile footprint Y offset from root origin
    int hp                  = 0;     // Current HP pool
    int max_hp              = 0;     // Max HP pool

    [[nodiscard]] constexpr bool is_empty() const noexcept { return type == FixtureType::None; }
    [[nodiscard]] constexpr bool is_root() const noexcept { return root_offset_x == 0 && root_offset_y == 0; }
};

Collision::AABB fixture_ground_aabb(int tx, int ty, float tile_size, FixtureType type = FixtureType::None);

} // namespace alx

