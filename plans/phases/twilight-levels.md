# [EP-TWLS]: Twilight Levels & Automation Survival Balancing Plan

Comprehensive phased roadmap for Headless Simulation Tooling, Telemetry Viewers, Twilight Net Flow Dynamics, Continuous Dark Tower Emergence, Infrastructure HP / Penalties, and Enemy Aggro Priorities.

---

## Architecture & Mathematical Foundations

### 1. Mathematical Curve Scaling for Cooldowns (`[TWDM-CURV]`)
Cooldown intervals scale dynamically based on current Twilight Level $T \in [0.0, 1.0]$:
- **Low Twilight Threshold**: $T_{\text{low}} = 0.10$ (10% Twilight) $\rightarrow$ Minimum Cooldown (aggressive spawning)
- **High Twilight Threshold**: $T_{\text{high}} = 0.75$ (75% Twilight) $\rightarrow$ Maximum Cooldown (relaxed spawning)
- **Clamped Progress Metric**:
  $$t = \text{clamp}\left(\frac{T - T_{\text{low}}}{T_{\text{high}} - T_{\text{low}}}, 0.0, 1.0\right)$$
- **Scaling Formula (Piecewise Linear with Power Curve Exponent $\gamma$)**:
  $$\text{Cooldown}(T) = \text{lerp}\left(\text{CD}_{\text{min}}, \text{CD}_{\text{max}}, t^\gamma\right) + \text{Random}(-R, +R)$$
  *Default $\gamma = 1.0$ (Linear), $\gamma < 1.0$ (Concave - stays fast longer), $\gamma > 1.0$ (Convex - slows down early).*

### 2. Continuous Global Tower Emergence vs. Single Cap (`[TWDM-SPWN]`)
- **Deprecated Behavior**: `TARGET_ACTIVE_DARK_TOWERS = 1` capped total map towers at 1 and halted emergence timers while active.
- **New Behavior**: Global Dark Tower Emergence Timer runs continuously. Whenever timer triggers:
  1. Find unoccupied `CorruptedDarkTowerTile` (with anti-repetition check against last destroyed tile).
  2. If available tile exists, spawn a new Dark Tower structure (up to all available tiles).
  3. Reset global emergence timer using $\text{Cooldown}(T)$.
  4. If all corrupted tiles are occupied, timer still resets cleanly without spawning.

---

## Phased Implementation Roadmap

### [PH-SIMT]: Phase 1 - Headless Simulator & Telemetry Tooling (COMPLETED)
Focus: Implement fast headless simulation CLI mode, atomic JSON telemetry dumper, `task telemetry` live terminal dashboard viewer, and in-game speed dilation.

- [x] `[SIMT-CLI]`: Headless Simulation CLI Flag - Add `--headless-sim` flag to `main.cpp` supporting `--ticks=N`, `--spires=N`, and `--towers=N` CLI arguments to run game logic without rendering graphics or creating a window.
- [x] `[SIMT-FILE]`: Atomic Telemetry Serializer - Add `TelemetryDumper` in C++ that atomically dumps game state (Twilight %, $dTW/dt$, active towers, egg counts, spire stats) to `/tmp/alx_telemetry.json`.
- [x] `[SIMT-VIEW]`: `task telemetry` Live Terminal Dashboard - Create Crystal viewer script `toolchain/src/telemetry_viewer.cr` and Taskfile alias `task telemetry` that updates ANSI text in-place every 100ms.
- [x] `[SIMT-DIL]`: In-Game Time-Dilation Hotkeys - Add speed multiplier keybindings in `MainScene` (Keys 1-4 for 1.0x, 2.0x, 5.0x, 10.0x frame step simulation speed).
- [x] `[SIMT-STAT]`: Headless Telemetry Exporter - Output end-of-simulation summary metrics (Average Twilight, Twilight $dTW/dt$, Fixture destruction counts, Egg spawn count, Time-to-100% Twilight) to stdout or CSV.

---

### [PH-TWDM]: Phase 2 - Twilight Dynamics & Continuous Tower Spawning (COMPLETED)
Focus: Re-architect Dark Tower emergence, remove single-tower cap, implement dynamic twilight scaling curves, and adjust pulse/egg twilight bumps.

- [x] `[TWDM-CURV]`: Implement Twilight Scaling Curve Helper - Add `calculate_twilight_cooldown(twilight_level, min_cd, max_cd, exponent)` supporting $10\%$–$75\%$ thresholds.
- [x] `[TWDM-TWR]`: Continuous Tower Emergence Engine - Remove `TARGET_ACTIVE_DARK_TOWERS = 1` hardcap in `EnemyManager`. Allow continuous global emergence across all unoccupied corrupted tiles.
- [x] `[TWDM-TWRM]`: Anti-Repetition Corrupted Tile Guard - Track `m_last_destroyed_tile_index` in `EnemyManager` and skip it during random tile selection if other unoccupied tiles are available.
- [x] `[TWDM-PULS]`: Tower Twilight Pulse & Egg Wave Bumps - Set Dark Tower pulse rate to `4.0s`–`5.0s`, pulse amount to `0.04f` (4%), and add a `+0.01f` (1%) Twilight bump when egg waves spawn.
- [x] `[TWDM-CLEA]`: Light Spire Cleanse Rate Adjustment - Update `TWILIGHT_DECREASE_PER_MANA` in `MainScene.h` to `0.010f` (1.0% per tick per spire).

---

### [PH-FXPA]: Phase 3 - Infrastructure HP, Destruction Penalties & Enemy Aggro
Focus: Balance Pipe vs Building HP ratios, set exact destruction twilight penalties, and prioritize enemy attacks on network infrastructure.

- [ ] `[FXPA-HP]`: Rebalance Fixture HP Ratios - Reduce Pipe HP to `3` (from `10`), and Refiner / Spire HP to `20` (from `30`) in `Fixture.h` so pipes break quickly under raid pressure.
- [ ] `[FXPA-PEN]`: Exact Fixture Destruction Twilight Penalties - Update `Network::damage_fixture` to apply exact fixed penalties: `0.075f` (7.5%) for Pipes and `0.250f` (25.0%) for Refiners/Spires.
- [ ] `[FXPA-AGGR]`: Network-First Enemy Aggro Hierarchy - Refactor `update_enemy_ai` in `EnemyManager.cpp` so enemies prioritize targeting network fixtures (Pipes, Refiners, Spires). Enemies only target the player when within close proximity (<= 32px) or upon taking direct damage.

---

### [PH-SIMV]: Phase 4 - Simulation Benchmark & Equilibrium Verification
Focus: Validate the balanced twilight net flow model using the Headless Simulator harness.

- [ ] `[SIMV-BENCH]`: Run 10,000-Tick Headless Benchmarks - Execute headless simulations under 0, 1, 2, and 3 Spire setups to verify Twilight equilibrium curve stability.
- [ ] `[SIMV-PLAY]`: Interactive Time-Dilation Verification - Use 5x/10x time dilation in interactive mode to confirm visual twilight feedback, wave pressure, and pipe defense mechanics feel responsive and rewarding.
