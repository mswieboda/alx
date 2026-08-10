---
name: game-design-mechanics
description: >-
  Evaluates game design ideas, feature proposals, and mechanics for Aetherlux (alx) against 2D game design best practices, drawing inspiration from top-down action-adventures (Zelda: A Link to the Past, Minish Cap, Golden Sun) and automation/crafting games (Factorio, Mindustry). Focuses purely on design, ergonomics, and game loop evaluation without inspecting codebase files unless requested.
---

# Game Design & Mechanics Evaluation Skill

Use this skill when evaluating game design proposals, feature ideas, combat tweaks, or automation mechanics for *Aetherlux* (`alx`).

## Primary Focus & Guidelines

- **Pure Design & Mechanics Focus**: Evaluate ideas based on game loop feedback, player feel, pacing, and mechanics design. Do **NOT** read or search C++ codebase files (`src/`) unless explicitly requested by the user.
- **Reference Benchmarks**:
  - **Top-Down Action-Adventures** (*Zelda: A Link to the Past*, *Minish Cap*, *Golden Sun*): Snappy player verbs, clear room/grid pacing, visual/audio impact, intuitive puzzle design, and satisfying combat feel.
  - **Micro-Automation & Logistics** (*Factorio*, *Mindustry*): Clear resource conversion loops, readable power/mana network flow, spatial layout constraints, and defense/refining bottlenecks.

---

## Evaluation Criteria

When evaluating a proposed mechanic or feature, analyze it against the following 5 criteria:

1. **Core Loop & Friction**
   - Does this feature support or clutter the primary loop (*Explore Room $\rightarrow$ Gather Alloy $\rightarrow$ Lay Mana Pipes $\rightarrow$ Refine Twilight $\rightarrow$ Purify Grid*)?
   - Does it create meaningful choices or unnecessary micro-management?

2. **Systemic Synergy**
   - How naturally does it connect with Twilight reduction, Mana Pipe networks, Refiners, Alloy collection, and Enemy Waves?

3. **Readability & Player Feedback**
   - Is state change instantly readable in a 2D GBA pixel-art style? Can the player understand what went wrong or right without checking menus?

4. **Controls & Ergonomics**
   - Does the input flow smoothly on both modern gamepads (X/Y, Triggers) and keyboard/GBA layouts without awkward finger stretches or redundant holds?

5. **Pacing & Escalation**
   - How does the mechanic scale from single-room skirmishes to multi-room twilight purification?

---

## Response Formatting

When responding to a design prompt:

1. **Executive Summary / Design Verdict**: Concise analysis highlighting key strengths, potential friction points, and overall alignment with GBA/2D automation action-adventures.
2. **Genre Benchmarks**: Draw direct comparisons to how similar mechanics are handled in *Factorio*, *Mindustry*, *Link to the Past*, *Minish Cap*, or *Golden Sun*.
3. **Actionable Recommendations**: Format proposed mechanics or improvements using the Codename Acronym Format:
   `[ACRONYM]`: Title - Description (e.g., `[MPRS]`: Mana Pipe Pressure Surge - Brief description of mechanics/values).
