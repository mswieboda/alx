### **Plan Scratchpad: *Aetherlux* (`alx`)**

---

## `EP-DT`: Dark Tower Implementation Roadmap

### `PH-CDS`: Phase Core Data Structures & Entity Definition

* `DTS`: Dark Tower Struct - data for the initial dark tower entity - `DarkTower struct: float x, y, int hp = 8, float shield_hp = 5.0f, float spawn_timer = 0.0f, float pulse_timer = 0.0f, bool active = true`
* `EGS`: Egg/Pest Spawner State Struct - data for tracking incubating shadow spawn pods on adjacent grid tiles - `ShadowEgg struct: float x, float y, float incubation_timer, bool hatched`
* `FHS`: Fixture Health Visualization Extension - data/logic addition to render a tiny health bar or dynamic color tint shift toward red directly over damaged network fixtures and structures

.

### `PH-SLR`: Phase Spawner Logic & Lifecycle Replacement

* `SRS`: Spawner Replacement Strategy - routing world enemy generation queries away from random edge-of-map boundaries and binding them directly to active `DarkTower` coordinate instances
* `ECH`: Egg-to-Chrysalis Hatching - incremental timer update logic where a `DarkTower` incubates and drops local `ShadowEgg` nodes on nearby empty grid tiles before they mature into active shadow pests targeting network pipes
* `TGP`: Tower Global Pulse - periodic emission of a visual pulse/shockwave ring every $N$ seconds that updates the room's global twilight count and readies future fixture-corruption mechanics

.

### `PH-CID`: Phase Combat Integration & Destruction Loop

* `HBI`: Hurtbox & Swept Attack Integration - leveraging existing player circular hurtbox and swept-arc attack mechanics to inflict direct damage to active `DarkTower` entities
* `DLS`: Destruction Loot Scatter - logic triggered when a dark tower's HP reaches zero, causing it to shatter and permanently scatter a handful of Cursed Alloy scrap resources across surrounding tiles

.

### `PH-NIM`: Phase Network Interaction & Shield Mechanics

* `DTSF`: Dark Tower Shield Framework - implementation of regenerating dark energy force field armor on towers that absorbs incoming structural damage
* `SRD`: Spire Resonance Drain - calculation checking if active Light Spires fall within a dark tower's pulse radius, actively draining the tower's dark energy shield and slowing down its pest spawn rates

.

---

## `EP-PFRR`: Pipe Flow & Dead-End Round-Robin Architecture

### `PH-BDFL`: Broad Directional Flow & Dead-End Inclusion
* `SDFE`: Dead-End Distance Propagation / Source Distance Fallback - allow pipe flow to travel into dead-end pipe branches when no active SDF path exists or in round-robin sequence
* `RRMS`: Round-Robin Multi-Direction Selection - cycle through all valid connected pipe ports at T/X intersections regardless of SDF distance
* `NRR`: No-Reverse Rule - universal rule across all mana types preventing 180-degree backtracking into entry pipes (`dx == -move_dx && dy == -move_dy`)
* `DES`: Dead-End Stop & Park State - cause mana to gracefully stop in place (`move_dx = 0, move_dy = 0`) when reaching a dead end, filling the dead end until full

### `PH-EXP`: Optional Experiments
* `PSTR`: Dark Mana Pulse Mode Experiment - optional experiment for Dark Mana stream generation to alternate between connected output ports in round-robin fashion, producing discrete 1-tile stream segments with gaps

