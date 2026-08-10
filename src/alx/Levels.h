#pragma once

#include <span>
#include "alx/GridPos.h"
#include "alx/Fixture.h"
#include "alx/Tile.h"

namespace alx {

struct FixturePlacement {
    GridPos     pos;
    FixtureType type{FixtureType::None};
};

struct DarkTowerSpawn {
    GridPos pos;
};

struct TilePlacement {
    GridPos  pos;
    TileType type{TileType::Floor};
};

struct Level {
    static constexpr int DEFAULT_MAP_WIDTH = 60;
    static constexpr int DEFAULT_MAP_HEIGHT = 30;
    static constexpr float DEFAULT_INITIAL_TWILIGHT = 0.9f;

    int     id{0};
    int     map_width{DEFAULT_MAP_WIDTH};
    int     map_height{DEFAULT_MAP_HEIGHT};
    GridPos player_spawn{9, 9};
    float   initial_twilight{DEFAULT_INITIAL_TWILIGHT};
    std::span<const FixturePlacement> fixtures;
    std::span<const DarkTowerSpawn>   dark_tower_spawns;
    std::span<const TilePlacement>    custom_tiles;
};

namespace Levels {
    [[nodiscard]] const Level* get_level(int id);
    [[nodiscard]] bool has_level(int id);
} // namespace Levels

} // namespace alx
