#include "alx/Levels.h"
#include <algorithm>
#include <span>

namespace alx::Levels {
namespace {

constexpr FixturePlacement level1_fixtures[] = {
    { {15, 12}, FixtureType::Seep },
    { {10,  8}, FixtureType::Refiner },
    { { 6,  6}, FixtureType::Spire },

    // seep top center port (16,12) to refiner east port (12,9)
    { {16, 11}, FixtureType::Pipe },
    { {16, 10}, FixtureType::Pipe },
    { {16,  9}, FixtureType::Pipe },
    { {15,  9}, FixtureType::Pipe },
    { {14,  9}, FixtureType::Pipe },
    { {13,  9}, FixtureType::Pipe },

    // refiner west port (10,9) to spire south port (7,8)
    { { 9,  9}, FixtureType::Pipe },
    { { 8,  9}, FixtureType::Pipe },
    { { 7,  9}, FixtureType::Pipe }
};

constexpr DarkTowerSpawn level1_spawns[] = {
    { {25, 10} },
    { {40, 12} },
    { {15, 20} },
    { {45,  8} }
};

constexpr WallPlacement level1_walls[] = {
    { {12, 14} },
    { {13, 14} },
    { {14, 14} }
};

const Level ALL_LEVELS[] = {
    Level{
        .id                = 1,
        .fixtures          = level1_fixtures,
        .dark_tower_spawns = level1_spawns,
        .walls             = level1_walls
    },
    Level{
        .id                = 2
    },
    Level{
        .id                = 3
    }
};

} // namespace

const Level* get_level(int id) {
    const auto it = std::ranges::find_if(ALL_LEVELS, [id](const Level& lvl) {
        return lvl.id == id;
    });
    return (it != std::end(ALL_LEVELS)) ? &(*it) : nullptr;
}

bool has_level(int id) {
    return get_level(id) != nullptr;
}

} // namespace alx::Levels
