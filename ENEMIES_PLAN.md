# ENEMIES_PLAN.md - Enemy Combat, Weapon Mechanics & Loot Dropping

This document outlines the detailed roadmap, technical specifications, and open design questions for player weapon combat, enemy hit reactions, and alloy loot drops in *Aetherlux* (`alx`).

---

## Technical Specifications (From /grill-me Alignment)

### 1. Player Facing Vector & Attack Hitbox
- **Player Facing Vector**: Tracks `facing_dx, facing_dy` (`(0, -1)` Up, `(0, 1)` Down, `(-1, 0)` Left, `(1, 0)` Right) updated on non-zero movement input.
- **Hitbox Dimensions**: **16x8 px rectangle**:
  - Extends **8px** deep along the facing direction.
  - Spans **16px** wide perpendicular to the facing direction (centered on the player).
- **Hitbox Timing**:
  - Active Hitbox Duration: **0.15s**.
  - Total Attack Cooldown: **0.25s** (~4 swipes/sec max rate).

### 2. Enemy HP & Impact Reactions
- **Enemy Health**: 3 HP (`max_hp = 3`, `current_hp = 3`).
- **Hit Knockback**: Pushed **2px** backward along the attack vector when hit.
- **Hit Flash**: *(Tabled for Phase 3)* Flash white/red for `0.1s` upon receiving damage.

### 3. Cursed Alloy Loot Drops
- **On Defeat**: Enemies drop **1 static Cursed Alloy Item Pickup** at the enemy's floor tile position.
- **Collection**: Player stepping on the item tile collects +1 Alloy and removes the item pickup.
- **Magnet Juice**: *(Tabled for Phase 3)* Magnetize items toward the player within 1 tile.

---

## Phased Implementation Roadmap

### Phase 1: Melee Swipe & Alloy Item Drops (Immediate)
1. Add `facing_dx, facing_dy` tracking to `Player`.
2. Implement `Enemy` 3 HP system and 2px knockback.
3. Implement Melee Arc Swipe (16x8 px frontal hitbox, 0.15s active, 0.25s cooldown).
4. Implement `AlloyItem` pickup struct/manager on static floor positions with walk-over collection.

### Phase 2: Directional Mana Spark Projectiles
1. Implement Hold Charge Timer (0.5s) on `ActionBtn`.
2. Spawn 4x4 directional projectile on charge completion.
3. Handle projectile motion and collision against enemies.

### Phase 3: Visual Polish & Juice
1. Implement 0.1s Enemy Hit Flash.
2. Implement 1-tile Alloy Pickup Magnet attraction effect.
3. Implement 2-frame Melee Arc visual swipe sprite/rect.

---

## Open Design Questions

- **Resource Consumption**: Should attacks eventually consume dark mana from connected pipes or increase room darkness on cast?
- **Charge Visual FX**: Should holding the wand show a charging particle/glow on the player before releasing the projectile?
