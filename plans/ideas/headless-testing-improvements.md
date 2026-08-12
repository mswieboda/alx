# [EP-SIMP]: Simulated Player Agent & Headless Balancing Barometer Plan

## Overview
This document outlines the design and implementation roadmap for an intelligent **Virtual Player Simulation Agent (`VirtualPlayerAgent`)** in Headless Mode. Current headless testing relies on static player positioning and fixed interval wipes, which fail to accurately model real gameplay because severed conduit pipes immediately halt mana flow until repaired.

By introducing an automated repair queue that tracks destroyed pipe locations and a multi-node threat defense patrol, headless simulation serves as a **statistically rigorous, repeatable barometer** for testing gameplay balance tweaks (such as `DEFAULT_CURVE_EXPONENT`, emergence cooldowns, and `MAX_ACTIVE_DARK_TOWERS`).

---

## High-Level Architecture & Testing Philosophy

```
+-------------------------------------------------------------------------+
|                        Headless Simulation Loop                         |
+-------------------------------------------------------------------------+
                                    |
            +-----------------------+-----------------------+
            |                                               |
            v                                               v
+-----------------------+                       +-----------------------+
|  VirtualPlayerAgent   |                       |    Enemy Simulation   |
|  - Destroyed Pipe Log |                       |  - Tower Emergence    |
|  - 50 Starting Alloy  |                       |  - Egg Spawning       |
|  - Multi-Node Patrol  |                       |  - Conduit Attacks    |
+-----------------------+                       +-----------------------+
            |                                               |
            +-----------------------+-----------------------+
                                    |
                                    v
+-------------------------------------------------------------------------+
|                  Headless Telemetry & Barometer Report                  |
|  - Time to 0% Twilight    - Mana Flow Uptime %     - Repairs / Min      |
+-------------------------------------------------------------------------+
```

---

## Roadmap & Task Breakdown

### `[PH-ARPR]`: Phase 1 - Automated Conduit Pipe Repair & Alloy Economy (`[ARPR]`)
*Focus: Track destroyed pipe tile coordinates and rebuild new pipes at exact locations using starting Alloy.*

- [ ] `[DLOG]`: Destroyed Pipe Location Queue - Maintain a log/queue of destroyed pipe tile coordinates (`std::vector<std::pair<int, int>> m_destroyed_pipe_locations`). Whenever an enemy destroys a pipe fixture, push its exact grid coordinates `(x, y)` onto the queue.
- [ ] `[INIT]`: 50 Starting Alloy Allocation - Grant the virtual player **50 starting Alloy** (`m_player.state.alloy = 50`) at level load in headless mode to simulate gathered starter resources.
- [ ] `[RPLN]`: Exact Pipe Reconstruction Loop - Every $N$ seconds (e.g. 5.0s), if `m_destroyed_pipe_locations` is not empty and player Alloy $\ge 5$, pop the destroyed pipe location, deduct 5 Alloy, and place a new `FixtureType::Pipe` at those exact coordinates.
- [ ] `[RALL]`: Enemy Loot Alloy Feedback - As headless auto-clear/defense kills enemies, add dropped Alloy cores (`+1 Alloy` per kill) to the virtual player's pool to sustain ongoing repairs.

### `[PH-ATDP]`: Phase 2 - Multi-Node Threat Defense Patrol (`[ATDP]`)
*Focus: Expand threat defense patrol across all active player structures (Refiners, Spires, and Pipe Junctions).*

- [ ] `[MNOD]`: Multi-Node Infrastructure Patrol - Check for active enemies within a 96px radius of **all active structures** (Refiner at `{10, 8}`, Spire at `{6, 6}`, and active Pipe coordinates) rather than just spawn point `{9, 9}`.
- [ ] `[CDOWN]`: Player Skill Tier Preset - Configurable combat clearing interval ($T_{\text{clear}}$):
  - **Expert Mode**: $T_{\text{clear}} = 2.5\text{s}$ (rapid combat defense & instant repairs)
  - **Standard Mode**: $T_{\text{clear}} = 5.0\text{s}$ (balanced combat & repair cadence)
  - **Casual Mode**: $T_{\text{clear}} = 8.0\text{s}$ (relaxed defense & slower reaction times)
- [ ] `[AGGO]`: Non-Blocking Aggro Override - Keep the virtual player spawn position at `(-100.0f, -100.0f)` in headless mode so enemies follow pure siege AI without getting stuck on a static player body.

### `[PH-HBAL]`: Phase 3 - Headless Telemetry & Balance Barometer (`[HBAL]`)
*Focus: Expand summary reporting to measure structural integrity, network uptime, and balance efficacy.*

- [ ] `[FLOW]`: Mana Network Flow Uptime Metric - Track `mana_flow_uptime_pct` (percentage of total simulation time that mana was flowing into the Spire).
- [ ] `[RPCT]`: Total Repairs Per Minute Metric - Record `repairs_per_min` to quantify how heavily the player is forced to repair under different enemy spawn settings.
- [ ] `[WTIM]`: Automated Time-to-Win Metric - Track exact `time_to_zero_twilight` under standard player simulation.

---

## Recommended Testing Workflow for Future Balancing

1. **Step 1: Baseline Test**: Run headless simulation with `VirtualPlayerAgent` enabled on Standard Player Mode ($T_{\text{clear}} = 5.0\text{s}$, 50 starting Alloy, 5.0s exact pipe repair cycle).
2. **Step 2: Parameter Sweeps**: Sweep balancing constants (`MAX_ACTIVE_DARK_TOWERS` = 2 vs 3 vs 4, `DEFAULT_CURVE_EXPONENT` = 0.5 vs 0.69 vs 1.0).
3. **Step 3: Evaluate Benchmark Output**:
   - High `mana_flow_uptime_pct` (>85%) + Reasonable `repairs_per_min` (2–4/min) = **Balanced Gameplay**.
   - Low `mana_flow_uptime_pct` (<50%) + Extreme `repairs_per_min` (>10/min) = **Overwhelming / Unfair Pressure**.
   - 100% `mana_flow_uptime_pct` + 0 `repairs_per_min` = **Trivial / Too Easy**.
