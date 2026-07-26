# Architecture Plan: Decoupling Visual Terrain (`Tiles`) from Buildable Fixture Layer (`Network`)

## Overview

This refactoring splits the heavy monolithic `alx::Grid` into two modular, decoupled systems:
1. **`alx::Tiles` (`Tiles.h`)**: Purely visual, static floor/wall terrain rendering and environmental boundaries/collisions.
2. **`alx::Network` (`Network.h`)**: Dynamic interactive player-built network entities (`Fixture` objects like Pipes, Refiners, Spires, and Seeps) with pull-based mana propagation simulation, Model B BFS auto-direction routing, and collision logic.

---

## Core Data Structures

### 1. `GridPos` (`src/alx/GridPos.h`)
- Lightweight 4-byte 2D integer position `(x, y)` (`sizeof(GridPos) == 4`).
- Zero heavy standard library includes (only `<cstdint>` and `<compare>`).
- C++20 defaulted comparisons (`operator==`, `<=>`).
- Built-in spatial helpers: `to_index(width)`, `from_index(idx, width)`, 4-way neighbors (`north()`, `south()`, `east()`, `west()`), and pure integer `manhattan_dist()`.

### 2. `Tile` & `Tiles` (`src/alx/Tile.h`, `src/alx/Tiles.h`)
- **`TileType`**: `Empty`, `Floor`, `Wall`.
- **`Tile` struct**: Clean representation of visual floor tiles.
- **`Tiles` class**:
  - `std::vector<Tile> m_tiles` ($W \times H$ dense array).
  - Handles bounds checking, visual floor/wall texture rendering, and static terrain collision (`is_wall`).

### 3. `Fixture` (`src/alx/Fixture.h`)
- **`FixtureType`**: `None`, `Pipe`, `Refiner`, `Spire`, `Seep`.
- **`ManaState`**: `None`, `Dark`, `Light`.
- **`DirectionMask`**: `North` ($1 \ll 0$), `East` ($1 \ll 1$), `South` ($1 \ll 2$), `West` ($1 \ll 3$).
- **`FixtureFlag`**: `Powered`, `Active`, `DirtySprite`.
- **8-Byte Binary Layout**:
  - `FixtureType type` (1 byte)
  - `ManaState mana_state` (1 byte)
  - `uint8_t flow_in_mask` (1 byte)
  - `uint8_t flow_out_mask` (1 byte)
  - `int8_t move_dx`, `int8_t move_dy` (2 bytes)
  - `uint8_t process_timer` / `stagnant_timer` (1 byte)
  - `uint8_t flags` / `last_out_dir` (1 byte)

### 4. `Network` (`src/alx/Network.h`)
- **Storage**:
  - `std::vector<Fixture> m_fixtures`: Flat spatial $W \times H$ array for $O(1)$ coordinate lookup.
  - `std::vector<int32_t> m_active_indices`: Compact list of active fixture grid indices for $O(1)$ fast active iteration without full grid scans.
- **Flow Routing & Simulation (Model B)**:
  - **Sink BFS Auto-Gradient**: Runs a lightweight Breadth-First Search (BFS) from active Sinks (Refiners / Spires) back through connected pipes. Every connected pipe tile automatically receives directional flow vectors (`move_dx`, `move_dy`) pointing toward the nearest sink.
  - **Dead-End Drainage**: Any leftover mana in dead-end branches connected to a sink flows directly into the sink until empty.
  - **Pull-Based Multi-Pass Ticks**: Downstream consumers pull mana first, opening slots for upstream pipes in the same tick.
  - **Rotating Priority (Round-Robin)**: Cycles `last_out_dir` (N $\rightarrow$ E $\rightarrow$ S $\rightarrow$ W) at T-junctions/splitters for 50/50 balanced distribution.
- **Stagnant Mana Decay & Twilight Release**:
  - If mana sits stagnant in a severed pipe or blocked line without moving for $N$ ticks (e.g., 30 ticks / ~3 seconds), the mana decays and vanishes from the pipe.
  - Spawns a temporary twilight haze cloud effect at the tile position.
  - Increases room twilight level (`m_twilight_level`).
- **Collision & Auto-Tiling**:
  - `is_solid(GridPos pos)`: Returns `true` for Refiner, Spire, Seep; `false` for Pipes (walkable/jumpable) and empty slots.
  - Auto-tiling: Dynamically computes 4-bit connection masks for 16-way pipe sprite auto-tiling.

---

## Phased Execution Plan

### Phase 1: Header Definitions & Core Data Types
- [x] Create `src/alx/GridPos.h` with `GridPos` struct, index converters, and equality/comparison operators.
- [x] Create `src/alx/Tile.h` defining static `TileType` and `Tile` struct.
- [x] Create `src/alx/Fixture.h` defining `FixtureType`, `ManaState`, `DirectionMask`, `FixtureFlag`, and packed 8-byte `Fixture` struct.

### Phase 2: Static Terrain Layer (`Tiles`)
- [ ] Create `src/alx/Tiles.h` replacing static map handling of `Grid.h`.
- [ ] Implement spatial bounds checking `in_bounds(GridPos pos)` and terrain collision `is_wall(GridPos pos)`.
- [ ] Implement `Tiles::draw(Renderer& renderer, const Camera& camera)`.

### Phase 3: Network Fixture Layer (`Network`)
- [ ] Create `src/alx/Network.h` with spatial fixture array and active index tracking (`m_active_indices`).
- [ ] Implement `place_fixture(GridPos pos, FixtureType type)` and `remove_fixture(GridPos pos)` with swap-and-pop active index management.
- [ ] Implement auto-tiling neighbor mask updates (`update_neighbor_masks(GridPos pos)`).
- [ ] Implement `Network::is_solid(GridPos pos)` fixture collision helper.
- [ ] Implement Model B Sink BFS gradient calculator `recalculate_flow_gradients()` to auto-direct pipe flow vectors.
- [ ] Implement `Network::update(float dt, const Tiles& tiles)` featuring pull-based multi-pass mana flow propagation, round-robin output distribution, and stagnant mana decay into twilight clouds.
- [ ] Implement `Network::draw(Renderer& renderer, const Camera& camera)`.

### Phase 4: `MainScene` & System Integration
- [ ] Update `MainScene.h` to replace `Grid m_grid` with `Tiles m_tiles` and `Network m_network`.
- [ ] Update `MainScene::load_level()` to populate `m_tiles` with room terrain and `m_network` with initial Seep, Refiner, Spire, and Pipe fixtures.
- [ ] Refactor player input action `Action::BuildTile` to `Action::BuildFixture`.
- [ ] Update player movement collision logic to evaluate `m_tiles.is_wall(pos) || m_network.is_solid(pos)`.
- [ ] Refactor render loop: Layer 0 `m_tiles.draw()`, Layer 1 `m_network.draw()`, Layer 2 dynamic entities (`EnemyManager`, `Player`).
- [ ] Safely remove legacy `Grid.h`.

### Phase 5: Verification & Compilation
- [ ] Execute `make build` to verify clean C++20 compilation, strict type safety, zero warnings/errors, and tight binary target alignment.
