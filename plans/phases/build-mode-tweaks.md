# [EP-BMMS]: Build Mode Ergonomics, Foundation System & Line Drag Plan

This document outlines the phased design and implementation plan for *Aetherlux* (`alx`) building mechanics, incorporating cardinal directional locks, axis hysteresis, held fixture visual feedback, continuous line dragging, and an Age of Empires style `FixtureFoundation` construction system.

---

## Game Design & Input Integration

### 1. Button Roles & Unconditional Combat Defense (`[CBMD]`)
To eliminate accidental builds during combat while ensuring players are never defenseless:
* **`South` (`A` Button / `J`/`Z` Keys)**: Primary Sword Attack. Operates unconditionally—even while holding `R1` (`BuildMode`) or `West` (`Place`)—ensuring instantaneous defense against ambushes.
* **`Right Trigger` (`RT` / `P` Key)**: Mana Spark Charge/Fire. Operates unconditionally—even while holding `R1` (`BuildMode`).
* **`West` (`X` Button / `U` Key)**: Dedicated `Place` Fixture action (`Action::Place`). Holding `X` (or `U`) initiates direct placement **and** activates `[CDLK]` (Cardinal Placement Lock) & `[CPLD]` (Continuous Line Dragging).
* **`East` (`B` Button / `K`/`X` Keys)**: Dedicated Demolish / Remove Fixture (when holding `BuildMode` modifier `R1` / `O`).
* **`North` (`Y` Button / `I` Key)**: Dedicated Cycle Build Type (Pipe $\rightarrow$ Wall $\rightarrow$ Refiner $\rightarrow$ Spire).
* **`Left Bumper` (`L1` / `Q` Key)**: Dedicated Pan Mode (Camera scouting).
* **Future `Build` Action (`Action::BuildFoundation`)**: Construct / repair an adjacent `FixtureFoundation` to 100% completion.

### 2. Age of Empires Style `FixtureFoundation` Design (`[FXFD]`)
Rather than a zero-cost ghost blueprint, placing a fixture consumes Alloy immediately upfront and drops a 0%-complete `FixtureFoundation` (e.g. `Foundation::Pipe`, `Foundation::Refiner`, `Foundation::Spire`, `Foundation::Wall`).
* **Why this is Great Game Design for `alx`**:
  - **Resource Commitment**: Spending Alloy upfront prevents infinite barricade spamming.
  - **RTS Construction Feeling**: Watching foundations transform into glowing operational fixtures provides tangible progression.
  - **Tactical Vulnerability**: Enemies can attack unconstructed foundations, creating tense "defend the construction site" wave moments.
  - **Lore Fit**: Physical Alloy foundation is laid down first, then energized by stable Light Mana flow!

---

## Phased Implementation Plan

### [PH-CDLK]: Phase 1 - Unconditional Combat & Cardinal Line Controls (COMPLETED)

Implement 4-way cardinal locking when holding the dedicated placement button (`Place` / `X` / `U`) and ensure combat actions (`ActionBtn` / `A`, `ManaSpark` / `RT`) operate unconditionally.

- [x] `[CBMD]`: Concurrent Build-Combat Defense - Permit primary attacks (`A`) and Mana Spark charges (`RT`) to trigger unconditionally even while `R1` (`BuildMode`) or `Place` (`X` / `U`) is held down; sword attacks pause fixture placement for their 0.15s sweep duration.
- [x] `[STCE]`: Strict 4-Way Cardinal Enforcer - Restrict build target placement vectors strictly to 4-way orthogonal cardinal axes ($0^\circ, 90^\circ, 180^\circ, 270^\circ$) whenever placement is active.
- [x] `[CDLK]`: Cardinal Placement Lock - Lock facing direction exclusively when actively holding `Place` (`X` / `U`), while preserving full 8-way player movement speed during strafe. Holding `R1` (`BuildMode`) alone retains normal 8-way player turning.
- [x] `[STUI]`: Direction Lock Visual Indicator - Display a crisp 2D directional reticle arrow locked along the active cardinal axis and a grid tile outline over the targeted tile position.
- [x] `[BLFD]`: Foundation Construction Action - Support `Action::BuildFoundation` ("build_foundation" / "build") to trigger manual foundation construction/repair when standing adjacent to a `FixtureFoundation`.

---

### [PH-AXHF]: Phase 2 - Axis Hysteresis & Hemisphere Flipping (COMPLETED)

Implement intelligent direction switching so the player can change build directions without releasing the build button.

- [x] `[AXHF]`: Axis Hysteresis Buffer - Maintain the current cardinal lock when moving within a $\pm 45^\circ$ forward cone (allowing diagonal walking while dragging a straight line).
- [x] `[OHFL]`: Opposite Hemisphere Flip Lock - Automatically flip the locked cardinal direction to its exact polar opposite (e.g., North $\rightarrow$ South) when input moves into the rear $90^\circ$ cone.
- [x] `[CRAT]`: Cross-Axis Threshold Turn - Rotate facing by $90^\circ$ (e.g., North $\rightarrow$ East) only when input crosses a hard $60^\circ$ sideways deflection threshold.

---

### [PH-CPLD]: Phase 3 - Continuous Pipe Line Dragging

Enable holding the build button to place continuous lines of pipes and walls as the player moves.

- [ ] `[CPLD]`: Continuous Pipe Line Drag - Automatically place a new tile of the selected fixture type whenever the player moves into an adjacent empty grid cell while holding `X` (West) or `U`.
- [ ] `[RSCK]`: Resource Shortage Gate - Gracefully stop drag placement and play a subtle audio cue (`SFX::wall_bump` or resource error tone) when Alloy runs out.
- [ ] `[DRSFX]`: Drag Placement Audio/Visual FX - Trigger light particle sparks and building placement SFX for each auto-placed segment along the drag path.

---

### [PH-HICN]: Phase 4 - Held Fixture Visual Feedback & Shared HUD Glyphs

Render visual indication of the equipped fixture directly over the player's hands when holding `R1`.

- [ ] `[HUDG]`: Shared HUD Glyph Helper - Extract/decouple HUD text glyph rendering helpers (near `Fixture.h`) so fixture icons can be drawn anywhere in world space or UI space.
- [ ] `[HICN]`: Held Fixture Hand Icon - Render the tiny HUD text glyph of the active fixture (Pipe, Wall, Refiner, Spire) near the player's hands whenever `R1` (Build Mode) is held.
  > [!NOTE]
  > The implementer MUST ask for user feedback to refine the exact pixel offsets relative to the player sprite for each facing direction (N/NE/E/SE/S/SW/W/NW). Do NOT attempt to read offset data from Aseprite asset files or headers.

---

### [PH-BLPF]: Phase 5 - Fixture Foundation Construction System

Transition direct building to an RTS-style foundation placement and energy construction pipeline.

- [ ] `[FXFD]`: Fixture Foundation Placement - Modify build commands so placing a structure consumes Alloy upfront and spawns a `FixtureFoundation` (`Foundation::Pipe`, `Foundation::Wall`, `Foundation::Refiner`, `Foundation::Spire`).
- [ ] `[FCON]`: Foundation Energy Construction - Automatically construct and energize a `FixtureFoundation` into an operational structure when Light Mana reaches it via the pipe network or when the player discharges a Mana Spark near it.
- [ ] `[FDHP]`: Low-Priority Foundation Targeting & Health - Give unbuilt foundations a reduced health pool. Assign a drastically lowered enemy target score so enemies prioritize players, active refiners, and spires first, only attacking foundations as a last resort when everything else is destroyed.
