# ENEMIES_PLAN.md - Enemy Combat, Weapon Mechanics & Loot Dropping

This document outlines the detailed roadmap, technical specifications, and design decisions for player weapon combat, enemy AI movement, hit reactions, fixture destruction, and alloy loot drops in *Aetherlux* (`alx`).

---

## Technical Specifications & Confirmed Architecture

### 1. Collision & Physics System
- **Map Wall Collisions**: Uses **Grid AABB Tile Sliding** ($O(1)$ corner tile checks) for fast, lightweight wall collision handling.
- **Entity & Fixture Interactions**: Uses **Circle-vs-Circle Overlap** for entity-vs-entity movement and **Circle-vs-AABB Ground Rectangles** for solid structure blocking (Refiners, Spires, Seeps), allowing 2.5D depth where characters can walk behind the top of structures.
- **Enemy Push Separation**: Uses **Iterative Soft Circle Separation** (Position-Based Overlap Displacement) to cleanly push overlapping enemies apart during movement without complex physics engines.

### 2. Player Facing Vector, Health & Attack Mechanics
- **Player Facing Vector**: Tracks `facing_dx, facing_dy`. Supports 4 cardinal directions (`(0, -1)` Up, `(0, 1)` Down, `(-1, 0)` Left, `(1, 0)` Right) and 4 diagonal directions (`(±0.707, ±0.707)`). Updated on non-zero movement input.
- **Player Health & i-Frames**:
  - Max Health: **10 HP** (`max_hp = 10`, `current_hp = 10`).
  - Damage Invulnerability: **0.5s i-frames** upon taking damage. Flash feedback handled in Phase 6.
- **Attack Hit Circle**: Uses a **Circle** (`radius = 8.0px`) for 8-directional circle-vs-circle hit detection:
  - Center offset: `cx = player.center_x() + facing_dx * 8.0f`, `cy = player.center_y() + facing_dy * 8.0f`.
  - Supports 4 cardinal directions and 4 diagonal directions (`(±0.707, ±0.707)`).
- **Hit Circle Timing**:
  - Active Hit Circle Duration: **0.15s**.
  - Total Attack Cooldown: **0.25s** (~4 swipes/sec max rate).

### 3. Enemy AI, Movement & Spawning System
- **Movement Engine**: **Vector Steering** (direct target directional vector + AABB wall sliding + soft circle enemy-vs-enemy push separation).
- **Target Prioritization**: Prioritizes high-value infrastructure targets (**Refiners and Spires**) over standard **Pipes**. If no high-value targets exist, targets the nearest Pipe.
- **Aggro Interception**: Overrides fixture targeting and chases the Player if the Player enters the enemy's *Detection Radius*.
- **Wave Spawning**: Enemies spawn in periodic timed wave batches near outer map walls (similar to initial spawn placements).

### 4. Enemy HP, Attack Windup & Impact Reactions
- **Enemy Health**: 3 HP (`max_hp = 3`, `current_hp = 3`).
- **Hit Knockback**: Pushed **2px** backward along the attack vector when hit.
- **Attack Cadence & Windup**:
  - Attack Rate: **1 attack per second** (1 DPS).
  - Attack Windup: Enemies stop and execute a **0.3s attack windup** before applying damage to either Player or Fixtures.

### 5. Fixture Health, Destruction & Dark Mana Spills
- **Fixture Health Pools**:
  - **Pipes**: **10 HP** (requires 10s of continuous enemy attacking to destroy).
  - **Refiners & Spires**: **30 HP** (requires 30s of continuous enemy attacking to destroy).
- **Structure Defenses**: Fixtures are purely passive targets with 0 self-defense capabilities; the Player is the sole line of defense.
- **Dark Mana Spills**: When an enemy destroys a Pipe or Refiner (or when dark mana decays after N ticks), dark mana spills into the room:
  - Increases global Twilight darkness level (`m_twilight_level`).
  - Dark mana packet vanishes and spawns a small temporary purple cloud circle that rapidly fades out.

### 6. Cursed Alloy Loot Drops & Lifespan
- **On Defeat**: Enemies drop **1 static Cursed Alloy Item Pickup** at the enemy's floor tile position.
- **Collection**: Player stepping on the item tile collects +1 Alloy and removes the pickup.
- **Despawn Timer**: Pickups persist for **30 seconds** total before despawning.
- **Despawn Warning**: Pickups flash visual warning during their final **10 seconds** (seconds 20–30).

---

## Phased Implementation Roadmap

### Phase 1: Generic Collision Radius Architecture [COMPLETED]
1. Implement collision radius for Player movement offset near feet (combining with AABB wall sliding). [COMPLETED]
2. Create generic circle-vs-circle and circle-vs-AABB collision math utilities for movement, hitboxes, hurtboxes, and fixtures. [COMPLETED]
3. Implement Fixture collision radius for entity blocking. [COMPLETED]
4. Implement Enemy soft circle push-separation algorithm for smooth crowd movement. [COMPLETED]
5. Implement Player/Enemy body hurt radius (`hurt_radius`). [COMPLETED]

### Phase 2: Player Melee Combat Refinement [COMPLETED]
1. Implement diagonal attack direction calculation alongside 4 cardinal directions. [COMPLETED]
2. Apply damage when player hit hitbox/radius collides with enemy hurt radius. [COMPLETED]
3. Implement enemy knockback (2px) and HP deduction (`max_hp = 3`). [COMPLETED]
4. Implement enemy defeat / removal and static Alloy Item drop at enemy position. [COMPLETED]

### Phase 3: Enemy Movement & Target Seeking
1. Implement basic temporary random small path looped movement (idle wandering). [COMPLETED]
2. Implement vector steering movement toward fixtures, prioritizing Refiners & Spires over Pipes, re-evaluating targets every 1.0s to 3.0s. [COMPLETED]
  1. **Spawn Orientation & Post-Destruction Wandering**: On spawn (and upon destroying a targeted fixture), enemies use the wander state for 5.0s before target-locking a fixture (telegraphs spawns and prevents rapid pipe-chain wipes using shared constants/mechanics). [COMPLETED]
  2. **Siege & Restless Wander Cycle**: Once targeting a fixture, mobs march/attack for 6.0s–8.0s, then enter a 2.0s–3.0s restless wander state to re-evaluate surrounding targets and prevent mechanical mob stacking. [COMPLETED]
  3. **Obstacle Detour Handling**: If an enemy is blocked by any obstacle (wall tiles, fixtures, or impassable terrain) for > 1.5s without forward progress, automatically trigger a 3.0s wander state to break free and maneuver around obstructions. [COMPLETED]
3. Implement grouped enemy spawn waves, spawning near each other in a tighter group, rather than all randomly around the outer walls. [COMPLETED]
4. Implement player aggro interception (checked every 1.0s to 3.0s): chase player if within detection radius, and return to high-value fixture targeting if player escapes range. [COMPLETED]
5. Threat indicators should no longer track all enemies, they should only track when any fixture is being damaged. once a fixture is damaged it should add an indicator, and pulse/flash red on/off and have a timer for like 1.5s. any further damage resets the timer. once timer is off, the only way to determine a damaged fixture is to look at the graphics (different sprite frames), but for now we will interpolate it towards red in damaged increments, etc.

### Phase 4: Enemy Attacking & Fixture Destruction
1. Implement enemy melee attack against Player when within reach (0.3s attack windup, 1.0s attack cycle, 1 damage, triggering 0.5s i-frames).
2. Implement enemy attack against targeted Fixture when within reach (0.3s attack windup, 1.0s attack cycle, 1 damage). [COMPLETED]
3. Retargeting: Mid-attack switch to player if player enters smaller *Threat Radius*; return to target fixture if player escapes. [COMPLETED]
4. Implement Fixture health pools (**10 HP Pipes**, **30 HP Refiners/Spires**) and apply attack damage over time. [COMPLETED]
5. Implement Fixture destruction: clear cell, break mana network, trigger Dark Mana Spill cloud effect, and increase global Twilight level. [COMPLETED]
6. Implement Player defeat state at 0 HP (temporarily disable drawing and player input while scene runs).

### Phase 5: Improved Player Melee
Here is the updated implementation roadmap with Phase 1 combined, LaTeX completely stripped out in favor of C++ pseudo-code, and the requested list formatting.

#### Sub Phase 1: Static Circle Conversion & Debug Visualizer

1. **Static Circle Mechanics & 2px Outlined Debug Rendering**
* **Goal:** Convert the instant attack from temporary rect math/rendering to a true circle-based hitbox drawn on-screen with a 2px outline.
* Swap out the temporary debug rect drawing call for your 2px inner-outlined circle renderer.
* Define the circle parameters (`center_x`, `center_y`, and `radius`) based on the player's current facing direction.
* Compute directional offsets using pure integer lookup (e.g., Cardinal offset = `(16, 0)`, Diagonal offset = `(11, 11)`).
* Replace rect collision checks with integer-only circle distance checks:
```cpp
// Pure integer distance check (no float, no sqrt)
int dx = attack_cx - enemy_cx;
int dy = attack_cy - enemy_cy;
int combined_r = attack_radius + enemy_hurt_radius;

bool is_hit = (dx * dx + dy * dy) <= (combined_r * combined_r);

```

* Add a simple `hit_enemies[]` boolean flag array per attack state so an enemy only takes damage once per swing.


#### Sub Phase 2: Time-Based Arc Motion (The Moving Swept Circle)

1. **Attack State Timer & Progress Tracking**
* **Goal:** Transform the single-frame instant attack into a multi-frame timed state machine.
* Add `swing_timer` and `swing_duration` (e.g., 6 frames) to the player's state.
* Calculate normalized progress `t` during update ticks:
```cpp
float progress = (float)swing_timer / (float)swing_duration; // 0.0f to 1.0f

```

* Manage attack transitions (`IDLE` -> `ACTIVE_SWEEP` -> `RECOVERY` -> `IDLE`).


2. **Arc Trajectory & Dynamic Center Lookup**
* **Goal:** Sweep the attack circle center along an arc dynamically using your pre-computed fixed-point Trig LUT.
* Map progress `t` to an angle offset relative to facing direction (e.g., -45 deg to +45 deg).
* Compute `current_angle = (facing_angle + angle_offset) % 360`.
* Look up cosine and sine values in your LUT to place the circle center:
```cpp
int cx = player_x + ((reach_radius * cos_lut[current_angle]) >> 12);
int cy = player_y + ((reach_radius * sin_lut[current_angle]) >> 12);

```


#### Sub Phase 3: Edge Case Polish & Sub-Stepping

1. **Anti-Tunneling via Sub-Stepping**
* **Goal:** Ensure ultra-fast sword swings don't skip over enemies positioned directly in front of the player.
* For fast swings (e.g., 2 frames covering 180 deg), run the collision check 2 to 3 times per frame update:
```cpp
// Sub-step loop within a single logic tick
for (int step = 0; step < SUB_STEPS; ++step) {
    float sub_t = progress + ((float)step / SUB_STEPS) * delta_t;
    // Calculate (cx, cy) for sub_t and test enemy collision
}

```


2. **Visual Decoupling & Debug Toggle**
* **Goal:** Clean up the production rendering while keeping debug tools available.
* Bind a debug toggle key to show/hide the 2px outlined circle overlay.
* Align weapon swing particles or visual sprite trails directly to the dynamic `(cx, cy)` trajectory points.

### Phase 6: Directional Mana Spark Projectiles
1. Implement Hold Charge Timer (0.5s) on `Action::Attack`.
2. Spawn 4x4 directional projectile on charge completion / button release.
3. Implement helper method to identify solid tall fixtures (`FixtureType::Refiner` and `FixtureType::Spire`).
4. Handle projectile motion and collision against enemies, wall tiles, and tall fixtures (`Refiner`/`Spire`). (Walls and tall fixtures absorb/destroy projectile with no damage).

### Phase 7: Visual Polish & Game Juice
1. Implement 0.1s Enemy Hit Flash & Player i-frame damage flash.
2. Implement 1-tile Alloy Pickup Magnet attraction effect towards player.
3. Implement 30s Alloy Pickup despawn timer with flashing warning for the final 10 seconds.
4. Implement 2-frame Melee Arc visual swipe sprite/rect.
5. Render small fading purple cloud visual effect for Dark Mana Spills.

---

## Future Considerations & Design Iterations

- **Seep Node Spawning vs. Outer Wall Spawning**: Re-evaluate whether enemies should eventually spawn directly from Dark Seep nodes. *Design note*: Spawning directly from Seeps may undermine player wand visibility range and map panning mechanics, so outer wall wave spawning is preferred for now.
- **Wave Tuning**: Fine-tune wave spawn intervals, wave sizes, and Twilight-based difficulty scaling.
- **Purification Enrage & Rubber-Band Equilibrium**: As players refine dark mana into light energy (reducing room Twilight level from default ~90% down toward 0%), dark entities become threatened and enraged by the cleansing of their domain (wander duration drops, target march duration increases, and aggro radius expands slightly). Conversely, if pipes break and dark mana spills back into the room (raising Twilight), enemies temporarily revert toward wandering, acting as a rubber-band recovery mechanism so players can repair broken conduits without facing instant death-spiral game-overs.
