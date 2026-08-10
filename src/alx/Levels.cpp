#include "alx/Levels.h"

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

const Level level1 = {
    .id               = 1,
    .map_width        = 60,
    .map_height       = 30,
    .player_spawn     = {9, 9},
    .initial_twilight = 0.9f,
    .fixtures         = level1_fixtures,
    .dark_tower_spawns = level1_spawns
};

const Level level2 = {
    .id               = 2,
    .map_width        = 60,
    .map_height       = 30,
    .player_spawn     = {9, 9},
    .initial_twilight = 0.9f,
    .fixtures         = {},
    .dark_tower_spawns = {}
};

const Level level3 = {
    .id               = 3,
    .map_width        = 60,
    .map_height       = 30,
    .player_spawn     = {9, 9},
    .initial_twilight = 0.9f,
    .fixtures         = {},
    .dark_tower_spawns = {}
};

} // namespace

const Level* get_level(int id) {
    switch (id) {
        case 1: return &level1;
        case 2: return &level2;
        case 3: return &level3;
        default: return nullptr;
    }
}

} // namespace alx::Levels
