
### Batch: Level Progression & Dynamic Twilight Equilibrium

* `[EP-LVLS]`: Level Progression & Room Navigation Epic - Three distinct stages forming the core game progression loop.
  * `[PH-LVLS]`: Three-Act Level Progression Phase - Progression from basement tutorial to open spire climax.
    * `[LV-CELL]`: Level 1: Damp Cellar Crypt - Compact tutorial room (16x16 grid) introducing movement, seeps, and pipe routing (`CellarLevel struct: int width = 16, height = 16; int target_light = 100;`).
    * `[LV-HALL]`: Level 2: Ruined Gothic Hall - Medium room (24x24 grid) featuring obstacles, multiple Dark Towers, and dark mana spills (`HallLevel struct: int width = 24, height = 24; int tower_count = 2;`).
    * `[LV-SPIR]`: Level 3: Open-Air Spire Rooftop - Climax room (32x32 grid) with multi-node networks, heavy pest waves, and elevator/beacon activation (`SpireLevel struct: int width = 32, height = 32; bool beacon_active;`).

* `[EP-EQUI]`: Purification Enrage & Dynamic Rubber-Band Equilibrium Epic - Difficulty balancing linked to room Twilight.
  * `[PEPB]`: Twilight Dynamic Equilibrium - As room Twilight drops toward 0%, enemies become enraged (shorter wander, larger aggro). If pipes break and Twilight spikes, enemies revert to passive wandering so players can repair without immediate death-spirals.

---

### Batch: Menus, Tracker Audio & Release Audit

* `[EP-UI]`: Menus, Dialogues & Audio Integration Epic - Complete game shell, audio driver hookup, and build footprint constraint.
  * `[PH-UI]`: Presentation & Audio Phase - Polish shell, menus, tracker music, and release size checks.
    * `[TITL]`: Main Title Screen & Map Setup - "Aetherlux" title screen with start, options, and input remapping (`TitleScreen struct: int selected_index = 0; bool active = true;`).
    * `[PAUS]`: In-Game Pause Menu - Pause overlay providing restart, audio toggles, and quit options (`PauseMenu struct: bool active = false; int selected_item = 0;`).
    * `[DIAL]`: Narrative & Tutorial Dialogue Popups - Text dialogue boxes for lore beats and pipe routing hints (`DialogueBox struct: const char* text; float timer; bool active;`).
    * `[MUS]`: Tracker Music Integration - `pocketmod` tracker background music playback for each stage (`MusicTrack struct: const char* mod_path; bool playing;`).
    * `[SND]`: Sound Effects Integration (COMPLETED) - `miniaudio` SFX for pipe placement, player swipe, projectile spark, damage, and purification.
    * `[SIZE]`: Floppy Disk Size Budget Audit - Optimization checks guaranteeing release binary fits under 1.44Mb.
