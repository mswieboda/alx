This document outlines the detailed roadmap, and design decisions for player weapon combat, enemy AI movement, hit reactions, fixture destruction, and alloy loot drops in *Aetherlux* (`alx`).

---

## Phased Implementation Roadmap

(previous phases and subtasks completed, removed for brevity)

### Phase 4: Enemy Attacking Player
1. Implement enemies having player aggro interception (checked every 1.0s to 3.0s): chase player if within detection radius, and return to high-value fixture targeting if player escapes range. [WIP - was disabled or removed]
1. Implement enemy melee attack against Player when within reach (0.3s attack windup, 1.0s attack cycle, 1 damage, triggering 0.5s i-frames).
2. Retargeting: Mid-attack switch to player if player enters smaller *Threat Radius*; return to target fixture if player escapes. [WIP - was disabled or removed]
3. Implement Player defeat state at 0 HP (temporarily disable drawing and player input while scene runs).

### Phase 6: Directional Mana Spark Projectiles
1. Implement Hold Charge Timer (0.5s) on `Action::Attack`.
2. Spawn 4x4 directional projectile on charge completion / button release.
3. Implement helper method to identify solid tall fixtures (`FixtureType::Refiner` and `FixtureType::Spire`).
4. Handle projectile motion and collision against enemies, wall tiles, and tall fixtures (`Refiner`/`Spire`). (Walls and tall fixtures absorb/destroy projectile with no damage).

### Phase 7: Visual Polish & Game Juice
1. Implement 0.1s Enemy Hit Flash & Player i-frame damage flash.
2. Implement 1-tile Alloy Pickup Magnet attraction effect towards player.
3. Implement 30s Alloy Pickup despawn timer with flashing warning for the final 10 seconds.
4. Render small fading purple cloud visual effect for Dark Mana Spills.

---

## Future Considerations & Design Iterations

- **Dark Tower Spawning vs. Outer Wall Spawning**: Re-evaluate whether enemies should eventually spawn directly from to-be-implemented new Dark Tower nodes.
- **Wave Tuning**: Fine-tune wave spawn intervals, wave sizes, and Twilight-based difficulty scaling.
- **Purification Enrage & Rubber-Band Equilibrium**: As players refine dark mana into light energy (reducing room Twilight level from default ~90% down toward 0%), dark entities become threatened and enraged by the cleansing of their domain (wander duration drops, target march duration increases, and aggro radius expands slightly). Conversely, if pipes break and dark mana spills back into the room (raising Twilight), enemies temporarily revert toward wandering, acting as a rubber-band recovery mechanism so players can repair broken conduits without facing instant death-spiral game-overs.
