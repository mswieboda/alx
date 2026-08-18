# [EP-GHST]: Ghost State, Lives & Diegetic Respawn Mechanics

**Status**: Brainstorming & Concept Scratchpad
**Priority**: Low (Post-Jam / Future Polish Idea)
**Related Epics**: `[GOVR]` (Game Over Flow), `[COMB]` (Combat & Player State)

---

## 1. System Overview & Core Loop

```mermaid
graph TD
    A[Player HP reaches 0] --> B{Remaining Lives > 0?}
    B -->|No (0 Lives)| C[Trigger Full Game Over Sequence]
    B -->|Yes (>0 Lives)| D[Deduct 1 Life & Enter Ghost State]

    D --> E[Ghost Phase: 30.0s Countdown]
    E --> F[Intangible Scouting: Float through Mobs & Walls]
    E --> G[Phantom Actions: Hollow SFX + Translucent Visuals (0 Damage)]
    
    E --> H{Revival Trigger}
    H -->|Passive Timer hits 0s| I[Radiant Light Burst & Solidify at Current Pos]
    H -->|Hover over Purified Spire| J[Accelerated Channelling (3.0x speed)]
    
    J --> I
    I --> K[Full HP Restored + Regain Collision & Solid Combat]
```

---

## 2. Mechanic Breakdown

### [LVSY]: Multi-Life System (Spark Cores)
- **Life Pool**: Player begins each level with **3 Spark Cores** (Lives), displayed diegetically on the HUD next to HP (e.g. `\x0F \x0F \x0F` glowing spirit orbs).
- **Core Depletion**:
  - HP hitting `0` consumes 1 Spark Core and triggers the 30s Ghost Phase.
  - If HP reaches `0` with `0` remaining Spark Cores, the player permanently falls, initiating the standard **Game Over Screen** ("Retry" / "Main Menu").

---

### [GHST]: Ghost State Behavior & Intangibility
- **Spectral Aesthetics**:
  - Player sprite renders with 40%–50% translucency / alpha dithering.
  - Emits floating ethereal cyan and violet particle wisps (`ParticleSystem`) that trail behind movement.
  - Movement speed slightly boosted (`1.15x` hover glide).
- **Intangibility (No Collision)**:
  - Player collision against enemies, mob projectiles, and physical fixtures is disabled (`is_intangible = true`).
  - Player cannot take damage, be knocked back, or body-block mobs during Ghost State.
- **Continuous World Simulation**:
  - The game does **not pause**. Dark Towers continue pulsing, enemies continue marching toward fixtures, and Twilight continues spreading—creating tension and high stakes while the player is intangible!

---

### [PDUD]: Phantom "Dud" Actions & Visual Feedback
- **Ghost Sword Swings**:
  - Pressing `Action::Attack` (Button A / `J`) swings a semi-transparent, luminous phantom sword blade.
  - Plays a hollow, muffled spectral swoosh SFX (`miniaudio`).
  - Produces zero physical hitboxes, deals 0 damage, and passes through enemies without hitting them.
- **Ghost Spark Casting**:
  - Firing `Action::ManaSpark` produces a fizzy, harmless light spark that pops without transferring mana or damaging mobs.
- **Ghost Blueprinting**:
  - Build mode allows placing glowing holographic fixture outlines for layout planning, but foundation construction and Alloy deduction remain disabled until solidification.

---

### [RESP]: Revival & Solidification Mechanisms

#### 1. Passive 30-Second Countdown
- A 30.0s countdown timer runs automatically.
- A radial spirit ring / countdown gauge pulses around the ghost sprite.
- When the timer reaches `0.0s`, the player triggers a radiant light burst, restoring 100 HP and returning to physical solid form.

#### 2. Spire Resonance (Fast-Revival Channelling)
- If the player floats inside the aura of an active, purified **Spire** or **Ley-Seep**, pure light energy channels into the ghost.
- **Revival Acceleration**: Countdown ticks down **`3.0x` faster** (`10 seconds` instead of 30 seconds).
- Plays a bright rising resonance chime, rewarding map awareness and network defense.

---

## 3. C++ Data Structure Proposal

```cpp
namespace alx {

struct PlayerGhostState {
    static constexpr int DEFAULT_LIVES = 3;
    static constexpr float GHOST_DURATION_SEC = 30.0f;
    static constexpr float SPIRE_CHANNEL_MULT = 3.0f;
    static constexpr float GHOST_SPEED_MULT = 1.15f;
    static constexpr uint32_t GHOST_SPRITE_ALPHA = 0x80FFFFFF; // 50% opacity

    int lives_remaining{DEFAULT_LIVES};
    float ghost_timer{0.0f};
    bool is_ghost{false};
    bool is_channelling_spire{false};

    [[nodiscard]] bool can_act() const noexcept { return !is_ghost; }
    [[nodiscard]] float remaining_ratio() const noexcept {
        return (GHOST_DURATION_SEC > 0.0f) ? std::clamp(ghost_timer / GHOST_DURATION_SEC, 0.0f, 1.0f) : 0.0f;
    }
};

} // namespace alx
```

---

## 4. Pros & Cons Analysis

### Pros:
1. **Eliminates Abrupt Restarts**: Softens punishment and lets players scout layout vulnerabilities while recovering.
2. **High Dramatic Tension**: Watching dark towers corrupt pipes while helpless as a ghost creates emergent "save the network" panic.
3. **Spire Synergy**: Encourages keeping at least one Spire purified as a safe revival anchor point.

### Cons / Jam Scope Considerations:
1. **Network Griefing Risk**: 30 seconds of intangibility during Frenzy or high twilight could leave the base decimated before the player can revive.
2. **Implementation Scope**: Requires alpha blit rendering for player sprites, intangible pathing checks, and HUD spirit core icons.
