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

### [PH-ETD]: Phase 4 - Explicit Tile & Wall Data Integration (COMPLETED)

Decouple tile/wall layout from hardcoded map borders, moving wall placement definition into `Level` data.

- [x] `[WTST]`: Wall & Tile Struct - declare `WallPlacement { GridPos pos; }` in `src/alx/Levels.h` and add `std::span<const WallPlacement> walls` to the `Level` struct
- [x] `[WLCD]`: Level Constants Wall Migration - update static level definitions in `src/alx/Levels.cpp` to supply `walls` arrays (migrating old border wall coords or specifying new interior/custom walls)
- [x] `[TLRF]`: Tile Initialization Refactor - refactor `MainScene::load_tiles_and_network` to initialize grid tiles to `TileType::Floor` by default, eliminating the hardcoded outer border wall generation (`x==0`, `x==1`, etc.), and applying explicit walls from `level.walls`

---

### [PH-YP]: Phase 5 - YAML Level Source Format (Design & Spec) (COMPLETED)

Define the full visual ASCII grid YAML format for level data to be processed by `pack_assets.cr`.

Canonical Level Format (`assets/levels/level_01.yml`):
```yaml
id: 1
map_width: 60
map_height: 30
player_spawn: { x: 9, y: 9 }
initial_twilight: 0.9

# 60x30 ASCII string grid.
# Character Legend:
#   . = Floor (TileType::Floor)
#   # = Wall  (TileType::Wall -> WallPlacement)
#   P = Pipe  (1x1 FixtureType::Pipe; drawn contiguously like 'PPPPPP')
#   R = Refiner   (3x3 footprint block of 'R's -> FixtureType::Refiner)
#   S = Spire     (2x3 footprint block of 'S's -> FixtureType::Spire)
#   s = Seep      (3x2 footprint block of 's's -> FixtureType::Seep)
#   D = DarkTower (3x4 footprint block of 'D's -> DarkTowerSpawn)
grid: |
  ############################################################
  #..........................................................#
  #......SS....RRR..............sss...................DDD....#
  #......SS....RRR..............sss...................DDD....#
  #......SS....RRR....................................DDD....#
  #...PPPPPPPPPPPP....................................DDD....#
  #..........................................................#
  ############################################################
```

- [x] `[YFD]`: YAML Format Decision - Adopt Option A Full ASCII Grid Painting with strict multi-tile footprint validation (3x3 `R`, 2x3 `S`, 3x2 `s`, 3x4 `D`).
- [x] `[YFS]`: YAML File Structure - Finalize directory (`assets/levels/`) and zero-padded file naming (`level_01.yml`, `level_02.yml`, `level_03.yml`).
- [x] `[YCS]`: Character Legend & Footprint Spec - Document character mapping (`#`, `.`, `P`, `R`, `S`, `s`, `D`) and validation bounds in `plans/phases/level-load-embed.md`.
- [x] `[YLS1]`: Level 1 YAML Authoring - Create `assets/levels/level_01.yml` converting Level 1 constants into full visual ASCII grid format.
- [x] `[YLS2]`: Level 2 & 3 YAML Stubs - Create baseline `assets/levels/level_02.yml` and `assets/levels/level_03.yml` stub YAML files.
- [x] `[RDME]`: README Asset Documentation - Document `assets/levels/*.yml` -> `src/assets/Levels.h` in `README.md`.

---

### [PH-YE]: Phase 6 - YAML Parsing & C++ Header Embedding (COMPLETED)

Implement the `pack_assets.cr` pipeline step that reads YAML level files and generates
`src/assets/Levels.h` with embedded `constexpr` level data replacing the hand-authored
`Levels.cpp` constants.

- [x] `[PAL]`: pack_assets.cr Level Parser - add a level YAML parsing step to `pack_assets.cr` supporting `--only=levels` that reads all `assets/levels/level_*.yml` files, validates required fields and footprints, and builds an in-memory level representation
- [x] `[CGN]`: C++ Code Generation - emit `src/assets/Levels.h` under `namespace Assets::Levels` containing:
  - `constexpr FixturePlacement k_level{N}_fixtures[]` arrays
  - `constexpr DarkTowerSpawn k_level{N}_spawns[]` arrays
  - `constexpr WallPlacement k_level{N}_walls[]` arrays
  - `constexpr Level k_level{N}` instances using designated initializers
  - `constexpr std::array<const Level*, N> k_all_levels` for iteration
- [x] `[TKFL]`: Taskfile Asset Integration - add `levels` task to `Taskfile.yml` watching `assets/levels/**/*`, emitting `src/assets/Levels.h`, and registered under `assets-parallel` dependencies
- [x] `[MIG]`: Levels.cpp Migration - once `src/assets/Levels.h` is generated and verified, remove `src/alx/Levels.cpp` hand-authored constants; `get_level()` implementation moves to include the generated header and index into `k_all_levels`
- [x] `[VLD]`: Validation & Build Integration - verify `task build` auto-regenerates `src/assets/Levels.h` when any `level_*.yml` is modified; add bounds and footprint checks in the parser (fixture coords within `map_width/height`, no duplicate grid positions, etc.)

---

### [PH-TG]: Phase 7 - Tileset & Ground GFX Enhancement (COMPLETED)

Enhance tile visuals and introduce custom floor/ground variations leveraging `assets/images/tileset.aseprite` and sparse `TilePlacement` arrays.

- [x] `[TETY]`: TileType Expansion - add `Water`, `Stone`, `Dirt` to `enum class TileType` in `src/alx/Tile.h`
- [x] `[RMWL]`: Deprecate & Remove WallPlacement for TilePlacement - replace `WallPlacement` with `TilePlacement` in `Levels.h` and `MainScene.cpp`
- [x] `[YCTM]`: YAML Tile Character Legend - update `pack_assets.cr` level parser character legend to support `~` (Water), `o` (Stone), `,` (Dirt) in ASCII string grids and emit `std::span<const TilePlacement> custom_tiles`
- [x] `[TSAT]`: Tileset Aseprite Asset - create `assets/images/tileset.aseprite` with tagged frames (`"floor"`, `"wall"`, `"water"`, `"stone"`, `"dirt"`) auto-exported to `Assets::Images::tileset_frames[]` and `Assets::Images::tileset_tags[]` by `pack_assets.cr`
- [x] `[TTRN]`: Tile Render Refactor - update `MainScene::draw_terrain_tile` to blit tileset RLE frames from `Assets::Images` instead of primitive rectangle borders

---

### [PH-DL]: Phase 8 - Dual-Layer YAML Level Grid (`tiles:` & `objects:`) & Multi-Terrain Support (COMPLETED)

Refactor YAML level source format from a single `grid:` block into aligned `tiles:` (ground layer) and `objects:` (overlay layer) ASCII string grids, enabling fixtures, walls, and structures to be placed on top of stone, dirt, and water surfaces without erasing ground terrain GFX.

- [x] `[DLFS]`: Dual-Layer Level Format Spec - update `assets/levels/*.yml` files to replace single `grid:` with dual `tiles:` (ground surface: `.`, `~`, `o`, `,`) and `objects:` (overlay entities: `.`, `P`, `R`, `S`, `s`, `D`, `#`) ASCII blocks
- [x] `[DLCR]`: Dual-Layer Crystal Parser Refactor - update `LevelExporter` in `toolchain/src/export/levels/level.cr` to parse `tiles:` into `custom_tiles` (`~`, `o`, `,`) and `objects:` into `fixtures` (`P`, `R`, `S`, `s`, `#`) and `dark_tower_spawns` (`D`)
- [x] `[DLVR]`: Grid Alignment & Footprint Validation - add validation in `level.cr` ensuring `tiles:` and `objects:` have identical line and char dimensions (`map_width` x `map_height`), validating object footprints across layers
- [x] `[DLMT]`: Multi-Terrain Rendering Verification - verify `task assets` and `task build` auto-regenerate `src/assets/Levels.h`, enabling fixtures and walls to render cleanly on top of dirt, stone, and water terrain surfaces

---

### [PH-WF]: Phase 9 - Wall Fixture & Line-of-Sight (LOS) Refactor (COMPLETED)

Refactor `Wall` from a terrain `TileType` into a solid, buildable/placeable 1x1 `FixtureType::Wall` with 15 HP, 2 Alloy cost, player shadow occlusion, and Enemy Line-of-Sight (LOS) blocking.

- [x] `[WFEX]`: FixtureType Expansion & Constants - add `Wall` to `enum class FixtureType` in `src/alx/Fixture.h`, define `WALL_MAX_HP = 15`, set 1x1 footprint, 0 ports, and 100% tile ground AABB in `Fixture.cpp`; remove `TileType::Wall` from `src/alx/Tile.h`
- [x] `[WFNT]`: Network Fixture Integration - update `Network::is_solid`, `Network::place_fixture`, and `Network::draw` in `src/alx/Network.cpp` to recognize and register `FixtureType::Wall`
- [x] `[WFLOS]`: Enemy Line-of-Sight (LOS) Raycasting - update `WorldCollision::has_line_of_sight` signature in `WorldCollision.h`/`.cpp` to accept `const Network* network` and return `false` when stepping through a `FixtureType::Wall` cell; update `EnemyManager::update_player_aggro` invocation
- [x] `[WFPL]`: Player Build & Placement Integration - add `Wall` to player build selection cycle (`Pipe` -> `Refiner` -> `Spire` -> `Wall`), set Alloy build cost = 2 in `Player::fixture_cost`, and update placement preview box rendering
- [x] `[WFRN]`: Wall Fixture Rendering - implement `DrawFixtures::wall()` in `DrawFixtures.h`/`.cpp` blitting the `"wall"` frame from `Assets::Images::tileset` at `Layer::WorldObj` with Y-sorting (`world_y + tile_size`), ensuring player shadows are occluded behind wall structures
