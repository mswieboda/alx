# ENEMIES_PLAN.md - Enemy Combat, Weapon Mechanics & Loot Dropping

This document outlines the detailed roadmap, technical specifications, and design decisions for player weapon combat, enemy AI movement, hit reactions, fixture destruction, and alloy loot drops in *Aetherlux* (`alx`).

---

## Technical Specifications & Confirmed Architecture

### 1. Collision & Physics System
- **Map Wall Collisions**: Uses **Grid AABB Tile Sliding** ($O(1)$ corner tile checks) for fast, lightweight wall collision handling.
- **Entity & Fixture Interactions**: Uses **Generic Circle Radius Overlap**.
- **Enemy Push Separation**: Uses **Iterative Soft Circle Separation** (Position-Based Overlap Displacement) to cleanly push overlapping enemies apart during movement without complex physics engines.

### 2. Player Facing Vector, Health & Attack Mechanics
- **Player Facing Vector**: Tracks `facing_dx, facing_dy`. Supports 4 cardinal directions (`(0, -1)` Up, `(0, 1)` Down, `(-1, 0)` Left, `(1, 0)` Right) and 4 diagonal directions (`(±0.707, ±0.707)`). Updated on non-zero movement input.
- **Player Health & i-Frames**:
  - Max Health: **10 HP** (`max_hp = 10`, `current_hp = 10`).
  - Damage Invulnerability: **0.5s i-frames** upon taking damage. Flash feedback handled in Phase 6.
- **Hitbox Dimensions**: **16x8 px rectangle** (oriented along cardinal or diagonal facing direction):
  - Extends **8px** deep along facing vector.
  - Spans **16px** wide perpendicular to facing vector (centered on player).
- **Hitbox Timing**:
  - Active Hitbox Duration: **0.15s**.
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

### Phase 1: Generic Collision Radius Architecture
1. Implement collision radius for Player movement offset near feet (combining with AABB wall sliding).
2. Create generic circle-vs-circle and circle-vs-AABB collision math utilities for movement, hitboxes, hurtboxes, and fixtures.
3. Implement Fixture collision radius for entity blocking.
4. Implement Enemy soft circle push-separation algorithm for smooth crowd movement.
5. Implement Player/Enemy body hurt radius (`hurt_radius`).

### Phase 2: Player Melee Combat Refinement
1. Implement diagonal attack direction calculation alongside 4 cardinal directions.
2. Apply damage when player hit hitbox/radius collides with enemy hurt radius.
3. Implement enemy knockback (2px) and HP deduction (`max_hp = 3`).
4. Implement enemy defeat / removal and static Alloy Item drop at enemy position.

### Phase 3: Enemy Movement & Target Seeking
1. Implement basic temporary random small path looped movement (idle wandering).
2. Implement vector steering movement toward fixtures, prioritizing Refiners & Spires over Pipes, re-evaluating targets every 1.0s to 3.0s.
3. Implement player aggro interception (checked every 1.0s to 3.0s): chase player if within detection radius, and return to high-value fixture targeting if player escapes range.
4. Implement timed wave spawning near outer map wall floor tiles.

### Phase 4: Enemy Attacking & Fixture Destruction
1. Implement enemy melee attack against Player when within reach (0.3s attack windup, 1.0s attack cycle, 1 damage, triggering 0.5s i-frames).
2. Implement enemy attack against targeted Fixture when within reach (0.3s attack windup, 1.0s attack cycle, 1 damage).
3. Retargeting: Mid-attack switch to player if player enters smaller *Threat Radius*; return to target fixture if player escapes.
4. Implement Fixture health pools (**10 HP Pipes**, **30 HP Refiners/Spires**) and apply attack damage over time.
5. Implement Fixture destruction: clear cell, break mana network, trigger Dark Mana Spill cloud effect, and increase global Twilight level.
6. Implement Player defeat state at 0 HP (temporarily disable drawing and player input while scene runs).

### Phase 5: Directional Mana Spark Projectiles
1. Implement Hold Charge Timer (0.5s) on `Action::Attack`.
2. Spawn 4x4 directional projectile on charge completion / button release.
3. Implement helper method to identify solid tall fixtures (`FixtureType::Refiner` and `FixtureType::Spire`).
4. Handle projectile motion and collision against enemies, wall tiles, and tall fixtures (`Refiner`/`Spire`). (Walls and tall fixtures absorb/destroy projectile with no damage).

### Phase 6: Visual Polish & Game Juice
1. Implement 0.1s Enemy Hit Flash & Player i-frame damage flash.
2. Implement 1-tile Alloy Pickup Magnet attraction effect towards player.
3. Implement 30s Alloy Pickup despawn timer with flashing warning for the final 10 seconds.
4. Implement 2-frame Melee Arc visual swipe sprite/rect.
5. Render small fading purple cloud visual effect for Dark Mana Spills.

---

## Future Considerations & Design Iterations

- **Seep Node Spawning vs. Outer Wall Spawning**: Re-evaluate whether enemies should eventually spawn directly from Dark Seep nodes. *Design note*: Spawning directly from Seeps may undermine player wand visibility range and map panning mechanics, so outer wall wave spawning is preferred for now.
- **Wave Tuning**: Fine-tune wave spawn intervals, wave sizes, and Twilight-based difficulty scaling.
