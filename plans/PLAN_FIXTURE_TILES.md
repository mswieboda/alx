# Plan: Multi-Tile Fixture Buildings & Connection Ports

## [EP-FTMB]: Multi-Tile Fixture Buildings & Port Network Epic
This epic refines multi-tile fixture building footprints (**Refiner: 3x3**, **Spire: 2x3**, **Seep: 3x2**), fixes world collision and player self-placement rules, removes x-ray transparency fading, implements strict perimeter connection ports, and updates 2.5D roof skylight frame rendering.

---

## [PH-COLR]: Phase 1: World Collision & X-Ray Removal (COMPLETE)

### [ROOTC]: 1. Multi-Tile Root AABB Resolution in World Collision
- `[ROOTC]`: Root Position Resolution - Update `WorldCollision::is_solid_ground` and `Network::is_solid` so querying any child tile `(tx, ty)` of a multi-tile building resolves to its root tile `(root_tx, root_ty)` before computing `fixture_ground_aabb`.
- `[COLL]`: Solid Footprints - Ensure player and enemy ground circles collide against the entire multi-tile ground AABB for Refiners (3x3), Spires (2x3), and Seeps (3x2).

### [NOXR]: 2. Complete Removal of X-Ray Transparency Fade
- `[NOXR]`: Remove X-Ray Fade - Delete `is_behind_tile` calculations and `BUILDING_ALPHA_FADED` (0x66000000) blending from `Network::draw` and `DrawFixtures::building`. Buildings draw at 100% opaque `0xFF000000` alpha at all times.

---

## [PH-PORT]: Phase 2: Exact Connection Ports & Seep 3x2 Update (COMPLETE)

### [RFPT]: 1. Refiner Connection Ports (3x3 Tiles / 48x48 px)
- `[RFPT]`: 4 Junction Ports - Restrict Refiner pipe connection interfaces to 4 exact perimeter positions:
  - **Middle Left**: West face at `(root_x, root_y + 1)`. Render side connector rect.
  - **Middle Right**: East face at `(root_x + 2, root_y + 1)`. Render side connector rect.
  - **Middle Top**: North face at `(root_x + 1, root_y)`. Render top connector circle.
  - **Middle Bottom**: South face at `(root_x + 1, root_y + 2)`. Render bottom connector circle.

### [SPPT]: 2. Spire Connection Port (2x3 Tiles / 32x48 px)
- `[SPPT]`: 1 Junction Port - Restrict Spire pipe connection to 1 exact position:
  - **Bottom Center**: South face at `(root_x + 1, root_y + 2)` (or `root_x, root_y + 2`). Render bottom connector circle.

### [SEEP]: 3. Seep Footprint Update (3x2 Tiles / 48x32 px) & Ports
- `[SEEP]`: 3x2 Footprint - Update Seep footprint from 2x2 to **3x2 tiles** (48x32 px).
- `[SPORTS]`: Top & Bottom Ports Only - Allow pipe connections only at **Top Center** `(root_x + 1, root_y)` and **Bottom Center** `(root_x + 1, root_y + 1)`. Render connector circles at top/bottom.
- `[NOBLD]`: Seep No-Build Zone - Prevent placing any fixture on top of any of the 6 tiles of a 3x2 Seep.

---

## [PH-RROUT]: Phase 3: Round-Robin Fixture Output Dispatcher (COMPLETE)
- `[RROUT]`: Round-Robin Output - Update fixture output dispatching for Seeps, Refiners, and future production buildings to rotate output port selection using `last_dir_output_idx` round-robin memory, ensuring even distribution across multiple connected output pipes.

---

## [PH-PLAC]: Phase 4: Keyboard/Gamepad Placement Safety Rules (COMPLETE)

### [PLSF]: 1. Multi-Tile Self-Placement Protection
- `[PLSF]`: Player AABB Guard - Update `Player::try_build_tile` to compute `fixture_ground_aabb` for the selected fixture's full multi-tile footprint (`w x h`).
- `[REJT]`: Self-Overlap Rejection - Reject building placement if any part of the proposed multi-tile ground AABB intersects the player's ground circle.

### [SPMP]: 2. Stop Player Movement Direction While Placing
- `[SPMP]`: Strafe While Building - Lock `facing_dx`/`facing_dy` while holding `Action::Build` so player can strafe around without shifting placement cursor facing direction.

---

## [PH-SKYL]: Phase 5: 2.5D Roof Skylight Frame & Layered Interior Rendering

### [RFFR]: 1. Roof Skylight Frame Assembly
- `[RFFR]`: Framed Roof Structure - Render top/left/right/bottom rects forming a hollow roof frame over the middle row, creating a natural open skylight window.
- `[LAYR]`: Interior Layering - Order rendering: Base Cutout BG -> Mana Liquid Pool / Aura -> Emitter Embers -> Roof Frame Bevels.

---

## Phased Execution Roadmap & Verification Steps

| Phase | Description | Status | Verification Method |
| --- | --- | --- | --- |
| `[PH-COLR]` | Fix Multi-Tile Collision & Remove X-Ray Fade | Complete | Player/Enemies blocked by buildings; zero alpha fading |
| `[PH-PORT]` | Exact Connection Ports & Seep 3x2 Update | Complete | Pipes connect only at middle/top/bottom ports |
| `[PH-RROUT]`| Round-Robin Fixture Output Dispatcher | Complete | Fixtures rotate output port selection evenly |
| `[PH-PLAC]` | Player Self-Overlap Placement Guard | Complete | Player cannot build structures on top of self; strafing while placing supported |
| `[PH-SKYL]` | 2.5D Roof Skylight Frame & Interior Layers | Pending | Roof cutout window shows internal mana & embers |
