# Combat, Enemy AI & Player Rites Roadmap

This document outlines the detailed roadmap and design decisions for player weapon combat, enemy AI movement, network targeting priorities, fixture destruction, player defeat state, alloy loot drops, and mana projectiles in *Aetherlux* (`alx`).

---

## Phased Implementation Roadmap

### [EP-EMBT]: Enemy Aggro, Network Targeting & Player Defeat Overhaul

#### [PH-EPRI]: Phase 4A - Enemy Network Priority & Threat Logic (IN-PROGRESS)
- [x] `[EPNW]`: Network-First Threat Hierarchy - Re-architect enemy target selection algorithm (`alx::Enemy`) to weight active mana conduits (`FixtureType::Pipe`, `Refiner`, `Spire`) 80% over the player. Enemies prioritize network targets and will ignore the player unless provoked or within a small chance threshold.
- [x] `[EPRN]`: Probabilistic Player Aggro Roll - Replace static aggressive player interception with a probabilistic roll (checked every 2.0s–4.0s). Enemies only attempt player aggro if within a 40px (~2.5 tiles) radius AND a 20% random aggro roll succeeds.
- [x] `[ERET]`: Provoked Retaliation & Aggro Decay - On taking player damage (`take_damage()`), the enemy becomes provoked, exiting `HitStun` directly into `EnemyState::ChasePlayer` to attack the player. If the player escapes beyond 48px (~3 tiles) OR 3.0s elapses without taking new damage, the enemy drops player aggro and returns to hunting network fixtures (`EnemyState::SeekTarget`).
- [ ] `[TSLC]`: Fixture Target Stickiness & Anti-Oscillation - Fix target switching bug where enemies switch targets too frequently while marching. Implement target lock timer (`static constexpr float TARGET_LOCK_DURATION = 5.0f;`) so enemies retain their designated network target unless destroyed or pathing is fully blocked.
- [ ] `[MWND]`: Combat Pathing & Reduced March Wandering - Reduce excessive wandering during `EnemyState::SeekTarget`. Increase direct line-of-sight tracking toward targeted network fixtures and reduce wander duration while near valid fixtures.

#### [PH-FBAL]: Phase 4B - Infrastructure Vulnerability & Fixture Balancing
* `[FDAM]`: Measured Fixture Damage - Adjust enemy melee damage against static fixtures (`FixtureType::Pipe`, `Refiner`, `Spire`) so enemies require 3–5 strikes to destroy a pipe segment, giving the player enough time to react while still creating localized repair needs.
* `[NSPL]`: Controlled Network Breach Spills - Re-verify dark mana spill cloud creation (`[DMPC]`) when a pipe is broken by an enemy attack. A pipe breach emits a Dark Mana Spill cloud that causes a +0.02 (+2%) Twilight surge in the room, creating tactical urgency without triggering instant loss loops.

#### [PH-PDRF]: Phase 4C - Player Defeat State & Ghost Mechanics
* `[PDSF]`: Player Defeat State Machine - Extend `PlayerState` to handle the defeat transition (`struct PlayerState: int hp = 3, max_hp = 3; float iframe_timer = 0.0f; bool is_defeated = false; float respawn_timer = 0.0f;`).
* `[GHTC]`: Viewport-Bound Ghost Movement & Gravestone - On player death at 0 HP, spawn a static gravestone marker (`Gravestone struct: float x, y;`) at the death coordinate. Transform player controls into a Ghost state with a 5.0s respawn timer. The camera remains locked to the death viewport location, and ghost movement is strictly constrained within the visible 320x240 camera viewport and blocked by solid wall tiles.
* `[RSTP]`: Respawn Sequence - Once the 5.0s timer expires, the player resurrects at full HP at the death site (or active spawn node), clearing the gravestone marker.

---

### [EP-PRJT]: Directional Mana Spark Rites Epic

#### [PH-PRJT]: Phase 5 - Directional Mana Spark Projectiles
* `[HCT]`: Hold Charge Timer - Implement 0.5s Hold Charge Timer on `Action::Attack`.
* `[DMSP]`: Directional Mana Spark Projectile - Spawn 4x4 directional projectile on charge completion / button release (`ManaSpark struct: float x, y, vx, vy; float lifetime = 2.0f; int damage = 2;`).
* `[STFH]`: Solid Tall Fixture Helper - Implement helper method to identify solid tall fixtures (`FixtureType::Refiner` and `FixtureType::Spire`).
* `[PMCL]`: Projectile Motion & Collision Loop - Motion and collision handling against enemies, wall tiles, and tall fixtures (`Refiner`/`Spire`). (Walls and tall fixtures absorb/destroy projectile with no damage).

---

### [EP-JUIC]: Visual Polish & Game Juice Epic

#### [PH-JUIC]: Phase 6 - Visual Polish & Game Juice
* `[HITF]`: Enemy & Player Hit Flashes - Implement 0.1s Enemy Hit Flash & Player i-frame damage flash.
* `[ALOP]`: Enemy Alloy Loot Drops - Upon defeat, enemies drop 1–3 Alloy pieces (`AlloyDrop struct: float x, y; float lifetime = 30.0f; bool flashing = false;`).
* `[AMAG]`: Alloy Pickup Magnet - Implement 1-tile Alloy Pickup Magnet attraction effect towards player.
* `[ADSP]`: Alloy Pickup Despawn Timer - Implement 30s Alloy Pickup despawn timer with flashing warning for the final 10 seconds.
* `[DMPC]`: Dark Mana Spill Cloud - Render small fading purple cloud visual effect for Dark Mana Spills.

---

### [EP-DTWR]: Dark Tower Spawning & Tuning Epic

#### [PH-DTWR]: Phase 7 - Dark Tower Emergence & Spawning Adjustments
* `[DTBS]`: Dark Tower Spawn Rate & Pulse Relaxation - Relax Dark Tower emergence and pulse parameters in `DarkTowerConstants`:
  - Increase `SPAWN_INTERVAL_MIN` from 6.0s to 8.0s to give players a wider window between egg spawns.
  - Extend `PULSE_INTERVAL_MIN` from 7.0s–15.0s to 10.0s–18.0s to reduce rapid Twilight buildup.

---

## Future Considerations & Design Iterations

* `[PEPB]`: Purification Enrage & Rubber-Band Equilibrium - As players refine dark mana into light energy (reducing room Twilight level from default ~90% down toward 0%), dark entities become threatened and enraged by the cleansing of their domain (wander duration drops, target march duration increases, and aggro radius expands slightly, scaled moderately so it does not become overwhelming). Conversely, if pipes break and dark mana spills back into the room (raising Twilight), enemies temporarily revert toward wandering, acting as a rubber-band recovery mechanism so players can repair broken conduits without facing instant death-spiral game-overs.
