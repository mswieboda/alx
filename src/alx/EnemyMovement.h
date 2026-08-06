#pragma once

#include <utility>
#include <vector>
#include "core/Collision.h"
#include "alx/Tiles.h"
#include "alx/Network.h"

namespace alx {

struct Enemy; // Forward declaration
struct WorldStructure; // Forward declaration

namespace EnemyMovement {

enum class MovementPattern {
    ForwardConeWander, // Default GBA Forward-Cone Micro-Step Wander
    SpiderBurst,       // Isaac-style pause & dart (reserved for future enemy types)
    GridPatrol         // Grid-aligned patrol (reserved for future enemy types)
};

struct WanderConfig {
    float step_min_time = 0.25f;  // Minimum micro-step duration (seconds)
    float step_max_time = 0.50f;  // Maximum micro-step duration (seconds)
    float pause_chance = 0.333f;  // ~1/3 chance to pause if just moved
};

struct MovementState {
    MovementPattern pattern = MovementPattern::ForwardConeWander;
    float micro_step_timer = 0.0f;
    int last_dir_idx = -1; // 0..7 index into 8-way compass, -1 if initial step
    bool was_moving = false;
};

// 8-Way directional vectors (0: Right, 1: Down-Right, 2: Down, 3: Down-Left, 4: Left, 5: Up-Left, 6: Up, 7: Up-Right)
extern const std::pair<float, float> DIRS[8];

// Updates a wander micro-step for the given enemy using its MovementState and WanderConfig
void update_wander_step(Enemy& enemy, MovementState& state, float dt, const Tiles& tiles, const Network& network, const WanderConfig& config = {}, const std::vector<WorldStructure>* structures = nullptr);

// Resets movement state upon changing macro states or targets
void reset_wander_state(MovementState& state);

// Called when an enemy hits a wall during micro-stepping to immediately pick a new direction
void handle_wall_collision(Enemy& enemy, MovementState& state, const Tiles& tiles, const Network& network, const WanderConfig& config = {}, const std::vector<WorldStructure>* structures = nullptr);

} // namespace EnemyMovement
} // namespace alx
