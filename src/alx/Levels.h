#pragma once

#include <span>
#include "alx/GridPos.h"
#include "alx/Fixture.h"

namespace alx {

struct FixturePlacement {
    GridPos     pos;
    FixtureType type{FixtureType::None};
};

struct DarkTowerSpawn {
    GridPos pos;
};

struct Level {
    int     id{0};
    int     map_width{0};
    int     map_height{0};
    GridPos player_spawn;
    float   initial_twilight{0.0f};
    std::span<const FixturePlacement> fixtures;
    std::span<const DarkTowerSpawn>   dark_tower_spawns;
};

namespace Levels {
    [[nodiscard]] const Level* get_level(int id);
} // namespace Levels

} // namespace alx
