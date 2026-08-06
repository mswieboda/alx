# [EP-DTJ]: Dark Tower Spawning, Visual Juice & Mechanics Plan

This document outlines the phased roadmap for implementing the **Dark Tower** (`StructureType::DarkTower`) spawning system, subterranean emergence animations, invulnerability states, and corruption aura mechanics in *Aetherlux* (`alx`).

---

## Architecture & Data Structures Overview

```cpp
// Suggested C++ Data Structures for Dark Tower emergence and ley-nodes
namespace alx {

enum class DarkTowerAnimState : uint8_t {
    Dormant = 0,
    Telegraph,   // Ground cracking, subtle rumble, particle hints
    Rising,      // Vertical displacement emergence with subterranean masking
    Settling,    // Snapping into position, dirt particles, smoke swirl
    Active       // Fully operational spawner and combat entity
};

struct DarkTowerLeyNode {
    int tile_x{0};
    int tile_y{0};
    bool is_occupied{false};
    float last_spawn_time{0.0f};
};

struct DarkTowerEmergenceAnim {
    DarkTowerAnimState state{DarkTowerAnimState::Dormant};
    float state_timer{0.0f};
    float vertical_offset_px{16.0f}; // Hidden portion offset in pixels (0 when fully emerged)
    float shake_offset_x{0.0f};
    float shake_offset_y{0.0f};
    bool is_invulnerable{true};
};

} // namespace alx
```

---

## Phased Implementation Plan

### `[PH-DTSM]`: Phase 1 - Corrupted Ley-Nodes & Inverse Twilight Spawning
*Focus: Level-based fixed placement nodes (`CLN`) combined with inverse-twilight spawn scaling.*

- [ ] `[CLN]`: **Pre-determined Ley-Node Grid Integration**
  - Define static array/list of `DarkTowerLeyNode` coordinate pairs per level layout.
  - Query grid metadata to ensure chosen Ley-Nodes are on valid tile positions.
- [ ] `[ITS]`: **Inverse-Twilight Spawn Interval Scaling**
  - Calculate dynamic spawn interval based on room twilight level: as twilight drops closer to 0%, the spawn cooldown range decreases (e.g., from `15s-25s` down to `5s-10s`).
  - Creates escalation tension as the player successfully purifies the room.
- [ ] `[LNSEL]`: **Unoccupied Ley-Node Selection**
  - Select spawn locations strictly from unoccupied `DarkTowerLeyNode` slots on the current level.

---

### `[PH-DTSA]`: Phase 2 - Subterranean Emergence Visuals & Invulnerability State
*Focus: Step-by-step Subterranean Erupt (`SBE`) animation state machine with frame-perfect vertical masking.*

- [ ] `[IVP]`: **Invulnerable Emergence Phase**
  - Flag `WorldStructure` as invulnerable (`is_invulnerable = true`) during `Telegraph`, `Rising`, and `Settling` anim states.
  - Disable hitboxes/damage receipt until transitioning to `DarkTowerAnimState::Active`.
- [ ] `[SBE-TEL]`: **Telegraph State (0.0s - 1.0s)**
  - Emit ground crack particles and subtle tile rumble hints at target Ley-Node location.
  - Start initial telegraph timer prior to physical surfacing.
- [ ] `[SBE-RISE]`: **Rising / Subterranean Masking (1.0s - 2.5s)**
  - Animate `vertical_offset_px` from tile height down to `0.0f`.
  - Apply render clip rectangle / bottom-row masking so subterranean lower section remains hidden below ground line as tower rises.
  - Emit continuous dirt/debris particle burst at base seam.
- [ ] `[SBE-SETT]`: **Settling & Dark Smoke Swirl (2.5s - 3.0s)**
  - Snap tower to final `0.0f` offset.
  - Spawn twirling dark smoke particle emitters around the tower perimeter (`TVR` swirl variant).
  - Transition state to `DarkTowerAnimState::Active` and clear invulnerability flag.

---

### `[PH-CAW]`: Phase 3 - Corruption Aura Wave & Movement Audit
*Focus: Room twilight re-infestation pulse and non-breaking player speed aura investigation.*

- [ ] `[CAP]`: **Periodic Corruption Aura Pulse**
  - Implement 5.0-second interval timer for `Active` Dark Towers to emit a expanding corruption pulse.
  - Increase ambient twilight on surrounding tiles within a designated tile radius (e.g., 3x3 radius).
  - Keep twilight increase balanced to avoid unwinnable room lockouts.
- [ ] `[PSA]`: **Player Speed Aura Investigation**
  - Audit float speed scaling (`m_speed * aura_modifier`) in `Player.cpp` to ensure pixel-perfect sub-pixel movement and collision resolution remain intact when slowed near a Dark Tower.
  - Implement a mild speed reduction aura if safety constraints are verified.

---

### `[PH-DTJP]`: Phase 4 - Camera Shake & Proximity Juice
*Focus: Distance-scaled screen shake and sound/visual polish.*

- [ ] `[PCS]`: **Proximity-Based Camera Shake**
  - Calculate distance between player camera position and settling Dark Tower.
  - Apply camera shake (`shake_offset`) upon settling step ONLY if player is within viewable camera distance threshold.
- [ ] `[VFXP]`: **Particle Emitter & Sound Polish**
  - Fine-tune debris particle lifetimes, smoke swirl rotation speeds, and audio cue triggers for emergence phases.

---

### `[PH-CWRN]`: Phase 5 - Corrupted Root Nodes System (Low Priority / Future Expansion)
*Focus: Tactical multi-target destruction requirement for Dark Towers.*

- [ ] `[RNOD]`: **Root Node Anchors**
  - Spawn 2-3 satellite root nodes on adjacent tiles when a Dark Tower becomes active.
- [ ] `[RNSH]`: **Shielded Core Mechanics**
  - Maintain tower shield until all satellite root nodes are shattered by player attacks or Spire blasts.
