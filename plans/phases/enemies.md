# Combat, Enemy AI & Player Rites Roadmap

This document outlines the detailed roadmap and design decisions for player weapon combat, enemy AI movement, hit reactions, fixture destruction, alloy loot drops, and mana projectiles in *Aetherlux* (`alx`).

---

## Phased Implementation Roadmap

### [EP-EATK]: Enemy Player Aggro & Melee Attacks Epic

#### [PH-EATK]: Phase 4: Enemy Attacking Player & Threat Logic
* `[EPAT]`: Enemy Player Aggro Interception - Chase player if within detection radius (checked every 1.0s to 3.0s), returning to fixture targeting if player escapes.
* `[EMAT]`: Enemy Melee Attack - Melee attack against player when within reach (0.3s attack windup, 1.0s attack cycle, 1 damage, triggering 0.5s i-frames).
* `[ERET]`: Mid-Attack Retargeting - Mid-attack switch to player if player enters smaller Threat Radius; return to target fixture if player escapes.
* `[PDFS]`: Player Defeat State - Implement Player defeat state at 0 HP (`PlayerState struct: int hp = 3, max_hp = 3; float iframe_timer = 0.0f; bool defeated = false;`).

---

### [EP-PRJT]: Directional Mana Spark Rites Epic

#### [PH-PRJT]: Phase 6: Directional Mana Spark Projectiles
* `[HCT]`: Hold Charge Timer - Implement 0.5s Hold Charge Timer on `Action::Attack`.
* `[DMSP]`: Directional Mana Spark Projectile - Spawn 4x4 directional projectile on charge completion / button release (`ManaSpark struct: float x, y, vx, vy; float lifetime = 2.0f; int damage = 2;`).
* `[STFH]`: Solid Tall Fixture Helper - Implement helper method to identify solid tall fixtures (`FixtureType::Refiner` and `FixtureType::Spire`).
* `[PMCL]`: Projectile Motion & Collision Loop - Motion and collision handling against enemies, wall tiles, and tall fixtures (`Refiner`/`Spire`). (Walls and tall fixtures absorb/destroy projectile with no damage).

---

### [EP-JUIC]: Visual Polish & Game Juice Epic

#### [PH-JUIC]: Phase 7: Visual Polish & Game Juice
* `[HITF]`: Enemy & Player Hit Flashes - Implement 0.1s Enemy Hit Flash & Player i-frame damage flash.
* `[AMAG]`: Alloy Pickup Magnet - Implement 1-tile Alloy Pickup Magnet attraction effect towards player.
* `[ADSP]`: Alloy Pickup Despawn Timer - Implement 30s Alloy Pickup despawn timer with flashing warning for the final 10 seconds (`AlloyDrop struct: float x, y; float lifetime = 30.0f; bool flashing = false;`).
* `[DMPC]`: Dark Mana Spill Cloud - Render small fading purple cloud visual effect for Dark Mana Spills.

---

## Future Considerations & Design Iterations

* `[PEPB]`: Purification Enrage & Rubber-Band Equilibrium - As players refine dark mana into light energy (reducing room Twilight level from default ~90% down toward 0%), dark entities become threatened and enraged by the cleansing of their domain (wander duration drops, target march duration increases, and aggro radius expands slightly). Conversely, if pipes break and dark mana spills back into the room (raising Twilight), enemies temporarily revert toward wandering, acting as a rubber-band recovery mechanism so players can repair broken conduits without facing instant death-spiral game-overs.
