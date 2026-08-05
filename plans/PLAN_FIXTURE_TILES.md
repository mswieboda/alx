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

## [PH-SKYL]: Phase 5: 2.5D Roof Skylight Frame & Layered Interior Rendering (COMPLETE)

### [RFSR]: 1. Refiner 2.5D Vaulted Skylight Assembly (3x3 Tiles / 48x48 px)
- `[RFGE]`: Cutout & Front Windows - Define an expanded top-reaching interior skylight cutout at `(world_x + 6, world_y + 6, 36, 22)` and dual $6 \times 6\text{ px}$ front face windows at `(world_x + 6, world_y + 37)` and `(world_x + 36, world_y + 37)`.
- `[RFFR]`: Vaulted Frame & Aligned Base - Extend middle section body rect up to `world_y + 4` with 4px borders on all sides; align bottom base width to 44px (`world_x + 2` to `world_x + 46`) matching middle section.
- `[RFTM]`: Refiner Timing Constants - Renamed `REFINER_TICKS_REQUIRED` to `REFINER_PROCESSING_TICKS_REQUIRED = 5` and added `REFINER_CONSUMING_WAIT_TICKS = 1` in `Game.h` to enforce a post-production wait delay before consuming the next dark mana input.
- `[RFAN]`: Dynamic 3-Phase Fill & Drain Cycle - Animate dark mana pool and front face windows through 3 phases: 1) Intake Fill ($0\% \to 100\%$) during `REFINER_CONSUMING_WAIT_TICKS`, 2) Active Refining ($100\%$ full with embers), and 3) Output Drain ($100\% \to 0\%$) on the final processing tick (`REFINER_PROCESSING_TICKS_REQUIRED`) as Light Mana is produced.




### [SPSR]: 2. Spire Vertical Crystal Chamber Assembly (2x3 Tiles / 32x48 px)
- `[SPGE]`: Chamber Cutout Geometry - Define a centered 16x16 px interior skylight cutout at `(world_x + 8, world_y + 16)`.
- `[SPFR]`: Emerald Pillar Frame - Render dual 4px vertical side pillars (`world_x + 4` and `world_x + 24`) and top/bottom bevel borders framing the inner crystal chamber.

### [LAYR]: 3. Strict 5-Stage Inset Interior Layering Order
- `[L1BG]`: Stage 1: Recess Pit Floor BG - Render deep dark floor background rect (`0xFF120A2A` Refiner, `0xFF002810` Spire) inside cutout window bounds.
- `[L2MN]`: Stage 2: Static Flat Mana Pool - Render solid dark mana pool (`0xFF4A0088` pipe dark mana base) or light mana cyan aura (`0xFF00FFFF`) + white core (`0xFFFFFFFF`) with static flat fluid rects matching current mana state.
- `[L3EM]`: Stage 3: Interior Emitter Embers - Spawn embers (`spawn_refiner_embers` / `spawn_spire_embers`) inside interior window bounds before roof frame draw.
- `[L4BV]`: Stage 4: Roof Frame & Bevel Edge Shadows - Render outer frame walls, side pillars, front face dual $6 \times 6\text{ px}$ windows showing dark mana, and 1px inner bevel edge shadow rects over liquid pool edges for inset depth.
- `[L5HL]`: Stage 5: Top Cap Highlights & Roof Vent Vanes - Draw top roof cap highlights (`0xFF7B4CE3` Refiner, `0xFF88FFCC` Spire tip) and roof vent vanes (`0xFF241454`).

### [CLPAL]: 4. High-Contrast Twilight Color Tokens & Shading Specs
- `[RFCLR]`: Refiner Palette Specs - Body: `0xFF341C66`, Foundation Base: `0xFF1F1240`, Pit Floor: `0xFF120A2A`, Pipe Dark Mana Base: `0xFF4A0088`.
- `[SPCLR]`: Spire Palette Specs - Spire Tip: `0xFF88FFCC`, Crystal Peak: `0xFF00FF88`, Shaft: `0xFF00A350`, Foundation Base: `0xFF004520`, Pit Floor: `0xFF002810`.

---

## Phased Execution Roadmap & Verification Steps

| Phase | Description | Status | Verification Method |
| --- | --- | --- | --- |
| `[PH-COLR]` | Fix Multi-Tile Collision & Remove X-Ray Fade | Complete | Player/Enemies blocked by buildings; zero alpha fading |
| `[PH-PORT]` | Exact Connection Ports & Seep 3x2 Update | Complete | Pipes connect only at middle/top/bottom ports |
| `[PH-RROUT]`| Round-Robin Fixture Output Dispatcher | Complete | Fixtures rotate output port selection evenly |
| `[PH-PLAC]` | Player Self-Overlap Placement Guard | Complete | Player cannot build structures on top of self; strafing while placing supported |
| `[PH-SKYL]` | 2.5D Roof Skylight Frame & Interior Layers | Complete | Roof cutout window shows internal mana & embers |

