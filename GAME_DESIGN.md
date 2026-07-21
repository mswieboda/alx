# *Aetherlux* (`alx`) Overview

* **Project Name:** ***Aetherlux***
* **Code Namespace / Prefix:** `alx`
* **Genre/Style:** GBA-style top-down gothic/emo adventure meets micro-Factorio grid automation (*Kingdom Hearts 358/2 Days* meets *Golden Sun* and *Factorio*).
* **The Factions:** The **Solux Order** (dogmatic light hoarders) vs. the **Dark Syndicate** (nihilistic shadow-weavers).
* **Your Role:** **The Mystic** (formal title: **Mystic Adept**)—a self-taught, scrappy outsider routing bootleg pipes to survive.
* **The Goal:** Filter volatile dark twilight mana into stable light energy across compact indoor/outdoor rooms (Cellar -> Hall -> Spire Rooftop) before September 4th.

# Game Design Document (GDD) Artifact: *Aetherlux* (`alx`)

* **Code Name / Namespace:** `alx`
* **Target Jam:** 1.44Mb Floppy Disk Game Jam (Deadline: Sept 4th)
* **Engine/Tech:** Custom C++20 Software Renderer via `minifb`, RLE-compressed indexed Aseprite assets, sub-300Kb base boilerplate.
* **Protagonist:** **The Mystic** (Formal title: **Mystic Adept**)—a self-taught, scrappy underground alchemist operating entirely outside the institutional conflict.

---

### 1. The Premise & Factions

The world is caught in a permanent twilight. Two monolithic powers fight for total control over the planet’s lifeblood:

* **The Solux Order:** Dogmatic, blinding zealots of pure light who hoard energy and leave the fringes to rot.
* **The Dark Syndicate:** Nihilistic shadow-weavers seeking to plunge reality into silent, absolute dark.
* **The Mystic Adept:** You. Armed with bootleg conduit pipes and basic esoteric knowledge, you wander the ruins to filter raw, volatile twilight mana into stable, life-giving light energy just to keep your sanctuary from collapsing.

---

### 2. Core Gameplay Loop (GBA-Style Micro-Factorio / Adventure Hybrid)

* **Grid-Based Spatial Puzzling:** Each level is a compact, top-down room (starting in a damp cellar crypt, moving through a ruined gothic hall, and ending on an open-air rooftop spire).
* **The Taps & Conduits:** Find glowing "Twilight Seeps" (resource nodes). Lay down **Conduit Pipes** to route raw purple-black mana across the floor.
* **The Refiner Prism:** Feed raw mana into a **Prism Spire** to convert it from dark chaos into bright cyan/gold energy.
* **The Core Objective:** Route the purified light into the room's gateway beacon or elevator engine to unlock the next stage.
* **The Pressure (Light Action):** Periodic shadow-pests spawn from the dark corners to claw at your pipes and attack you. Use a quick-swipe mystic tool or blast spell to defend your infrastructure while you build.

---

### 3. Visual & Audio Style

* **Palette:** 256-color indexed palette. Deep midnight blues, bruised purples, slate grays, contrasted sharply with glowing neon cyan and amber.
* **Aesthetic:** GBA top-down perspective, chunky pixel art, moody *Kingdom Hearts 358/2 Days* solitude mixed with gothic architecture (wrought iron, cracked stone, sputtering candles).
* **Audio:** Melancholic, minimalist chiptune melodies.

---

### 4. Jam Milestones & Scope Control (Target: Sept 4th)

* **Phase 1 (Current):** Boilerplate engine, window rendering, delta time, input handling (sub-300Kb base).
* **Phase 2:** Grid tilemap rendering, Aseprite indexed sprite loader, player movement, and basic collision.
* **Phase 3:** Pipe-laying logic, item drop routing, and basic resource conversion states.
* **Phase 4:** Enemy spawner, combat interaction, and constructing 3 compact levels (Cellar -> Hall -> Spire Rooftop).
* **Phase 5:** Polishing UI text crawls, title screen ("Aetherlux"), and final size check to ensure it stays well under the 1.44Mb limit.
