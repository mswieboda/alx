# Bug Fix Plan: Egg Spawn & Out-of-Bounds Enemy Resolution

## Summary
Fixed Dark Tower Shadow Egg spawning into walls/fixtures/out-of-bounds and resolved enemies getting stuck outside map boundaries when attempting to return inside.

---

### `[EP-OOBE]`: Epic - Out-of-Bounds Enemy & Shadow Egg Spawn Remediation

#### `[PH-ESV]`: Phase 1 - Shadow Egg Validation & Ejection Safety (COMPLETED)
- [x] `[SEVC]`: **Comprehensive Egg Landing Validation** - Updated candidate landing check in [EnemyManager.cpp](file:///Users/matt/code/cpp/alx/src/alx/EnemyManager.cpp) to strictly enforce map bounds (`[0, map_w - ShadowEgg::EGG_WIDTH]` $\times$ `[0, map_h - ShadowEgg::EGG_HEIGHT]`), open floor ground checks, no overlap with Dark Tower AABB, player ground circle, active enemies, or existing eggs.
- [x] `[SERF]`: **Remove Force-Spawn Fallback** - Removed unvalidated fallback in [EnemyManager.cpp](file:///Users/matt/code/cpp/alx/src/alx/EnemyManager.cpp). If all 16 ring search attempts fail, egg spawning is cleanly aborted instead of force-spawning inside solid objects.
- [x] `[EHES]`: **Safe Hatch Ejection Check** - Added 4-way direction nudge validation upon egg hatching in [EnemyManager.cpp](file:///Users/matt/code/cpp/alx/src/alx/EnemyManager.cpp). If a player built a fixture over an incubating egg, hatching safely nudges the enemy onto open ground.

#### `[PH-OOBM]`: Phase 2 - Boundary Collision & Return-To-Map Resolution (COMPLETED)
- [x] `[OOBG]`: **Strict Out-of-Bounds Wall Treatment** - Updated [WorldCollision.cpp](file:///Users/matt/code/cpp/alx/src/alx/WorldCollision.cpp) so out-of-bounds tile coordinates return `is_solid_ground() == true`. Prevented knockback physics and enemy push separation from shoving enemies past outer map boundaries.
- [x] `[RMNW]`: **ReturnToMap Wall Phase-Through & Timeout** - Updated [EnemyManager.cpp](file:///Users/matt/code/cpp/alx/src/alx/EnemyManager.cpp) so enemies in `EnemyState::ReturnToMap` phase through outer perimeter wall tiles toward the inner map bounds, and added a 3.0s safety timeout (`RETURN_TO_MAP_TIMEOUT` in [Enemy.h](file:///Users/matt/code/cpp/alx/src/alx/Enemy.h)) to emergency-teleport any stuck out-of-bounds enemies back into playable map bounds.
