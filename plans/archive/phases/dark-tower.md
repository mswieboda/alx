# Dark Tower Implementation & Pipe Flow Roadmap

## [EP-DT]: Dark Tower Implementation Epic

### [PH-CDS]: Phase Core Data Structures & Entity Definition (COMPLETED)

* `[DTS]`: Dark Tower Struct (COMPLETED) - data for the initial dark tower entity, visuals (3x4 obsidian monolith, spired peak, violet core pulse, hit flash), AABB ground collision, and scene placement - `DarkTower struct: float x, y; int hp = 8; float shield_hp = 5.0f; float spawn_timer = 0.0f; float pulse_timer = 0.0f; bool active = true;`
* `[EGS]`: Egg/Pest Spawner State Struct (COMPLETED) - data for tracking incubating shadow spawn pods on adjacent grid tiles - `ShadowEgg struct: float x, y; float incubation_timer; bool hatched;`. Note: Visual appearance, game design behavior, collision detection (likely similar to `Enemy.h`), and juice (e.g. egg shaking / wobble micro-animations) are to be determined with the user via a `/grill-me` session.

---

### [PH-SLR]: Phase Spawner Logic & Lifecycle Replacement (COMPLETED)

* `[SRS]`: Spawner Replacement Strategy - routing world enemy generation queries away from random edge-of-map boundaries and binding them directly to active `DarkTower` coordinate instances
* `[ECH]`: Egg-to-Chrysalis Hatching - incremental timer update logic where a `DarkTower` incubates and drops local `ShadowEgg` nodes on nearby empty grid tiles before they mature into active shadow pests targeting network pipes
* `[TGP]`: Tower Global Pulse - periodic emission of a visual pulse/shockwave ring every $N$ seconds that updates the room's global twilight count and readies future fixture-corruption mechanics

---

### [PH-CID]: Phase Combat Integration & Destruction Loop (COMPLETED)

* `[HBI]`: Hurtbox & Swept Attack Integration - leveraging existing player circular hurtbox and swept-arc attack mechanics to inflict direct damage to active `DarkTower` entities
* `[DLS]`: Destruction Loot Scatter - logic triggered when a dark tower's HP reaches zero, causing it to shatter and permanently scatter a handful of Cursed Alloy scrap resources across surrounding tiles

---

## [EP-PFRR]: Pipe Flow & Dead-End Round-Robin Architecture (COMPLETED)

### [PH-EXP]: Optional Experiments
* `[PSTR]`: Dark Mana Pulse Mode Experiment - optional experiment for Dark Mana stream generation to alternate between connected output ports in round-robin fashion, producing discrete 1-tile stream segments with gaps
