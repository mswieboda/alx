# *Aetherlux* (`alx`) Game Design Document & Overview

* **Project Name:** ***Aetherlux***
* **Code Namespace / Prefix:** `alx`
* **Genre/Style:** GBA-style top-down gothic/emo adventure meets micro-Factorio grid automation (*Kingdom Hearts 358/2 Days* meets *Golden Sun* and *Factorio*).
* **The Factions:** The **Solux Order** (dogmatic light hoarders) vs. the **Dark Syndicate** (nihilistic shadow-weavers).
* **Your Role:** **The Mystic** (formal title: **Mystic Adept**)—a self-taught, scrappy outsider routing bootleg pipes to survive.
* **The Goal:** Filter volatile dark twilight mana into stable light energy across compact indoor/outdoor rooms (Cellar -> Hall -> Spire Rooftop) before September 4th.
* **Target Jam:** 1.44Mb Floppy Disk Game Jam (Deadline: Sept 4th)
* **Engine/Tech:** Custom C++20 Software Renderer via `minifb`, RLE-compressed indexed Aseprite assets, sub-300Kb base boilerplate.

---

### 1. The Premise & Factions

The world is caught in a permanent twilight. Two monolithic powers fight for total control over the planet’s lifeblood:

* **The Solux Order:** Dogmatic, blinding zealots of pure light who hoard energy and leave the fringes to rot.
* **The Dark Syndicate:** Nihilistic shadow-weavers seeking to plunge reality into silent, absolute dark.
* **The Mystic Adept:** You. Armed with bootleg conduit pipes and basic esoteric knowledge, you wander the ruins to filter raw, volatile twilight mana into stable, life-giving light energy just to keep your sanctuary from collapsing. Short name **Mystic**. —a self-taught, scrappy underground alchemist operating entirely outside the institutional conflict between The Factions.

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

#### Art Assets & Templates
* **[TDCP]**: Top-Down Character Proportions - Pixel templates based on Slynyrd's proportions (e.g. [Pixelblog 55](https://www.slynyrd.com/blog/2025/3/24/pixelblog-55-top-down-character-animation) for animation, [Pixelblog 56](https://www.slynyrd.com/blog/2025/5/23/pixelblog-56-top-down-character-attack-animation) for attacks).
* **[TSET]**: Tile Sheet Drafting - 16x16 pixel tile sheets containing gothic stone floors, walls, and conduit pipe/wireframe templates, potentially adapted from [Anokolisa's free assets](https://anokolisa.itch.io/free-pixel-art-asset-pack-topdown-tileset-rpg-16x16-sprites).

#### Audio & Sound Design
* **[MUS]**: Custom Tracker Music - Compose mood-fitting tracks based on pocketmod template `.mod` assets (e.g., `SongTrack struct: const char* file_path; int pattern_count;`).
* **[SND]**: Sound Effects - Add basic SFX for actions (pipe laying, attack/swipe, mana purifying, taking damage) (e.g., `SfxAsset struct: int sample_rate; float volume;`).

---

### 4. Level Structure & Progression

* **[LV-CELL]**: Level 1: The Damp Cellar Crypt - Tutorial layout designed to introduce basic player movement, twilight seeps, and pipe routing controls (e.g., `CellarLevel struct: int width = 16, height = 16; std::vector<Pipe> pipes;`).
* **[LV-HALL]**: Level 2: The Ruined Gothic Hall - A wider room introducing more complex spatial layouts, obstacle navigation, and expanded automation puzzles (e.g., `HallLevel struct: int width = 24, height = 24; std::vector<Obstacle> hazards;`).
* **[LV-SPIR]**: Level 3: The Open-Air Spire Rooftop - The climax room featuring advanced routing layouts and final gateway beacon activation (e.g., `SpireLevel struct: int width = 32, height = 32; bool beacon_active = false;`).

---

### 5. Polish, UI & Game Feel

* **[TITL]**: Title Screen & Controls - Dedicated main title screen displaying "Aetherlux" with a menu to start, exit, or configure gamepad button mapping (e.g., `TitleScreen struct: int selected_index = 0; bool configuring_gamepad = false;`).
* **[PAUS]**: Main Pause Menu - In-game menu enabling scene restarts, option configuration, and game exit (e.g., `PauseMenu struct: bool active = false; int selected_item = 0;`).
* **[DIAL]**: Message Dialogues - Clean text-box popups for tutorial guides, narrative beats, and game transitions (e.g., `DialogueBox struct: const char* text; float display_time; bool active;`).
* **[SIZE]**: Release Binary Footprint - Maintain strict optimization checks ensuring final release sizes remain within the 1.44Mb Floppy Disk limit.

---

### 6. Jam Milestones & Scope Control (Target: Sept 4th)

* **Phase 1 (done):** Boilerplate engine, window rendering, delta time, input handling (sub-300Kb base).
* **Phase 2 (done):** Grid floor tiles, Aseprite indexed sprite loader, player movement, and basic wall collision.
* **Phase 3 (done):** Pipe-laying logic, item drop routing, and basic resource conversion states.
* **Phase 4:** Enemy spawner, combat interaction (done), enemy attacking player (WIP - disabled or removed)
* **Phase 5:** Levels: 3 compact levels (Cellar -> Hall -> Spire Rooftop).
* **Phase 6:** Polishing UI text, adding juice, adding menus and title screen ("Aetherlux"), and final size check to ensure it stays well under the 1.44Mb limit.
