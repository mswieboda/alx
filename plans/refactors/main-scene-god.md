# `[EP-MSDC]`: MainScene God Object Decomposition

## Overview
[`src/alx/MainScene.cpp`](file:///Users/matt/code/cpp/alx/src/alx/MainScene.cpp) (869 lines) and [`src/alx/MainScene.h`](file:///Users/matt/code/cpp/alx/src/alx/MainScene.h) (213 lines) currently function as a monolithic "God Object", bundling scene orchestration with HUD rendering, fog rasterization, test telemetry instrumentation, terrain drawing, particle effects, and dead query functions.

This plan details a phased, zero-overhead decomposition into modular components, shrinking `MainScene.cpp` by ~58% down to ~350 lines.

---

## Current Responsibility Inventory

```
                                  ┌──────────────────────────────┐
                                  │       MainScene (God)        │
                                  └──────────────┬───────────────┘
          ┌──────────────┬──────────────┬────────┴─────┬──────────────┬──────────────┐
          ▼              ▼              ▼              ▼              ▼              ▼
   [HUD & Overlays] [Twilight Fog] [Telemetry]   [Terrain Draw] [Sword VFX]   [Dead Code]
      ~165 lines      ~80 lines     ~140 lines     ~53 lines      ~40 lines    ~20 lines
```

| Domain / Responsibility | Current Location | Line Count | Key Methods & Data |
|---|---|---|---|
| **Scene Orchestration & Loop** | [`MainScene.cpp:27-139, 223-364`](file:///Users/matt/code/cpp/alx/src/alx/MainScene.cpp#L27-L139) | ~250 lines | `init()`, `load_level()`, `update()`, `update_victory_condition()`, `update_tick_simulation()` |
| **HUD, Overlays & Game Over** | [`MainScene.cpp:627-792`](file:///Users/matt/code/cpp/alx/src/alx/MainScene.cpp#L627-L792) | ~165 lines | `draw_hud()`, `draw_game_over_fade()`, `draw_game_over_hud()`, `draw_victory_and_pause_overlays()`, `m_game_over_menu` |
| **Telemetry & Headless Harness** | [`MainScene.cpp:140-221, 442-498`](file:///Users/matt/code/cpp/alx/src/alx/MainScene.cpp#L140-L221) | ~140 lines | `record_twilight_event()`, `calculate_rolling_twilight_rate()`, `dump_telemetry_snapshot()`, `update_headless_defense()`, `m_rolling_samples` |
| **Twilight Fog & Vignette FX** | [`MainScene.cpp:547-625`](file:///Users/matt/code/cpp/alx/src/alx/MainScene.cpp#L547-L625) | ~80 lines | `draw_twilight()` (software radial blend), `draw_vignette_surge()`, `m_twilight_pixel_buffer`, `m_vignette_surge_timer` |
| **Terrain & Tile Rendering** | [`MainScene.cpp:814-866`](file:///Users/matt/code/cpp/alx/src/alx/MainScene.cpp#L814-L866) | ~53 lines | `draw_tiles_and_network()`, `draw_terrain_tile()`, tile frame UV offsets |
| **Sword Slash VFX Trail** | [`MainScene.cpp:367-405`](file:///Users/matt/code/cpp/alx/src/alx/MainScene.cpp#L367-L405) | ~40 lines | `update_sword_slash_trail()`, `m_slash_prev_tip_x/y`, `m_slash_was_attacking` |
| **Unused / Dead Queries** | [`MainScene.cpp:794-812`](file:///Users/matt/code/cpp/alx/src/alx/MainScene.cpp#L794-L812) | ~20 lines | `is_connectable_fixture()`, `connects_dark_mana()`, `is_node_fixture()` (0 external call sites) |

---

## Actionable Phases & Task Breakdown

### `### [PH-HUDO]`: Phase 1 - In-Game HUD & Overlay Separation (`HUD.h` / `HUD.cpp`)
- [ ] `[HUD-DATA]`: Define `struct HUDState` and extraction helper functions in `src/alx/HUD.h` to decouple UI presentation from scene state.
- [ ] `[HUD-IMPL]`: Migrate `draw_hud()`, `draw_game_over_fade()`, `draw_game_over_hud()`, and `draw_victory_and_pause_overlays()` into `src/alx/HUD.cpp`.
- [ ] `[HUD-INTG]`: Replace inline HUD calls in [`MainScene::draw_screen()`](file:///Users/matt/code/cpp/alx/src/alx/MainScene.cpp#L556) with a single composed `HUD::draw()` call.

### `### [PH-TWFX]`: Phase 2 - Twilight Mist & Screen FX Isolation (`TwilightOverlay.h` / `TwilightOverlay.cpp`)
- [ ] `[TW-DECL]`: Create `class TwilightOverlay` in `src/alx/TwilightOverlay.h` owning `m_pixel_buffer` and `m_vignette_timer`.
- [ ] `[TW-IMPL]`: Move `draw_twilight()` radial software rasterization and `draw_vignette_surge()` to `src/alx/TwilightOverlay.cpp`.
- [ ] `[TW-INTG]`: Integrate `TwilightOverlay` instance into `MainScene`, routing `trigger_vignette_surge` and overlay drawing through it.

### `### [PH-TELEM]`: Phase 3 - Telemetry & Headless Tracker Isolation (`MainSceneTelemetry.h` / `MainSceneTelemetry.cpp`)
- [ ] `[TELM-DECL]`: Create `MainSceneTelemetry` component with full telemetry fields under `#if ALX_ENABLE_TELEMETRY || ALX_ENABLE_HEADLESS` and zero-cost stubs otherwise.
- [ ] `[TELM-IMPL]`: Move `record_twilight_event()`, `calculate_rolling_twilight_rate()`, `dump_telemetry_snapshot()`, `update_headless_defense()`, and `print_headless_summary_report()` into `src/alx/MainSceneTelemetry.cpp`.
- [ ] `[TELM-INTG]`: Replace bloated telemetry members in [`MainScene.h`](file:///Users/matt/code/cpp/alx/src/alx/MainScene.h) with a clean `MainSceneTelemetry m_telemetry` instance.

### `### [PH-TRDR]`: Phase 4 - Delegate Terrain Tile Drawing to `Tiles`
- [ ] `[TILE-DRAW]`: Add `void draw(const Camera& camera)` to [`Tiles.h`](file:///Users/matt/code/cpp/alx/src/alx/Tiles.h) and [`Tiles.cpp`](file:///Users/matt/code/cpp/alx/src/alx/Tiles.cpp).
- [ ] `[TILE-IMPL]`: Relocate `draw_terrain_tile()` and tile camera bounding box loops from `MainScene.cpp` to `Tiles.cpp`.
- [ ] `[TILE-INTG]`: Simplify `MainScene::draw_tiles_and_network()` into clean sequential calls: `m_tiles.draw(m_camera)` and `m_network.draw(...)`.

### `### [PH-PLFX]`: Phase 5 - Move Sword Slash FX Trail to `Player`
- [ ] `[SWFX-DECL]`: Add sword trail tip position tracking state (`m_slash_prev_tip_x/y`, `m_slash_was_attacking`) to [`Player.h`](file:///Users/matt/code/cpp/alx/src/alx/Player.h).
- [ ] `[SWFX-IMPL]`: Move `update_sword_slash_trail()` logic inside `Player::update()` in [`Player.cpp`](file:///Users/matt/code/cpp/alx/src/alx/Player.cpp) where `ParticleSystem*` is already passed.
- [ ] `[SWFX-INTG]`: Remove `update_sword_slash_trail()` and associated tip tracking fields from `MainScene`.

### `### [PH-DEAD]`: Phase 6 - Prune Dead Methods
- [ ] `[DED-PRUN]`: Remove unused `is_connectable_fixture()`, `connects_dark_mana()`, and `is_node_fixture()` methods from `MainScene.h` and `MainScene.cpp`.

---

## Projected Line Count & Complexity Impact

| File | Current Lines | Projected Lines | Delta |
|---|---|---|---|
| [`src/alx/MainScene.cpp`](file:///Users/matt/code/cpp/alx/src/alx/MainScene.cpp) | **869** | **~340 - 370** | **-58% (-500 lines)** |
| [`src/alx/MainScene.h`](file:///Users/matt/code/cpp/alx/src/alx/MainScene.h) | **213** | **~75 - 90** | **-60% (-130 lines)** |
| `src/alx/HUD.cpp` (new) | 0 | ~150 | Extracted HUD & Overlays |
| `src/alx/TwilightOverlay.cpp` (new) | 0 | ~75 | Extracted Software Fog |
| `src/alx/MainSceneTelemetry.cpp` (new) | 0 | ~130 | Extracted Telemetry & Headless |

---

## Engineering Standards & Verification Constraints

1. **Zero Dynamic Allocation in Main Loop**: All extracted components must maintain fixed-size storage, value semantics, and pre-allocated buffers.
2. **Binary Size Target**: Keep compiled binary within the 1.44MB floppy limit (< 1,474,560 bytes).
3. **Build Target**: Compile after each phase with `task build` to ensure type safety and zero regressions.
