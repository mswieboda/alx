# Aetherlux Game Jam Priority Analysis & Roadmap

**Deadline**: September 1st, 2026 (~19 days from now)
**Current Date**: August 12th, 2026

---

## 1. Updated Current State Assessment

| Subsystem / Layer | Current Status | Assessment |
|-------------------|----------------|------------|
| **Engine & Software Rendering** | ✅ Solid | `minifb` 320x240 renderer, draw pipeline, non-owning `RenderTarget` context — complete. |
| **Audio Engine & SFX Triggers** | ✅ Advanced | 32-voice SFXR voice pool, `pocketmod` tracker integration, preset SFX for swipes, hits, build snaps, refiner bubbles, dark tower pulses, and spire crackles are already wired! |
| **Particle Systems & Visual Juice** | ✅ Advanced | Mana flow particles in pipes, spire light furnace, sword strike sparks, enemy blood/wound particles, refiner bubbles, and dark tower wave emissions are fully functional. |
| **Mana Network Simulation** | ✅ Solid | Seep -> Pipe -> Refiner -> Spire simulation with dark/light flow mechanics — works. |
| **Player Movement & Combat** | ✅ Functional | 8-directional movement, melee sword swing, i-frame damage, knockback. |
| **Build Mode Systems** | ✅ Functional | Grid cursor, fixture placement, cost deduction, cardinal locking (`PH-BLDM`). |
| **Enemies & Dark Towers** | ⚠️ Functional | ShadowCreep / Dusk mob, egg hatching, dark tower emergence, and pulse logic exist. |
| **Twilight System** | ✅ Functional | Per-room twilight level, spire reduction, screen-space overlay. |
| **Title Screen Scene** | ✅ Missing | Game boots directly into gameplay without a main menu. |
| **Pause Menu System** | ✅ Solid | Pause overlay with Resume, Retry, and Main Menu options. |
| **Game Over Experience** | ✅ Missing | Player death currently auto-respawns instantly with no dramatic feedback or menu choices. |
| **Level Clear / Victory Experience** | ✅ Placeholder | Win condition currently displays static "YOU WIN!" text without stats or level transition. |
| **Scaffolded 3-Level Content** | ⚠️ Partial | Level 1 map exists; Levels 2 & 3 are 5x5 placeholders. |
| **Scene Transitions** | ✅ Missing | Hard cuts between states; no fade-to-black or iris wipe transitions. |

> [!NOTE]
> **Key Takeaway**: The engine, audio synthesis, particle effects, and network simulation are much farther along than initially assumed. The primary remaining hurdle is building the **Game Shell** (Title, Game Over, Level Clear, Pause), adding **Readability** (Twilight Momentum Barometer, Contextual Prompts), and authoring the **Scaffolded 3-Level Progression**.

---

## 2. Refined Game Design Decisions

### A. Death & Game Over Flow
- **Decision**: No roguelike mechanics, no complex penalty system.
- **Flow**: Player HP hits 0 -> 1.0s freeze + death particles/SFX -> Fade to black -> **Game Over Screen** ("Retry Level" / "Quit to Title").
- **Level Reset**: Retrying resets the level layout and network state from scratch.

### B. Purification Victory & 15-Second Hold
- **Decision**: Shorten twilight hold window from 60 seconds down to **15 seconds**, turning it into an intense "Hold the Line" climax.
- **Flow**:
  1. Room Twilight drops below <1% -> Visual shift (screen-wide light flash) + audio chime.
  2. Display prominent countdown banner: **"PURIFYING... 15s"**.
  3. Dark Towers enter frenzy mode (accelerated spawn intervals).
  4. If Twilight spikes back above threshold, countdown resets.
  5. Timer hits 0s -> Satisfying light burst -> **Level Clear Screen** (stats + "Next Level" / "Title").

### C. Scaffolded 3-Level Progression
- **Level 1: OBSERVE (Tutorial)**
  - Pre-built operational mana network.
  - Building is disabled.
  - Player learns basic combat, observes how seeps feed refiners and spires, and watches twilight recede.
- **Level 2: CONNECT (Repair / Link)**
  - Partial pre-placed network with missing pipe links.
  - Player learns to lay pipes to connect existing seeps, refiners, and spires.
- **Level 3: CREATE (Hard Challenge)**
  - Build from scratch on a full grid map.
  - Includes **Stone Tile** terrain constraints (players and enemies can walk on Stone, but fixtures *cannot* be built on Stone).

### D. Teaching Without Hand-Holding (Contextual Text Prompts)
- **Decision**: No heavy NPC dialogue boxes or invasive tutorials.
- **Implementation**: Lightweight fade-in / fade-out text overlay prompts at key moments:
  - Level 1 Start: *"Defend the Purification Network"*
  - First Dark Tower Pulse: *"Dark Towers increase twilight"*
  - Twilight Drops: *"Spires are purifying the room"*
  - Level 2 Start: *"Build pipes to connect the network"*
  - Level 3: Blank slate (player applies everything learned).

### E. Twilight Momentum Barometer (`[TWBAR]`)
- **Concept**: High-priority UI indicator solving the "am I winning or losing?" readability problem.
- **Mechanics**:
  - Calculates rolling 15-second average delta of room twilight.
  - Displays a clean visual directional arrow next to the twilight percentage:
    - **`▲` (Light / Yellow)**: Twilight is decreasing (light is winning).
    - **`▼` (Dark / Purple)**: Twilight is increasing (dark is winning).

---

## 3. Prioritized 3-Week Execution Roadmap

### 🔴 WEEK 1 (Aug 13 – Aug 19): Game Shell, Readability & Win/Loss Flow

- [x] **`[TTLS]`**: Title Screen Scene — Logo, "Start Game", "Options", "Quit". 1.0 day.
- [x] **`[TRNS]`**: Scene Transitions — Smooth fade-to-black transitions between scenes. 0.5 day.
- [x] **`[GOVR]`**: Game Over Scene — Death freeze (1s) -> fade to black -> "You Died" (Retry Level / Quit to Title). 0.5 day.
- [x] **`[LVCM]`**: Win Condition Tuning & Level Clear Screen — 15s hold countdown, light flash FX, Level Clear screen with stats. 1.0 day.
- [x] **`[PAUS]`**: Pause Menu Overlay — Resume / Restart Level / Options / Quit to Title. 0.5 day.
- [x] **`[TWBAR]`**: Twilight Momentum Barometer — Rolling 15s average delta calculation + directional arrow indicator. 0.5 day.
- [~] **`[TUTR]`**: Contextual Text Prompts — Fade-in/fade-out message overlay trigger system. 0.5 day.

---

### 🟡 WEEK 2 (Aug 20 – Aug 26): Scaffolded Levels 2 & 3 + Visuals & Ending

- [ ] **`[LV1D]`**: Level 1 Tutorial Polish — Finalize pre-built network layout and observe mode constraints. 1.0 day.
- [ ] **`[LV2D]`**: Level 2 Implementation — Layout with missing pipe connections for player to bridge. 1.5 days.
- [ ] **`[LV3D]`**: Level 3 Implementation — Full build-from-scratch room layout with strategic Stone tiles. 1.5 days.
- [ ] **`[STNBLK]`**: Stone Tile Placement Blocking — Enforce fixture placement checks on non-buildable terrain. 0.5 day.
- [ ] **`[SPRIT]`**: Player & Enemy Sprite Assets — Integrate 8-direction player sprites and mob visual pass. 1.0 day.
- [ ] **`[CRED]`**: Ending / Credits Screen — Simple scrolling text card after Level 3 completion. 0.5 day.

---

### 🟢 WEEK 3 (Aug 27 – Sept 1): Audio Polish, Testing & Jam Submission

- [ ] **`[MSCBG]`**: Tracker Music Polish — Custom / modified `.mod` background music tracks for Title and Gameplay. 1.5 days.
- [ ] **`[BALNC]`**: Balance Tuning — Leverage `--telemetry` and `--report` logs to fine-tune spawn rates and twilight deltas. 1.5 days.
- [ ] **`[SIZE]`**: Binary Size Audit — Verify release build stays under the 1,440 KB floppy disk limit. 0.5 day.
- [ ] **`[SUBMT]`**: Packaging & Submission — Final bug fixes, verification, and jam submission before Sept 1st! 1.0 day.

---

## 4. Scope Discipline (Explicitly CUT Items)

| Cut / Deferred Item | Rationale |
|---------------------|-----------|
| **Procedural / Random Rooms (`[RNDS]`)** | Conflicts with hand-crafted 3-level progression. |
| **Fixture Upgrades (`[UPGR]`)** | Unnecessary complexity for a 3-level jam submission. |
| **Fixture Durability & Repair (`[BRKN]`)** | Replaced by missing pipe connections in Level 2. |
| **Charged Attack (`[CHRG]`) & Ranged Attack (`[PRJT]`)** | Sword swing + Mana Spark are sufficient. |
| **Shadow Phantom (`[SHPH]`) & Dark Slime (`[DKSL]`)** | ShadowCreep / Dusk mob is sufficient for 3 levels. |
| **Save System (`[SAVE]`)** | Not needed for a 5-15 minute linear experience. |
| **Wave System (`[WVSYS]`)** | Continuous Dark Tower spawning works well. |
| **Monolithic Manager Decomposition (`[EP-MGRD]`)** | Pure refactoring; defer to avoid risk before deadline. |
| **Fixture Rotation (`[BLDR]`)** | Not required for existing fixture set. |

---

## 5. Daily Execution Calendar

| Day | Date | Focus Area | Deliverable |
|-----|------|------------|-------------|
| **Day 1** | Aug 13 | `[TTLS]` + `[TRNS]` | Main Title Screen & Fade Transitions |
| **Day 2** | Aug 14 | `[GOVR]` + `[PAUS]` | Game Over Screen & Pause Menu Overlay |
| **Day 3** | Aug 15 | `[LVCM]` | 15s Win Hold Countdown & Level Clear Screen |
| **Day 4** | Aug 16 | `[TWBAR]` + `[TUTR]` | Twilight Momentum Barometer & Contextual Prompts |
| **Day 5** | Aug 17 | `[LV1D]` | Level 1 Pre-built Observe Layout & Polish |
| **Day 6** | Aug 18 | `[LV2D]` | Level 2 Missing Pipe Connection Layout |
| **Day 7** | Aug 19 | `[LV3D]` + `[STNBLK]` | Level 3 Build-from-Scratch Layout & Stone Blocking |
| **Day 8** | Aug 20 | Full Playthrough Test | Validate Level 1 -> 2 -> 3 Loop & Game Shell |
| **Day 9** | Aug 21 | `[SPRIT]` | 8-Direction Player Sprites & Mob Visual Pass |
| **Day 10** | Aug 22 | `[CRED]` | Credits Screen & Game Completion Flow |
| **Day 11** | Aug 23 | Playtesting & Polish | Combat feel, camera, hit flashes |
| **Day 12** | Aug 24 | Build UX Polish | Cost Preview & Placement Feedback |
| **Day 13** | Aug 25 | `[MSCBG]` | Background Music Track Integration |
| **Day 14** | Aug 26 | `[BALNC]` | Telemetry & Headless Balance Tuning |
| **Day 15** | Aug 27 | Bug Fixes | Edge case testing across all 3 levels |
| **Day 16** | Aug 28 | `[SIZE]` Audit | Floppy disk binary size verification (<1,440 KB) |
| **Day 17** | Aug 29 | Final Playtesting | Playthrough verification on gamepads/keyboard |
| **Day 18** | Aug 30 | Release Prep | Final release build compilation |
| **Day 19** | Aug 31 | **SUBMIT** | Submit to 1.44Mb Floppy Disk Game Jam! |
