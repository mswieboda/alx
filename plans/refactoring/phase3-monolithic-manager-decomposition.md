# Phase 3 Plan: Monolithic Manager & Method SLAP Decomposition (`phase3-monolithic-manager-decomposition.md`)

## `[EP-MMDC]`: Executive Overview & Objectives

The primary objective of **Phase 3** is to decompose God objects (`EnemyManager`, `Network`, `Player`, `MainScene`) into focused, single-responsibility subsystems and break down massive monolithic functions (exceeding 100–400 lines) using the Composed Method pattern and Single Level of Abstraction Principle (SLAP).

Resolving these monolithic structures will:
1. Enforce the Single Responsibility Principle (SRP) across entity and world managers.
2. Reduce cognitive complexity and function length ($\le 25$ lines / $\le 5$ statements per composed helper).
3. Separate high-level state policy from low-level vector arithmetic, coordinate math, bitwise flags, and pixel rendering.
4. Replace raw numeric literals with strongly typed `constexpr` domain constants.

---

## Target Files & Code Locations

* [`src/alx/EnemyManager.cpp`](file:///Users/matt/code/cpp/alx/src/alx/EnemyManager.cpp) (51.2 KB, 1,085 lines) / [`src/alx/EnemyManager.h`](file:///Users/matt/code/cpp/alx/src/alx/EnemyManager.h)
* [`src/alx/Network.cpp`](file:///Users/matt/code/cpp/alx/src/alx/Network.cpp) (36 KB, 922 lines) / [`src/alx/Network.h`](file:///Users/matt/code/cpp/alx/src/alx/Network.h)
* [`src/alx/Player.cpp`](file:///Users/matt/code/cpp/alx/src/alx/Player.cpp) (21.3 KB, 589 lines) / [`src/alx/Player.h`](file:///Users/matt/code/cpp/alx/src/alx/Player.h)
* [`src/alx/DrawFixtures.cpp`](file:///Users/matt/code/cpp/alx/src/alx/DrawFixtures.cpp) (34.7 KB, 704 lines) / [`src/alx/DrawFixtures.h`](file:///Users/matt/code/cpp/alx/src/alx/DrawFixtures.h)
* [`src/alx/MainScene.h`](file:///Users/matt/code/cpp/alx/src/alx/MainScene.h#L152-L374) (or new `MainScene.cpp`)

---

## `[PH-EMMG]`: `EnemyManager` Monolithic Class Decomposition

### Identified Anti-Patterns
* `EnemyManager` acts as a God Object managing 5 separate domain entities (`Enemy`, `AlloyItem`, `WorldStructure`/DarkTower, `ShadowEgg`, `ManaSpark`).
* It handles AI state machines, wave spawner calculations, loot item magnetic physics, structure pulses, projectile collision checks, melee hit detection, and off-screen threat indicator rendering in one 1,085-line file.

### Action Plan & Sub-Tasks
* `[EMSPW]`: Extract wave spawner logic (`spawn_enemy_wave`, cluster searching, distance filtering) into a dedicated `EnemySpawner` class.
* `[EMLUT]`: Extract alloy loot item magnet physics and collection into a dedicated `LootSystem` class.
* `[EMSTR]`: Extract Dark Tower structure updates and pulse logic into a dedicated `StructureManager` class.
* `[EMIND]`: Extract threat indicator UI rendering into `ThreatIndicatorRenderer`.
* `[EMCTX]`: Retain `EnemyManager` as a thin coordinator delegating to these dedicated subsystems.

---

## `[PH-EAIU]`: `update_enemy_ai()` & `update_combat_and_loot()` SLAP Refactoring

### Identified Anti-Patterns
* `update_enemy_ai()` (lines 386–784) is a ~400-line method containing a massive nested `switch` statement for all `EnemyState` cases (`Wander`, `SeekTarget`, `AttackWindup`, `AttackRecoilRest`, `ChasePlayer`, `HitStun`). It mixes state transition policy with knockback deceleration math, blood particle emission, stuck timer counters, and fixture damage.
* `update_combat_and_loot()` (lines 821–973) spans 153 lines combining melee attack swipe collisions, entity death cleanup/loot scattering, and alloy collection.

### Action Plan & Sub-Tasks
* `[EAISL]`: Refactor `update_enemy_ai()` using the Composed Method pattern. Extract state handlers into private helper functions in `EnemyManager.cpp`:
  * `process_wander_state(Enemy& enemy, float dt)`
  * `process_seek_target_state(Enemy& enemy, float dt)`
  * `process_attack_windup_state(Enemy& enemy, float dt)`
  * `process_attack_recoil_state(Enemy& enemy, float dt)`
  * `process_chase_player_state(Enemy& enemy, float dt)`
  * `process_hit_stun_state(Enemy& enemy, float dt)`
* `[ECLSL]`: Decompose `update_combat_and_loot()` into three composed methods:
  * `process_player_melee_hits(Player& player, ParticleSystem* particles)`
  * `process_entity_deaths_and_loot(ParticleSystem* particles)`
  * `process_alloy_item_collection(Player& player, ParticleSystem* particles)`
* `[EESM]`: Encapsulate `Enemy` state transitions behind a controlled `Enemy::change_state(EnemyState new_state)` method that automatically resets timers and state invariants.

---

## `[PH-NETD]`: `Network` Fluid Simulation & Grid Decomposition

### Identified Anti-Patterns
* `Network.cpp` (922 lines) violates SRP by combining grid storage, fixture placement/removal, graph BFS distance pathfinding (`compute_distance_field`), fluid/power grid state machine simulation, and rendering delegation.
* `sim_pipe_flow()` (180 lines) mixes pipeline flow orchestration with vector sorting, downstream neighbor search, draining tail recession math, and discrete light mana orb stepping.

### Action Plan & Sub-Tasks
* `[NETGR]`: Decompose `Network` into focused single-responsibility classes:
  1. `NetworkGrid`: Manages 2D tile grid storage, boundary checks, fixture placement validation, and neighbor mask updates.
  2. `ManaDistanceField`: Encapsulates BFS distance field generation and distance caching for Seep and Spire endpoints.
  3. `ManaSimEngine`: Contains step-by-step fluid and power simulation logic (`sim_consume`, `sim_pipe_flow`, `sim_produce`).
* `[NETSL]`: Refactor `sim_pipe_flow()` into composed helper routines: `sim_dark_mana_flow()` and `sim_light_mana_flow()`, delegating node steps to `process_dark_pipe_node()` and `step_light_mana_orb()`.

---

## `[PH-PLYR]`: `Player` Responsibility Extraction & Build Placement System

### Identified Anti-Patterns
* `Player` mixes player movement physics, sprite animation selection, build placement raycasting/footprint search (`placement_fixture_center`, lines 217–266), attack arc trigonometric calculations, input buffering, and debug drawing within a single class.

### Action Plan & Sub-Tasks
* `[PLBPS]`: Extract build placement raycasting and footprint validation into a dedicated `BuildingPlacementSystem` struct/class.
* `[PLARC]`: Extract attack arc trigonometric calculations and collision box generation into a helper function `calculate_sword_swipe_arc()`.
* `[PLPNE]`: Move `m_pending_spark` to `private:` section and provide accessor methods.

---

## `[PH-DFIX]`: `DrawFixtures.cpp` Geometry vs FX Decoupling

### Identified Anti-Patterns
* `DrawFixtures.cpp` (34.7 KB, 704 lines) combines low-level primitive pixel shape rendering (pipes, refiners, spires, seeps) with particle emitter triggering (`spawn_refiner_embers`, `spawn_spire_embers`, `spawn_corner_pipe_mana`) and emission timer state (`s_emit_timer`).
* `pipe_dark_mana_corner()` (128 lines) has deeply nested `if/else if` ladders handling 4 distinct head/tail animation states.

### Action Plan & Sub-Tasks
* `[DFXDC]`: Separate primitive visual geometry rendering from particle FX emission. Move particle emission logic into a dedicated `FixtureFX` helper module.
* `[DFXSL]`: Refactor `pipe_dark_mana_corner()` into composed helper functions: `draw_corner_full()`, `draw_corner_head()`, and `draw_corner_tail()`.
* `[ZMNM]`: Replace raw rendering pixel offsets (`world_x + 6`, `world_y + 22`, `44`, `16`) with named constants in `FixtureRenderConstants`.

---

## Verification & Build Criteria

After completing Phase 3 edits:
1. Run syntax and type-safety verification:
   ```bash
   task build
   ```
2. Verify function metrics: target functions $\le 25$ lines or $\le 5$ statements.
3. Verify that cognitive complexity and nesting depth remain $\le 3$ levels across all AI state machine and simulation routines.
