# PLAN_ENEMY_INDICATORS.md - Off-Screen Threat Awareness & Camera Scouting

## Goal Description
Provide macro-awareness and tactical scouting for off-screen alloy enemies, dark mana seeps, and conduit infrastructure without re-introducing camera tracking jitter.

---

## Feature Specifications & Phased Roadmap

### Phase 1: Immediate Implementation

#### 1.1 Idea 2: Camera Edge Threat Chevrons
- **Behavior**: Scans world entities/enemies outside the current camera viewport bounds.
- **Rendering**: Projects off-screen enemy vectors onto the screen edge (4px margin inside viewport) and draws small glowing directional chevrons/arrows.
- **Visuals**:
  - Color: Dusky Magenta / Violet (matches Cursed Alloy / Dark Mana).
  - Proximity: Pulse frequency speeds up as the enemy gets closer to entering screen view.

#### 1.2 Idea 3: Hold-to-Pan Camera Scouting
- **Behavior**: Holding a designated key (e.g. `Left Shift` or `C`) enters **Pan Mode**.
- **Controls**: While holding Pan Mode, directional movement keys (`W/A/S/D` or Arrows) pan the camera target up to ±96px (6 tiles) in any direction without moving the player entity.
- **Release**: Releasing the key smoothly returns the camera framing to the player.

---

### Phase 2: Modular Enhancements

#### 2.1 Idea 6: Wand Light Perimeter Glints (Atmospheric Enhancement to Idea 2)
- **Behavior**: Instead of (or in addition to) screen-edge chevrons, project off-screen threat vectors onto the perimeter ring of the player's Wand of Twilight light radius (96px circle).
- **Rendering**: Draws small glowing light glints / notches on the outer edge of the light circle pointing toward off-screen alloy targets.
- **Toggle**: Configurable in settings or seamlessly combined with Idea 2.

#### 2.2 Idea 5: Full-Screen Tactical Map Overlay
- **Behavior**: Pressing a key (e.g. `Tab` or `M`) opens a full-screen semi-transparent overlay (320x240).
- **Rendering**:
  - Displays the 20x15 grid room bounds with 16px scaled cell outlines.
  - Draws Player blip (O), Cursed Alloy enemy markers (X), and conduit node icons (Refiners, Spires, Seeps).
  - Pauses or slows physics while reviewing the map.
