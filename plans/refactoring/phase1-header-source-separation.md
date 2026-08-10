# Phase 1 Plan: Header/Source Separation & Physical Boundary Cleanup (`phase1-header-source-separation.md`)

## `[EP-HSRC]`: Executive Overview & Objectives

The primary objective of **Phase 1** is to resolve pervasive Header (`.h`) vs. Source (`.cpp`) separation violations across the codebase. Currently, multiple header files in `src/alx/*` and `src/core/*` contain hundreds of lines of executable method bodies, nested includes inside namespace blocks, inline rendering logic, and non-virtual method hiding.

Resolving these violations will:
1. Drastically reduce translation unit compilation times and symbol bloat.
2. Prevent cyclic header dependencies and header namespace pollution.
3. Enforce strict One Definition Rule (ODR) compliance as mandated by `AGENTS.md` and `refactor-alx` standards.

---

## Target Files & Code Locations

* [`src/alx/Enemy.h`](file:///Users/matt/code/cpp/alx/src/alx/Enemy.h#L105-L337) / [`src/alx/Enemy.cpp`](file:///Users/matt/code/cpp/alx/src/alx/Enemy.cpp#L1-L8)
* [`src/alx/MainScene.h`](file:///Users/matt/code/cpp/alx/src/alx/MainScene.h#L1-L453) (Proposed: `src/alx/MainScene.cpp`)
* [`src/alx/Fixture.h`](file:///Users/matt/code/cpp/alx/src/alx/Fixture.h#L83-L172) (Proposed: `src/alx/Fixture.cpp`)
* [`src/alx/AlloyItem.h`](file:///Users/matt/code/cpp/alx/src/alx/AlloyItem.h#L35-L105) (Proposed: `src/alx/AlloyItem.cpp`)
* [`src/core/Camera.h`](file:///Users/matt/code/cpp/alx/src/core/Camera.h#L66-L144) / [`src/alx/Camera.h`](file:///Users/matt/code/cpp/alx/src/alx/Camera.h#L10-L187) (Proposed: `src/core/Camera.cpp`)
* [`src/core/Collision.h`](file:///Users/matt/code/cpp/alx/src/core/Collision.h#L19-L107) (Proposed: `src/core/Collision.cpp`)
* [`src/core/Entity.h`](file:///Users/matt/code/cpp/alx/src/core/Entity.h#L57-L82) (Proposed: `src/core/Entity.cpp`)
* [`src/core/FrameTime.h`](file:///Users/matt/code/cpp/alx/src/core/FrameTime.h#L18-L27) (Proposed: `src/core/FrameTime.cpp`)
* [`src/core/SceneManager.h`](file:///Users/matt/code/cpp/alx/src/core/SceneManager.h#L10-L47) (Proposed: `src/core/SceneManager.cpp`)

---

## `[PH-ENMY]`: `Enemy.h` Method Relocation & Const-Cast Cleanup (COMPLETED)

### Identified Anti-Patterns
* `Enemy.h` contains over 230 lines of non-trivial inline method implementations (`take_damage`, `draw`, circle calculations, trig math, hit flash color blending), while `Enemy.cpp` is a trivial 8-line stub (~131 bytes).
* `Enemy::draw()` performs a `const_cast<Enemy*>(this)->facing_dx = move_dx;` on lines 297–300 to mutate internal movement state inside a `const` drawing pass.

### Action Plan & Sub-Tasks
* `[EHSO]`: Move all method implementations (`Enemy::Enemy()`, `center_x`, `center_y`, `ground_circle`, `hurt_circle`, `set_steering_vector_8way`, `take_damage`, `draw`) out of `Enemy.h` into [`Enemy.cpp`](file:///Users/matt/code/cpp/alx/src/alx/Enemy.cpp). (COMPLETED)
* `[ECST]`: Eliminate `const_cast` in `draw()` by updating facing vector state during entity movement logic in [`EnemyMovement.cpp`](file:///Users/matt/code/cpp/alx/src/alx/EnemyMovement.cpp) or `Enemy::update()`. (COMPLETED)
* `[EHDR]`: Keep `Enemy.h` clean with member declarations, constants, forward declarations, and inline 1-line getters only. (COMPLETED)

---

## `[PH-MSCN]`: `MainScene.h` Source Extraction & Header Cleanup (COMPLETED)

### Identified Anti-Patterns
* `MainScene.h` is 453 lines (~16.5 KB) and contains complete inline function definitions for `init()`, `load_level()`, `load_tiles_and_network()`, `update()`, `update_tick_simulation()`, `sync_camera()`, `draw_world()`, `draw_screen()`, `draw_twilight()`, `draw_hud()`, `draw_tiles_and_network()`, and `draw_terrain_tile()`.
* Line 23 includes `"alx/Camera.h"` inside the `namespace alx { ... }` block, causing header namespace pollution.

### Action Plan & Sub-Tasks
* `[MSCP]`: Create a new source file [`src/alx/MainScene.cpp`](file:///Users/matt/code/cpp/alx/src/alx/MainScene.cpp). (COMPLETED)
* `[MSM1]`: Move method implementations (`init`, `load_level`, `update`, `draw_world`, `draw_screen`, `draw_twilight`, `draw_hud`) out of `MainScene.h` into `MainScene.cpp`. (COMPLETED)
* `[MSIN]`: Relocate `#include "alx/Camera.h"` to the top of `MainScene.h` outside of the `namespace alx` block. (COMPLETED)
* `[MSHD]`: Leave only struct/class definitions, member declarations, and static constants in [`MainScene.h`](file:///Users/matt/code/cpp/alx/src/alx/MainScene.h). (COMPLETED)

---

## `[PH-FXAL]`: `Fixture.h` & `AlloyItem.h` Source File Creation (COMPLETED)

### Identified Anti-Patterns
* `Fixture.h` contains non-trivial helper logic inline in the header: `get_fixture_ports` (lines 83–101), `is_fixture_port` (lines 103–113), `max_fixture_footprint_dimension` (lines 115–129), and `fixture_ground_aabb` (lines 155–172).
* `AlloyItem.h` is a header-only file containing full implementations of `update()` and a 50+ line `draw()` method with loops and rendering calls inside the header.

### Action Plan & Sub-Tasks
* `[FXSP]`: Create `src/alx/Fixture.cpp` and move non-trivial port calculation functions and AABB helpers out of `Fixture.h`. (COMPLETED)
* `[ALSP]`: Create `src/alx/AlloyItem.cpp` and relocate `AlloyItem::update()` and `AlloyItem::draw()` implementations from `AlloyItem.h`. (COMPLETED)

---

## `[PH-CORE]`: Core Engine Header Extraction

### Identified Anti-Patterns
* `src/core/Collision.h` implements an 88-line 2D physics collision library (`aabb`, `circle_vs_circle`, `circle_contact_point`, `circle_vs_aabb`, `resolve_soft_circle_overlap`) inline inside the header.
* `src/core/Camera.h` implements a 56-line `Camera::update()` method with lerp/clamp math inline in the header.
* `src/core/Entity.h` contains non-template `AnimatedSpriteRender` helper methods inline.
* `src/core/FrameTime.h` contains high-resolution clock update logic inline.
* `src/core/SceneManager.h` contains inline implementations for `process_pending_changes()`, `change_scene()`, `update()`, and `draw()`.

### Action Plan & Sub-Tasks
* `[CLSP]`: Create `src/core/Collision.cpp` and move 2D physics collision functions into it.
* `[CMSP]`: Create `src/core/Camera.cpp` and move `Camera::update()` logic out of `Camera.h`.
* `[ENSP]`: Create `src/core/Entity.cpp` and move `AnimatedSpriteRender` methods into it.
* `[FTSP]`: Create `src/core/FrameTime.cpp` and move `FrameTime::update()` logic into it.
* `[SMSP]`: Create `src/core/SceneManager.cpp` and move `SceneManager` methods into it.

---

## `[PH-CAMU]`: Camera Unification & Class Shadowing Removal

### Identified Anti-Patterns
* Dual-camera ambiguity: `alx::Camera` derives from `core::Camera` but hides/overrides non-virtual `update()` methods, duplicating boundary clamping math (`has_limits`, `std::clamp`).
* `core::Camera` defines `using Camera = core::Camera;` in global scope.
* `Scene` base class holds `core::Camera m_camera;` which `MainScene` shadows with `alx::Camera m_camera;` and overrides `camera()` virtual getters.

### Action Plan & Sub-Tasks
* `[CAM1]`: Consolidate target position tracking, deadzone calculation, map limits clamping, and smooth lerping into a single canonical `core::Camera` class.
* `[CAM2]`: Remove non-virtual method hiding and member variable shadowing in `MainScene`.

---

## Verification & Build Criteria

After completing Phase 1 edits:
1. Run syntax and type-safety verification:
   ```bash
   task build
   ```
2. Verify that header files (`src/alx/*.h`, `src/core/*.h`) contain only declarations, constants, `constexpr`, and templates.
3. Enforce zero trailing whitespace and a single newline at the end of all modified or newly created files.
