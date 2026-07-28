#pragma once

#include "core/Collision.h"
#include "alx/Tiles.h"
#include "alx/Network.h"
#include "alx/Fixture.h"

namespace alx::WorldCollision {

struct MoveResult {
    float moved_dx = 0.0f;
    float moved_dy = 0.0f;
    bool blocked_x = false;
    bool blocked_y = false;
};

// Single Authoritative Solid Ground Check (Tile Walls + Solid Network Fixtures)
bool is_solid_ground(const Collision::Circle& ground, const Tiles& tiles, const Network& network);

// Shared Axis-Separated Movement Helper
MoveResult try_move(float& x, float& y, float dx, float dy, const Collision::Circle& ground, const Tiles& tiles, const Network& network);

// Shared Ejection Safety Net (Nudges 2.0px away from obstacles IF overlapping)
bool enforce_solid_ground_ejection(float& x, float& y, const Collision::Circle& ground, const Tiles& tiles, const Network& network, float nudge_dist = 2.0f, std::string tag = "?");

} // namespace alx::WorldCollision
