# Plan Priorities

This document outlines the prioritized execution roadmap for upcoming features in *Aetherlux* (`alx`). It synthesizes all uncompleted tasks from the phase plans ([dark-tower.md](file:///Users/matt/code/cpp/alx/plans/phases/dark-tower.md), [enemies.md](file:///Users/matt/code/cpp/alx/plans/phases/enemies.md), [fixtures.md](file:///Users/matt/code/cpp/alx/plans/phases/fixtures.md), and [game-design.md](file:///Users/matt/code/cpp/alx/plans/game-design.md)) into structured, sequential implementation batches.

---

## Strategic Prioritization Overview

The roadmap is divided into 5 sequential execution batches designed to transform current mechanics (passive pipe-laying and enemy wandering) into a tense, complete micro-Factorio survival adventure loop:

1. **Batch 1: Core Combat & Aggro Loop** - Enable shadow pests to chase and attack the player directly, introducing survival stakes.
2. **Batch 2: Dark Tower Infrastructure & Spawners** - Replace map-edge spawning with active in-world Dark Tower entities that incubate pests and pulse dark energy.
3. **Batch 3: Mana Spark Projectiles & Game Juice** - Give players a charged ranged rite to combat towers and pests safely, alongside magnet/despawn item polish and hit flashes.
4. **Batch 4: Level Progression & Dynamic Twilight Equilibrium** - Build the 3-act room progression (Cellar -> Hall -> Spire Rooftop) and twilight-linked rubber-band difficulty.
5. **Batch 5: Menus, Tracker Audio & Release Audit** - Add title/pause screen UI, dialogue boxes, `pocketmod` tracker music, and verify binary size < 1.44Mb.

---

## Implementation Batches Breakdown

### Batch 1: Core Combat & Aggro Loop (COMPLETED)

* `[EP-EATK]`: Enemy Player Aggro & Melee Attacks Epic - Threat detection, melee swarming, and player HP defeat loop.
  * `[PH-EATK]`: Enemy Melee Combat Phase - Enable shadow pests (`Enemy.h`) to aggro and damage the Mystic Adept (`Player.h`).
    * `[EPAT]`: Player Aggro Interception - Periodic timer check (1.0s-3.0s) for shadow pests to switch focus from fixtures to chasing the player when within detection radius.
    * `[EMAT]`: Enemy Melee Attack Cycle - Melee attack execution (0.3s windup, 1.0s attack cycle, 1 damage, 0.5s i-frame trigger, player 5 HP max).
    * `[ERET]`: Mid-Attack Retargeting - Dynamic threat-radius evaluation to swap target between fixture and player mid-cycle.
    * `[PDFS]`: Player Defeat & Respawn State - Handle 0 HP state by disabling player control/rendering while scene updates run (`PlayerState struct: int hp = 5, max_hp = 5; float iframe_timer = 0.0f; bool defeated = false;`).

---

### Batch 2: Dark Tower Infrastructure & Spawners

* `[EP-DT]`: Dark Tower Implementation Epic (WIP) - Spawning shadow towers that incubate pests, pulse dark energy, and drop alloy on destruction.
  * `[PH-CDS]`: Core Data Structures & Entity Definition - `WorldStructure.h` architecture (`StructureType::DarkTower`, 3x4 tile / 48x64px looming dark obsidian column visual with spired peak and pulsing violet core) and `ShadowEgg.h` incubator definition.
    * `[DTS]`: Dark Tower Data Struct (COMPLETED) - State tracking, 3x4 obsidian column visuals, spired peak, violet core pulse, hit flash, AABB ground collision, and scene initialization (`WorldStructure struct: float x, y; int hp = 8, max_hp = 8; float spawn_timer = 0.0f; float pulse_timer = 0.0f; bool active = true;`).
    * `[EGS]`: Shadow Egg State Struct (PENDING SPECIFICATION) - Data tracking for incubating pest pods (`ShadowEgg struct: float x, y; float incubation_timer = 5.0f; int hp = 1; bool hatched; bool destroyed;`). Note: Visual appearance, game design behavior, collision detection (likely similar to `Enemy.h`), and juice (e.g. egg shaking / wobble micro-animations) are to be determined with the user via a `/grill-me` session.
  * `[PH-SLR]`: Spawner Logic & Lifecycle Replacement - Shifting enemy spawns from map edges to active towers.
    * `[SRS]`: Dark Tower Enemy Spawner Routing - Re-route world enemy spawn queries directly to active `DarkTower` coordinates.
    * `[ECH]`: Egg-to-Chrysalis Hatching Logic - Incubate and drop local `ShadowEgg` nodes (3D layered twilight ovals with top highlight) on adjacent tiles that hatch directly into `Enemy.cpp` after 5.0s.
    * `[TGP]`: Tower Global Pulse Emission - Periodic visual pulse ring every 10s that increments room Twilight (+0.02) and sets up corruption mechanics.
  * `[PH-CID]`: Combat Integration & Destruction Loop - Player offensive interaction with Dark Towers.
    * `[HBI]`: Swept Attack Damage Integration - Allow player melee swipe hurtbox to deal damage to active `DarkTower` entities and `ShadowEgg` pods.
    * `[DLS]`: Destruction Loot Scatter - Trigger tile-scattered Cursed Alloy scrap drops when a Dark Tower's HP hits zero.

---

### Batch 2.5: Architecture Refactoring (EnemyManager Header/Source Separation)

* `[EP-REFC]`: Architecture Refactoring Epic - Decoupling inline class implementation into dedicated `.cpp` source files for build speed and C++ best practices.
  * `[PH-REFC-EM]`: `EnemyManager` Source Separation Phase - Extract inline implementation logic from `EnemyManager.h` into a dedicated `EnemyManager.cpp` file, preserving raw headers for declaration types.

---

### Batch 3: Mana Spark Projectiles & Game Juice

* `[EP-PRJT]`: Directional Mana Spark Rites Epic - Charge-based ranged attack capability.
  * `[PH-PRJT]`: Ranged Projectile Phase - Hold attack button to launch concentrated light projectiles.
    * `[HCT]`: Hold Charge Timer - Add 0.5s hold charge check to `Action::Attack`.
    * `[DMSP]`: Directional Mana Spark Projectile - Spawn 4x4 projectile moving in player facing direction on charge release (`ManaSpark struct: float x, y, vx, vy; float lifetime = 2.0f; int damage = 2;`).
    * `[STFH]`: Tall Solid Fixture Helper - Helper to query tall structures (`Refiner`, `Spire`) that block projectiles.
    * `[PMCL]`: Projectile Motion & Collision Loop - Step projectile motion and handle impact against enemies (damage) vs walls/tall fixtures (absorption).

* `[EP-JUIC]`: Visual Polish & Game Juice Epic - Combat feedback and resource cleanup polish.
  * `[PH-JUIC]`: Game Feel Polish Phase - Micro-animations, hit flashes, and pickup mechanics.
    * `[HITF]`: Hit Flash Effects - 0.1s flash visual state on hit for enemies and player i-frames.
    * `[AMAG]`: Alloy Magnet Attraction - 1-tile magnetic pull pulling nearby Alloy toward the player.
    * `[ADSP]`: Alloy Despawn Warning - 30s lifetime for uncollected Alloy with 10s flashing warning (`AlloyDrop struct: float x, y; float lifetime = 30.0f; bool flashing;`).
    * `[DMPC]`: Dark Mana Spill Cloud - Fading purple smoke/cloud particle FX over broken pipe spills.

---

### Batch 4: Level Progression & Dynamic Twilight Equilibrium

* `[EP-LVLS]`: Level Progression & Room Navigation Epic - Three distinct stages forming the core game progression loop.
  * `[PH-LVLS]`: Three-Act Level Progression Phase - Progression from basement tutorial to open spire climax.
    * `[LV-CELL]`: Level 1: Damp Cellar Crypt - Compact tutorial room (16x16 grid) introducing movement, seeps, and pipe routing (`CellarLevel struct: int width = 16, height = 16; int target_light = 100;`).
    * `[LV-HALL]`: Level 2: Ruined Gothic Hall - Medium room (24x24 grid) featuring obstacles, multiple Dark Towers, and dark mana spills (`HallLevel struct: int width = 24, height = 24; int tower_count = 2;`).
    * `[LV-SPIR]`: Level 3: Open-Air Spire Rooftop - Climax room (32x32 grid) with multi-node networks, heavy pest waves, and elevator/beacon activation (`SpireLevel struct: int width = 32, height = 32; bool beacon_active;`).

* `[EP-EQUI]`: Purification Enrage & Dynamic Rubber-Band Equilibrium Epic - Difficulty balancing linked to room Twilight.
  * `[PEPB]`: Twilight Dynamic Equilibrium - As room Twilight drops toward 0%, enemies become enraged (shorter wander, larger aggro). If pipes break and Twilight spikes, enemies revert to passive wandering so players can repair without immediate death-spirals.

---

### Batch 5: Menus, Tracker Audio & Release Audit

* `[EP-UI]`: Menus, Dialogues & Audio Integration Epic - Complete game shell, audio driver hookup, and build footprint constraint.
  * `[PH-UI]`: Presentation & Audio Phase - Polish shell, menus, tracker music, and release size checks.
    * `[TITL]`: Main Title Screen & Map Setup - "Aetherlux" title screen with start, options, and input remapping (`TitleScreen struct: int selected_index = 0; bool active = true;`).
    * `[PAUS]`: In-Game Pause Menu - Pause overlay providing restart, audio toggles, and quit options (`PauseMenu struct: bool active = false; int selected_item = 0;`).
    * `[DIAL]`: Narrative & Tutorial Dialogue Popups - Text dialogue boxes for lore beats and pipe routing hints (`DialogueBox struct: const char* text; float timer; bool active;`).
    * `[MUS]`: Tracker Music Integration - `pocketmod` tracker background music playback for each stage (`MusicTrack struct: const char* mod_path; bool playing;`).
    * `[SND]`: Sound Effects Integration - `miniaudio` SFX for pipe placement, player swipe, projectile spark, damage, and purification.
    * `[SIZE]`: Floppy Disk Size Budget Audit - Optimization checks guaranteeing release binary fits under 1.44Mb.
