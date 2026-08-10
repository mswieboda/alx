# Level Load & Embed System Roadmap

## [EP-LLE]: Level Load & Embed Epic

Replaces the hardcoded `load_level` block in `MainScene.cpp` with a clean `Level` data
struct, hand-authored level constants, and a future YAML-driven asset pipeline that
generates embedded C++ level data at build time.

---

### [PH-LS]: Phase 1 - Level Struct & Data Types (COMPLETED)

Define the core `Level` data structure and its companion types in `src/alx/Levels.h`.

- [x] `[LSTR]`: Level Struct - declare `Level` in `src/alx/Levels.h` inside `namespace alx` with the following fields:
  ```cpp
  struct Level {
      int     id;
      int     map_width;
      int     map_height;
      GridPos player_spawn;                              // tile coords; load_level multiplies by tile_size
      float   initial_twilight;                          // [0.0, ~0.99] same scale as TWILIGHT_MAX
      std::span<const FixturePlacement> fixtures;        // all fixture types in one flat list
      std::span<const DarkTowerSpawn>   dark_tower_spawns;
  };
  ```
- [x] `[FPT]`: FixturePlacement Type - declare `FixturePlacement { GridPos pos; FixtureType type; }` in `Levels.h`; pairs a grid coordinate with its fixture kind so each entry is self-describing (maps cleanly to a future YAML row)
- [x] `[DTST]`: DarkTowerSpawn Type - declare `DarkTowerSpawn { GridPos pos; }` in `Levels.h`; thin wrapper over `GridPos` for semantic clarity and YAML key mapping (`dark_tower_spawns:`)
- [x] `[GLVL]`: get_level Declaration - declare `const Level* get_level(int id)` in `namespace Levels` inside `Levels.h`; returns `nullptr` for invalid IDs

---

### [PH-LC]: Phase 2 - Level Constants & Lookup (COMPLETED)

Author the 3 hand-written level constant instances in `src/alx/Levels.cpp`.

- [x] `[LCST]`: Level 1 Constants - define `k_level1_fixtures[]` and `k_level1_spawns[]` as `static constexpr` arrays, then `static const Level k_level1` using C++20 designated initializers, migrating all hardcoded data from `MainScene::load_level()`
  ```cpp
  static constexpr FixturePlacement k_level1_fixtures[] = {
      { {15, 12}, FixtureType::Seep    },
      { {10,  8}, FixtureType::Refiner },
      { { 6,  6}, FixtureType::Spire   },
      { {16, 11}, FixtureType::Pipe    },
      { {16, 10}, FixtureType::Pipe    },
      // ... remaining pipes ...
  };
  static constexpr DarkTowerSpawn k_level1_spawns[] = {
      { {25, 10} }, { {40, 12} }, { {15, 20} }, { {45,  8} }
  };
  static const Level k_level1 = {
      .id               = 1,
      .map_width        = 60,
      .map_height       = 30,
      .player_spawn     = {9, 9},
      .initial_twilight = 0.9f,
      .fixtures         = k_level1_fixtures,
      .dark_tower_spawns = k_level1_spawns,
  };
  ```
- [x] `[LC23]`: Level 2 & 3 Stubs - define placeholder `k_level2` and `k_level3` with empty fixture/spawn spans so `get_level` can return valid pointers for all 3 IDs (full data filled in later during level design)
- [x] `[GLVI]`: get_level Implementation - implement `namespace alx::Levels { const Level* get_level(int id) }` with a `switch` over the 3 IDs returning pointers to the static level constants; returns `nullptr` for unknown IDs

---

### [PH-MS]: Phase 3 - MainScene Refactor (COMPLETED)

Wire `MainScene` to consume `Level` data, removing all hardcoded values.

- [x] `[LLR]`: load_level Refactor - rewrite `MainScene::load_level(int id)` to call `Levels::get_level(id)`, null-check the result (early return / assert), then apply all fields from the `Level` struct (`map_width/height` → `Tiles` & `Network` constructors, `player_spawn * tile_size` → `Player` position, `initial_twilight` → `m_twilight_level`)
- [x] `[LTNR]`: load_tiles_and_network Signature Refactor - update private method signature from 4 separate `std::vector<std::pair<int,int>>` parameters to a single `std::span<const FixturePlacement>`; internally dispatch each entry's `type` to the correct `m_network.place_fixture()` call
- [x] `[RSP]`: Respawn Hardcode Fix - replace the hardcoded `9.0f * m_tiles.tile_size()` spawn coordinates in `update_player_respawn()` (and the headless variant in `update_headless_defense`) with a stored `m_player_spawn` member (type `GridPos`) set from `Level::player_spawn` during `load_level`

---

### [PH-YP]: Phase 4 - YAML Level Source Format (Design & Spec)

Define the YAML authoring format for level data to be processed by `pack_assets.cr`.
Two candidate formats are under consideration — final choice to be made before
implementation of Phase 5.

**Option A — Text Grid Map:**
```yaml
# levels/level_01.yml
id: 1
map_width: 60
map_height: 30
player_spawn: { x: 9, y: 9 }
initial_twilight: 0.9
# Grid uses characters to place fixtures and spawn zones.
# S=Seep (3x2), R=Refiner (3x3), L=Spire (2x3), P=Pipe (1x1), D=DarkTowerSpawn
# Top-left corner of multi-tile fixtures is their anchor GridPos.
grid: |
  ............................................................
  ............................................................
  ......L.....R..............S........................D.......
  ............................................................
  ...P.P.P.P.P.P.............................D.D.D...........
  ............................................................
  ....................................D......D................
  ............................................................
```
- Pros: Extremely visual and spatial — easy to see layout at a glance
- Cons: Harder to parse multi-tile fixtures correctly (anchor vs. body cells); sparse grids are mostly dots; less obvious for non-grid entities like spawn zones

**Option B — Key-Value Lists:**
```yaml
# levels/level_01.yml
id: 1
map_width: 60
map_height: 30
player_spawn: { x: 9, y: 9 }
initial_twilight: 0.9
fixtures:
  - { x: 15, y: 12, type: seep }
  - { x: 10, y:  8, type: refiner }
  - { x:  6, y:  6, type: spire }
  - { x: 16, y: 11, type: pipe }
  - { x: 16, y: 10, type: pipe }
  - { x: 16, y:  9, type: pipe }
  - { x: 15, y:  9, type: pipe }
  - { x: 14, y:  9, type: pipe }
  - { x: 13, y:  9, type: pipe }
  - { x:  9, y:  9, type: pipe }
  - { x:  8, y:  9, type: pipe }
  - { x:  7, y:  9, type: pipe }
dark_tower_spawns:
  - { x: 25, y: 10 }
  - { x: 40, y: 12 }
  - { x: 15, y: 20 }
  - { x: 45, y:  8 }
```
- Pros: Direct 1-to-1 mapping to `FixturePlacement` and `DarkTowerSpawn` structs; simple to parse; easy to add new fields per entry in the future
- Cons: Less visual — requires mental mapping to understand layout; verbose for many pipes

- [ ] `[YFD]`: YAML Format Decision - pick Option A, Option B, or a hybrid (e.g. text grid for fixtures, key-value list for spawn zones) and document the canonical format
- [ ] `[YFS]`: YAML File Structure - finalize directory (`_assets/levels/` or `assets/levels/`), naming convention (`level_01.yml`), and whether one file per level or one combined file

---

### [PH-YE]: Phase 5 - YAML Parsing & C++ Header Embedding

Implement the `pack_assets.cr` pipeline step that reads YAML level files and generates
`src/assets/Levels.h` with embedded `constexpr` level data replacing the hand-authored
`Levels.cpp` constants.

- [ ] `[PAL]`: pack_assets.cr Level Parser - add a level YAML parsing step to `pack_assets.cr` that reads all `level_*.yml` files, validates required fields, and builds an in-memory level representation
- [ ] `[CGN]`: C++ Code Generation - emit `src/assets/Levels.h` containing:
  - `constexpr FixturePlacement k_level{N}_fixtures[]` arrays
  - `constexpr DarkTowerSpawn k_level{N}_spawns[]` arrays
  - `constexpr Level k_level{N}` instances using designated initializers
  - `constexpr std::array<const Level*, N> k_all_levels` for iteration
- [ ] `[MIG]`: Levels.cpp Migration - once `src/assets/Levels.h` is generated and verified, remove `src/alx/Levels.cpp` hand-authored constants; `get_level()` implementation moves to include the generated header and index into `k_all_levels`
- [ ] `[VLD]`: Validation & Build Integration - wire the YAML parsing step into `Taskfile.yml` asset pipeline so `task build` auto-regenerates `src/assets/Levels.h` when any `level_*.yml` is modified; add bounds checks in the parser (fixture coords within `map_width/height`, no duplicate grid positions, etc.)
