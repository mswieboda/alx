#pragma once

#ifndef ALX_ENABLE_DEBUG
#  ifdef DEBUG
#    define ALX_ENABLE_DEBUG 1
#  else
#    define ALX_ENABLE_DEBUG 0
#  endif
#endif

namespace Debug {
    // Debug Options
#if ALX_ENABLE_DEBUG
    inline constexpr bool DRAW_GROUND_AREAS = false;
    inline constexpr bool DRAW_FIXTURE_COLLISION_AREAS = false;
    inline constexpr bool DRAW_HURT_AREAS = false;
    inline constexpr bool DRAW_MELEE_ARCS = false;
    inline constexpr bool SHOW_SEED = false;
    inline constexpr bool DRAW_ENEMY_FACING = true;
    inline constexpr bool DRAW_ENEMY_AGGRO_AREAS = true;
    inline constexpr bool DRAW_WORLD_STRUCTURE_COLLISION_AREAS = false;
    inline constexpr bool DRAW_WORLD_STRUCTURE_TEST = true;
    inline constexpr bool CAN_PAUSE = true;
#else
    inline constexpr bool DRAW_GROUND_AREAS = false;
    inline constexpr bool DRAW_FIXTURE_COLLISION_AREAS = false;
    inline constexpr bool DRAW_HURT_AREAS = false;
    inline constexpr bool DRAW_MELEE_ARCS = false;
    inline constexpr bool SHOW_SEED = false;
    inline constexpr bool DRAW_ENEMY_FACING = false;
    inline constexpr bool DRAW_ENEMY_AGGRO_AREAS = false;
    inline constexpr bool DRAW_WORLD_STRUCTURE_COLLISION_AREAS = false;
    inline constexpr bool DRAW_WORLD_STRUCTURE_TEST = false;
    inline constexpr bool CAN_PAUSE = false;
#endif
}

