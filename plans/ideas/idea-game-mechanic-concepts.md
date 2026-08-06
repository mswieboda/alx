# Aetherlux (alx) - High-Level Game Polish & Mechanic Concepts

## Overview
This document serves as a scratchpad and architectural proposal for game polish, traversal mechanics, enemy archetypes, network energy types, infrastructure fixtures, health restoration systems, and novel high-level gameplay mechanics for *Aetherlux* (`alx`).

All designs strictly respect the **GBA Controller Layout Constraint** (D-Pad, A, B, L, R, Start, Select) and maintain C++20 clean architecture standards without relying on magic numbers or invasive screen HUD clutter.

---

## 1. Player Sprint & Stamina Traversal System

### [SPRT] Player Sprint & Dynamic Movement
- **GBA Input Trigger**: 
  - **Primary Trigger Option A (Double-Tap D-Pad)**: Quickly double-tapping any direction vector on the D-Pad (`W/A/S/D` or Arrow Keys) enters **Sprint Mode** for as long as the direction is held down. Double-tap window: `0.22s`.
  - **Alternative Trigger Option B (Hold Button B while Moving)**: Holding `Button B` (`K` / `X`) while feeding D-Pad movement vectors triggers Sprinting (classic Mario / Pokemon dash pattern). Since `Button B` without `R-Shoulder` is currently reserved for secondary action/cancel, holding it while in motion cleanly transitions the player into high-speed sprint.
- **Sprint Physics & Speed Multiplier**:
  - Normal Walk Speed: `15.0 px/s` (or defined dynamic base speed).
  - Sprint Speed: `27.0 px/s` (`1.8x` dynamic multiplier).
- **Stamina & Cooldown Mechanics (No Visual HUD)**:
  - **Max Stamina Pool**: `3.0s` continuous sprint capacity.
  - **Stamina Depletion Rate**: `1.0 unit / sec`.
  - **Stamina Recharge Delay**: `0.4s` pause after sprinting stops before recharge begins.
  - **Stamina Recharge Rate**: `1.5 units / sec` (`2.0s` to full recovery from zero).
  - **Exhaustion State**: If stamina hits `0.0`, the player enters an **Exhausted State** for `1.2s`. Movement speed drops to `0.6x` walk speed (`9.0 px/s`) and sprinting is locked until stamina reaches at least `40%`.
- **Diegetic Audiovisual Feedback (Zero HUD clutter)**:
  - **Particle Dust Trail**: Spawns subtle ground dust clouds (`ParticleSystem`) at the player's feet every `0.08s` while sprinting.
  - **Sprite Animation Speed**: Accelerates player legs/walking animation speed by `1.8x` and applies a slight forward torso pixel tilt during sprint.
  - **Wind/Speed Lines**: Subtle transparent vector speed streaks surrounding the player sprite when max sprint velocity is achieved.
  - **Exhaustion FX**: When exhausted, puffing steam/sweat drop particles float above the player's head alongside a low heavy-breathing audio cue (`miniaudio` SFX).
- **C++ Data Structure Proposal**:
```cpp
struct PlayerSprintState {
    static constexpr float WALK_SPEED = 15.0f;
    static constexpr float SPRINT_SPEED_MULT = 1.8f;
    static constexpr float MAX_STAMINA = 3.0f;
    static constexpr float RECHARGE_RATE = 1.5f;
    static constexpr float RECHARGE_DELAY = 0.4f;
    static constexpr float EXHAUSTION_DURATION = 1.2f;
    static constexpr float EXHAUSTION_PENALTY_MULT = 0.6f;
    static constexpr float DOUBLE_TAP_WINDOW = 0.22f;

    float stamina_cur = MAX_STAMINA;
    float recharge_delay_timer = 0.0f;
    float exhaustion_timer = 0.0f;
    float last_dir_tap_timer = 0.0f;
    uint8_t last_tap_dir = 0; // 0: None, 1: Up, 2: Down, 3: Left, 4: Right
    bool is_sprinting = false;
    bool is_exhausted = false;
};
```

---

## 2. Diverse Enemy Archetypes & Behaviors

Inspired by *Binding of Isaac*, *Hades*, and *Hollow Knight*.

### [TWSL] Twilight Slime (Swarm Leaper)
- **Movement Style**: Periodic hop/lunge behavior. Remains stationary for `0.8s` coiling down, then performs an explosive arc leap toward the player or nearest network pipe, vaulting over low obstacles.
- **Attack Style**: Contact splash damage upon landing + leaves a temporary `3x3` tile **Twilight Tar Puddle** on the ground for `4.0s` that slows player movement speed by `40%`.
- **C++ Struct**:
```cpp
struct TwilightSlimeState {
    float coil_timer = 0.8f;
    float leap_progress = 0.0f;
    float leap_duration = 0.45f;
    float target_x = 0.0f;
    float target_y = 0.0f;
    bool is_leaping = false;
};
```

### [SHST] Shadow Stalker (Ambush Flanker)
- **Movement Style**: Subterranean shadow form. Invulnerable while submerged inside floor shadows, stealthily circling around to the player's rear or flank at high speed (`24.0 px/s`).
- **Attack Style**: Unsubmerges behind the player, charges for `0.3s`, and unleashes a `3-burst Twilight Needle` projectile fan (`45°` arc) before submerging back into the floor shadows.
- **C++ Struct**:
```cpp
struct ShadowStalkerState {
    bool is_submerged = true;
    float submerge_timer = 3.0f;
    float unborrow_windup = 0.3f;
    int burst_count_remaining = 0;
};
```

### [TWMB] Twilight Corpse-Bomb (Suicide Bomb Fixture Demolisher)
- **Movement Style**: Relentless direct charge toward the highest-density pipe network junction or active Refiner spire. Ignores player unless blocked.
- **Attack Style**: Upon reaching within `12px` of a pipe or structure, enters a `1.0s` lock-in windup (flashing intense crimson/white glow and swelling in scale), followed by a violent `48px` explosion that vaporizes conduits and damages adjacent entities.
- **C++ Struct**:
```cpp
struct TwilightCorpseBombState {
    float arming_timer = 1.0f;
    float blast_radius = 48.0f;
    int blast_damage = 4;
    bool is_armed = false;
};
```

### [VOID] Void Sentinel (Armored Frontal Juggernaut)
- **Movement Style**: Slow, heavy rhythmic march (`8.0 px/s`). Equips a frontal twilight shield that reflects/blocks all incoming melee and ranged attacks from its front `120°` arc.
- **Attack Style**: Telegraphed shield-bash that creates a `60px` linear shockwave, knocking back the player and stunning fixtures for `2.0s`. Must be flanked from behind or sprinted around.
- **C++ Struct**:
```cpp
struct VoidSentinelState {
    float shield_angle = 0.0f; // Facing direction in radians
    float bash_windup = 0.6f;
    bool is_bashing = false;
    float shockwave_length = 60.0f;
};
```

### [TWMR] Twilight Mortar Weaver (Zone-Denial Artillery)
- **Movement Style**: Stalking perimeter keeper. Continuously backs away from the player to maintain a safe distance (`96px - 144px` range).
- **Attack Style**: Lobs high-arc twilight plasma mortars into the air. Highlights a targeted `24px` red floor warning indicator ring where the mortar lands `1.5s` later, dealing heavy splash damage and leaving lingering twilight corruption.
- **C++ Struct**:
```cpp
struct MortarWeaverState {
    float mortar_cooldown = 3.5f;
    float target_landing_x = 0.0f;
    float target_landing_y = 0.0f;
    float air_time_remaining = 1.5f;
    bool mortar_in_flight = false;
};
```

---

## 3. New Network Energy & Resource Types

### [KNET] Kinetic Hydraulic Fluid (Pressure Grid)
- **Concept**: Mechanical waterwheel turbine power. Transported under fluid pressure (`PSI`).
- **Mechanics**: Long pipe runs experience pressure friction loss (`-5% PSI` per tile). Must be reinforced with inline **Pump Stations** to maintain fluid drive to distant spires.

### [TFLX] Thermal Flux (Heat Grid / Magma Energy)
- **Concept**: Ultra-high temperature volcanic energy harvested from twilight thermal vents.
- **Mechanics**: Melts frozen twilight obstacles and powers heavy refiners. Thermal pipes overheat if routed immediately adjacent to each other for long stretches, requiring **Radiator Sink** tiles to dissipate excess heat.

### [SPRT] Radiant Photonic Essence (Solar Beam Grid)
- **Concept**: High-frequency light energy harvested from Solar Collectors during daylight room phases.
- **Mechanics**: Transferred wire-lessly via **Prism Relays** in straight lines across chasms and open room space without needing contiguous ground pipes.

### [BRES] Bio-Sap Essence (Organic Growth Grid)
- **Concept**: Living botanical sap extracted from cleansed twilight flora.
- **Mechanics**: Slowly heals and auto-repairs connected fixtures over time (`+1 HP / 5s`), but can become infected with twilight blight if dark creep touches an unshielded node.

---

## 4. New Network Fixtures & Infrastructure

### [CRYB] Crystal Prism Relay (Wireless Beam Transit)
- **Function**: Replaces ground pipes for long-distance or over-chasm energy transmission. Projects a continuous light/energy beam across up to `8 tiles` in a cardinal line of sight.
- **Mirror Redirectors**: Placing a `45° Prism Mirror` allows refracting the beam around corners cleanly.

### [AIRC] Pneumatic Conduit Pad (Player & Resource Jump-Pad)
- **Function**: Powered by Kinetic/Air pressure network lines. Stepping onto the launch pad instantly propels the player across room gaps or between distant sub-bases.

### [SHLD] Aegis Hex-Shield Generator
- **Function**: Consumes `2 units/sec` of network energy to project a `3x3` tile energy dome shield protecting enclosed pipes, refiners, and player from enemy mortar projectiles and suicide blasts.

### [SOLB] Solarium Bath (Regen Sanctuary Area)
- **Function**: A `2x2` radiant tile fixture fed by refined Light Energy. Standing within its warm aura grants continuous health regeneration and cleanses slow/debuff effects.

### [TURR] Lightbeam Defense Sentry
- **Function**: Automated defense node connected to the pipe grid. Consumes stored light energy to automatically fire targeted light bolts at creeping enemies within a `5-tile` radius (`1.2s` attack cadence).

---

## 5. Player Health Recovery Systems

### [SOLB] Sanctuary Bathing (Fixture Healing)
- **Mechanic**: Standing near an active **Solarium Bath** or fully powered **Refiner** slowly mend player health (`+1 HP per 2.0s` stationary contact). Accompanied by soothing visual light particle wisps floating upwards.

### [ALDP] Alloy Core & Essence Drops (Combat Drops)
- **Mechanic**: Defeated enemies have a `25%` chance to drop **Twilight Essence Shards** or **Alloy Cores**. Collecting 3 Essence Shards synthesizes a full health heart (`+1 HP`) with a distinct GBA pickup chime.

### [PARR] Timed Parry & Light Siphon (Skill Combat Healing)
- **Mechanic**: Tapping `Button B` (`Cancel/Secondary`) right before an enemy melee strike or projectile lands executes a **Perfect Parry**, absorbing the kinetic strike to immediately restore `0.5 HP` and release a radial knockback wave.

### [FRUT] Radiant Flora Harvesting (Grid Environmental Drops)
- **Mechanic**: Purifying twilight corruption from tiles spawns harvestable **Radiant Berries** on freshly cleansed ground.

---

## 6. Brainstormed High-Level Game Mechanic Concepts

### [OVER] Fixture Overclocking (GBA Combo Interaction)
- **Concept**: Pressing `R-Shoulder + Action (A)` while standing on an already built fixture triggers an **Overclock State** for `10.0s`.
- **Effect**: Production / firing speed is boosted by `200%`, but consumes energy at `300%` rate and risks bursting pipe conduits if sustained for too long without thermal cooling.

### [TWBL] Dynamic Twilight Corruption Creep (Territory Control)
- **Concept**: Unpurified areas actively grow **Twilight Tendrils** toward nearby network pipes every `10.0s`. If tendrils breach an unshielded pipe, the pipe becomes corrupted, inverting its energy flow to empower nearby enemy spawns.

### [MODS] Spire Crystal Socketing (Custom Modifiers)
- **Concept**: Spires feature socket slots for **Modular Crystals** discovered in secret room caches.
- **Crystals**: *Overload Crystal* (increases splash range), *Alloy Catalyst* (reduces alloy building costs by 25%), *Siphon Crystal* (restores player HP when enemies die near the spire).

### [RSTR] Room-Wide Overcharge Surge (Chain Reactions)
- **Concept**: Reaching `100%` twilight purification in a room triggers a **Purification Overcharge**.
- **Effect**: All connected spires emit a massive holy shockwave that instantly cleanses all remaining dark tiles and vaporizes non-boss enemies in the room with a satisfying screen pulse FX.

### [WEAP] Dual-Form Weapon Stance (Select Button Toggle)
- **Concept**: Pressing `SELECT` (`Tab` / `Space`) toggles the player's weapon mode between **Light Blade** and **Twilight Lancer**.
- **Light Blade**: Fast `180°` melee arc with high knockback (great for swarm slimes).
- **Twilight Lancer**: Piercing linear thrust (`48px` reach) that ignores heavy front armor (ideal for Void Sentinels and Corpse-Bombs).

### [ECHO] Twilight Memory Echoes (Mini-Events)
- **Concept**: Discovering ancient builder altars triggers `30-second` micro-defense challenges. Successfully defending the altar yields rare Alloy schematics and permanent energy capacity upgrades.
