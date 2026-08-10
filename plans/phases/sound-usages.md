# [PH-SUAG]: Sound Usages & Audio Architecture Plan

This document outlines the phased roadmap for integrating background music, sound effects (SFX), spatial 2D view-culled audio, custom SFXR parameters, and missing sound triggers into *Aetherlux* (`alx`).

---

## Architecture & Design Goals

1. **Low Memory & Tiny Binary Footprint**: Leverage existing `SfxrParams` procedural generation and embedded `pocketmod` tracker music without bloating binary assets.
2. **Viewport Culling & Spatial 2D Distance Volume**:
   - Play SFX only if the source position `(x, y)` is within or near the active camera viewport bounds.
   - Scale SFX gain/volume based on distance to player: \( \text{volume} = \text{base\_volume} \times \text{clamp}(1.0 - \frac{\text{distance}}{\text{max\_radius}}, \text{min\_attenuation}, 1.0) \).
3. **Background Music Management**:
   - Play background tracker music quietly on loop (\(\approx 0.15 - 0.25\) volume).
   - Auto-pause track when game enter pause state, resume on unpause.
4. **Structured Sound Presets**: Define dedicated `SoundLibrary` static helpers/presets to encapsulate `SfxrParams` configuration for each event type.

---

## Phase Breakdown

### `[PH-SUAG-P1]`: Background Music Integration & Audio Subsystem Enhancements (COMPLETED)
- [x] `[SUAG-MSCV]`: Configure default background music volume control (\(\sim 0.20\)) and ensure clean loop initialization upon game scene start.
- [x] `[SUAG-MSPS]`: Wire `Audio::pause_music()` and `Audio::resume_music()` to Game State pause/unpause toggles.
- [x] `[SUAG-VOLS]`: Enhance `Audio::play_sfx` or introduce `Audio::play_sfx_at(params, volume, pan)` to support per-instance playback volume and stereo panning.

### `[PH-SUAG-P2]`: Sound FX Library & Preset Definitions (`SfxrParams`) (COMPLETED)
Define procedural `SfxrParams` presets for all core game interactions:

- [x] `[SUAG-SFX-SWP]`: **Player Sword Swipe** (Noise / Sawtooth pitch sweep, short attack, fast decay).
- [x] `[SUAG-SFX-ENH]`: **Enemy Hit** (Low square/noise drop, quick punchy decay).
- [x] `[SUAG-SFX-PLH]`: **Player Hit** (Sharp high square burst to low drop, distinct warning tint).
- [x] `[SUAG-SFX-DTH]`: **Dark Tower Hit** (Heavy resonant square rumble, pitch slide down).
- [x] `[SUAG-SFX-FXH]`: **Fixture Hit** (Crisp metallic/wooden square click).
- [x] `[SUAG-SFX-TWP]`: **Dark Tower Twilight Pulse** (Deep pulsating sine wave sweep with slow vibrato).
- [x] `[SUAG-SFX-EGH]`: **Egg Hatch** (Rising high-pitch square slide ending in noise pop).
- [x] `[SUAG-SFX-DTS]`: **Dark Tower Spawn** (Ominous low-frequency noise & square slide).
- [x] `[SUAG-SFX-RFB]`: **Refining Bubble** (Subtle soft high sine/square pitch blip while refining).
- [x] `[SUAG-SFX-SPC]`: **Spire Cackle/Burn** (Intermittent crackling noise bursts during energy discharge).
- [x] `[SUAG-SFX-PLD]`: **Player Death** (Multi-tone descending square sweep into static decay).
- [x] `[SUAG-SFX-BMP]`: **Player Solid Collision / Bump** (Thudding low sine/square thump).
- [x] `[SUAG-SFX-SNP]`: **Player Build Snap / Grid Placement** (Crisp high square click / latch chirp).

### `[PH-SUAG-P3]`: Camera Viewport Culling & Simple 2D Spatial Audio
- [ ] `[SUAG-SPAT-CUL]`: Add `Camera`/`Viewport` intersection query before triggering world SFX (suppress sounds outside view rect).
- [ ] `[SUAG-SPAT-DIST]`: Implement distance-based volume attenuation:
  - Full volume within close range (\(0 - 96\) px).
  - Smooth linear attenuation up to max range (\(384\) px / camera viewport width).
  - Minimum audibility floor (\(0.15\)) if visible on screen so actions remain clear.
- [ ] `[SUAG-SPAT-PAN]`: (Optional/Nice-to-have) Basic stereo panning based on horizontal screen displacement relative to camera center.

### `[PH-SUAG-P4]`: Game World Event Triggers & Integration
- [ ] `[SUAG-TRIG-COMB]`: Wire combat sounds (sword swipe, enemy hit, player hit, player death) into combat & collision resolution logic.
- [ ] `[SUAG-TRIG-MNFA]`: Wire automation sounds (refiner bubbling, spire cackling, twilight pulse, egg hatch, dark tower spawn).
- [ ] `[SUAG-TRIG-BLD]`: Wire construction/interaction sounds (grid placement snap, wall bump).

---

## Brainstormed Missing Sounds (Game System Expansion)

The following additional game actions and feedback events were identified to round out audio feedback across exploration, crafting, UI, and world progression:

1. `[SUAG-XTRA-ALC]`: **Alloy Pick-up / Resource Collection** (Bright ascending 2-note chime when collecting Alloy or Mana shards).
2. `[SUAG-XTRA-PPC]`: **Pipe Connection / Mana Link Established** (Humming resonance chime when mana line connects source to spire).
3. `[SUAG-XTRA-UIM]`: **UI Menu Select / Cursor Move** (Minimal blip for inventory/pause menu navigation).
4. `[SUAG-XTRA-STC]`: **Structure Deconstruction / Dismantle** (Breakage sound when destroying pipes/refiners).
5. `[SUAG-XTRA-MAN]`: **Mana Depletion Warning / Low Alloy Chime** (Warning beep when attempting to build without enough Alloy).
6. `[SUAG-XTRA-PUR]`: **Room Purified / Twilight Cleared** (Triumphant multi-frequency ascending shimmer sweep when Twilight reaches 0% in a room).
7. `[SUAG-XTRA-SLD]`: **Door Unlock / Grid Room Transition** (Heavy stone slide or energetic portal hum upon entering a new room).
8. `[SUAG-XTRA-DSH]`: **Player Dash / Dodge Roll** (Whoosh noise envelope during rapid movement burst).
