# AGENTS.md

## Project Overview
*Aetherlux* (`alx`) is a GBA-aesthetic 2D top-down action-adventure and micro-automation survival crafting game built in C++ with software rendering (`minifb`) as well as audio (`miniaudio`), mod support (`pocketmod`), and gamepad (`minigamepad` support). Players explore room grids, gather Alloy, and build mana pipe infrastructure (Pipes, Refiners, Spires) to refine dark twilight mana into stable light energy to reduce twilight in the room/level.

---

## Build & Compilation Instructions

- **Use `task build` to compile**: Always use `task build` when compiling to verify syntax, type safety, and build success without launching the GUI window.
- **Avoid plain `task` / `task run`**: Plain `task` executes the default target (`build run`), which launches the interactive game window. Only run `task` or `task run` when explicitly asked to, to inspect runtime startup or scene initialization logs.
- **Deprecated build pipeline**: GNU Make and the `Makefile` have been deprecated for this project. Running it will output in a warning/error and will not do anything.

---

## Codebase Standards & Engineering Guidelines

AI agents operating in this codebase MUST follow these strict engineering standards at all times.

### 1. Strict Prohibition of Magic Numbers & Hardcoded Values
- **Zero Magic Numbers**: Never hardcode raw numeric literals (e.g. `16`, `32`, `8.0f`, pixel offsets, layout margins) in drawing, rendering, collision, coordinate transformation, or logic math.
- **Single Source of Truth**:
  - Always query existing struct/class properties, getters, method returns, or shared grid/room metadata (e.g., `tile_size`, `room.getTileWidth()`, `grid.width`).
  - If a shared constant is missing, define it as a named `constexpr` or static constant in an appropriate header or namespace.
- **Dynamic Sizing & Layout Math**:
  - Render positioning, entity bounds, UI layouts, and grid cell mappings must be calculated dynamically based on shared dimensions and container bounds rather than hardcoded pixel adjustments (`+ 16`, `* 8`).

### 2. High-Engineering Standards & Clean Architecture
- **No Quick Hacks or Cowboy Coding**:
  - Never patch symptoms, swallow exceptions, inject temporary band-aid workarounds, or return dummy fallback values to bypass bugs.
  - Every change must fix the root cause and maintain clean architectural boundary lines.
- **Prioritize Engineering Quality Over Speed**:
  - AI agents must design robust, maintainable, modular, and scalable solutions following good software engineering principles—even if doing so requires significantly more time, effort, and thorough refactoring.

### 3. Header (`.h`) vs. Source (`.cpp`) Separation
- **Declarations in Headers, Implementations in Source**:
  - Header files (`.h` / `.hpp`) must contain ONLY declarations (struct/class definitions, interface signatures, constants, enums).
  - All executable function and method bodies MUST be placed in corresponding `.cpp` source files to ensure fast compilation, clean API contracts, and proper One Definition Rule (ODR) compliance.
- **Minimal Header Include Bloat**:
  - Use forward declarations (e.g., `class Player;`, `struct Room;`) in header files whenever possible instead of including external headers. Keep heavy `#include` directives inside `.cpp` files.
- **Allowed Header Implementation Exceptions**:
  - C++ Templates (`template <typename T>`) where instantiation requires full definition visibility.
  - `constexpr` functions evaluated at compile-time.
  - Trivial 1-line inline accessors/mutators (e.g., `inline int getWidth() const { return m_width; }`).

### 4. Modern C++ & 2D Game Architecture Best Practices
- **Expressive & Safe Types**:
  - Target C++ Standard: **C++20**. Leverage modern standard features (`std::span`, `std::string_view`, `constexpr`, `[[nodiscard]]`, designated initializers).
  - Use strong typing, `constexpr`, `enum class`, and explicitly-sized integer types (`uint32_t`, `int32_t`).
  - Pass non-primitive parameters by `const` reference (`const T&`) or string/array views (`std::string_view`, `std::span`) when read-only.
  - Enforce `const` correctness on all inspector methods, queries, and getters.
  - Always use in-class member initializers (e.g., `int m_hp{100};`) to eliminate uninitialized memory undefined behavior (UB).
- **RAII & Memory Safety**:
  - Avoid raw ownership pointers (`new`/`delete`). Rely on RAII, value semantics, and smart pointers.
  - Prefer contiguous storage (`std::vector<T>`) for cache locality over pointer indirection arrays. Use `std::unique_ptr` for exclusive owner boundaries (e.g., System/Scene managers) and reserve `std::shared_ptr` strictly for shared resources (e.g., asset handles).
- **Frame Loop Performance & Hot Path Hygiene**:
  - Zero dynamic heap allocations inside `update()` or `render()` loops. Pre-allocate collections, reuse draw buffers, and avoid temporary container instantiations per frame to prevent stutters and cache line misses.
- **Software Rendering & Framebuffer Hygiene**:
  - Keep state update logic (`update()`) strictly decoupled from rendering passes (`render()`).
  - Always enforce clip rectangle checks (`x >= 0 && x < width && y >= 0 && y < height`) before raw pixel buffer indexing (`y * screen_width + x`) to prevent out-of-bounds memory writes.
  - Update logic should accept explicit delta time (`float dt` or fixed timestep) for frame-rate independent simulation.

---

## Planning & Documentation

- **Status of Plan Files (`.plans/PLAN*.md`)**:
  - Files inside `.plans/` are informal, initial brainstorming scratchpads and phase proposals.
  - **Highly Unstable & Out-of-Date**: Plan files quickly become obsolete as implementation evolves and MUST NOT be treated as authoritative sources of truth for codebase state, architectural contracts, or current tasks.
- **Strict Rules for AI Agents**:
  - **Do NOT auto-consult or rely on `.plans/`**: Do not read, query, or check files in `.plans/` during routine coding, refactoring, or debugging tasks. Only inspect plan files if the user explicitly instructs you to reference or evaluate a specific plan.
  - **Updating Plan Files**: Do NOT attempt to update, rewrite, or sync `.plans/` files during routine work. However, if you are actively working off a specific plan file requested by the user, updating task status (e.g., marking items `(COMPLETED)`) within that plan document is allowed and encouraged.
  - **Codebase as Single Source of Truth**: Always inspect actual C++ source files (`src/`), headers, and current project code to determine implementation state.

## Brainstorming & Task Formatting

When collaborating on feature planning, roadmaps, task breakdowns, subtasks, data structures, or architectural designs, strictly format every item using the **Codename Acronym Format**:

`[ACRONYM]`: Title - brief description/details (include concise C++ struct suggestions if applicable, e.g., `DarkTower struct: float x, y; int hp = 8;`).

### Naming & Prefix Rules
- **Acronym**: 1 to 6 uppercase letters derived from title words (e.g., `SCTE` for "Short Concise Title of Epic").
- **Epics**: Prefix `EP-` &rarr; `[EP-SCTE]`
- **Phases**: Prefix `PH-` &rarr; `[PH-SCTE]`
- **Sub-Tasks / Tasks / Data Structures / Bulleted Items**: No prefix &rarr; `[SCTE]`
