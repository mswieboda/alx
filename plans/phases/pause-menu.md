# [EP-PAUS]: Pause Menu Overlay & Unified Fullscreen Overlays

**Status**: Ready for Implementation
**Estimated Timeline**: 0.5 Day
**Related Epic**: `[PAUS]` in [plans/game_jam_priority_analysis.md](file:///Users/matt/code/cpp/alx/plans/game_jam_priority_analysis.md)

---

## 1. System Overview & Architecture

```mermaid
graph TD
    A[Action::Menu (Start) Pressed] -->|Toggle| B{m_paused?}
    B -->|Enter Pause| C[Pause Music & Freeze Sim]
    C --> D[Reset Menu Selection to Resume]
    D --> E[Render Dimmed Backdrop + Title + Menu]

    E --> F{Input Handling}
    F -->|Action:MoveUp/Down (D-Pad Up/Down| G[Update Menu Selection & SFX]
    F -->|Action::Cancel (B) or Action::Menu (Start)| H[Unpause & Resume Music]
    F -->|Action::ActionBtn (A) Confirm| I{Selected Item}

    I -->|Resume| H
    I -->|Retry| J[Reload MainScene with Current Level ID]
    I -->|Main Menu| K[Change Scene to StartScene]

    B -->|Already Paused| H
```

---

## 2. Shared Fullscreen Overlay Unification

Currently, `HUD.cpp` contains duplicate drawing logic for Game Over and Victory screens. We unify them under a single, reusable `draw_overlay_menu` helper:

```
+-------------------------------------------------------+
|              Semi-Transparent Dark Backdrop           |
|                                                       |
|                     * TITLE *                         |
|                                                       |
|                    > Option 1 <                       |
|                      Option 2                         |
|                      Option 3                         |
|                                                       |
+-------------------------------------------------------+
```

---

## 3. Phased Task Breakdown

### [PH-UOVR]: Phase 1 - Fullscreen Overlay View Unification (COMPLETED)
- [x] `[OVRM]`: Unified Overlay Menu Renderer — Create a shared `draw_overlay_menu` function in `HUD.h`/`HUD.cpp` (or `OverlayConfig`) that renders:
  - Full-screen dimmed backdrop rect (`0xAA000000` / `fade_color` at `Layer::HUD_Overlay`).
  - Centered large header title with black drop shadow at `Layer::HUD_OverlayText`.
  - Centered [`Menu`](file:///Users/matt/code/cpp/alx/src/alx/Menu.h) instance using consistent colors (`TextStyles::color`, `TextStyles::color_shadow`) and spacing.
- [x] `[ROVR]`: Refactor Game Over & Victory Renderers — Update `draw_game_over_menu` and `draw_victory_menu` to leverage the unified overlay menu helper.

### [PH-PAUS]: Phase 2 - Pause Menu Items & Actions
- [ ] `[PMOD]`: Pause Menu Item Definition — Define `PauseMenuItem` enum and array in [`MainScene.h`](file:///Users/matt/code/cpp/alx/src/alx/MainScene.h):
  ```cpp
  enum class PauseMenuItem : uint8_t { Resume, Retry, MainMenu, Count };
  ```
  Items array: `{"Resume", "Retry", "Main Menu"}`.
- [ ] `[PACT]`: Pause Menu Action Execution — In [`MainScene.cpp`](file:///Users/matt/code/cpp/alx/src/alx/MainScene.cpp), handle confirmed item choices:
  - **Resume**: Set `m_paused = false`, call `Audio::resume_music()`.
  - **Retry**: Switch scene to a fresh `alx::MainScene(m_current_level_id)`.
  - **Main Menu**: Switch scene to `alx::StartScene()`.

### [PH-PCTR]: Phase 3 - GBA Controller Action Controls & UX Flow
- [ ] `[PKEY]`: GBA-Style Action Navigation & Quick-Resume:
  - `Action::Menu` (Start / Enter) toggles pause on and off.
  - While paused, `Action::Cancel` (Button B / K / X) and `Action::Menu` (Start) immediately unpause.
  - `Action::ActionBtn` (Button A / J / Z) confirms the highlighted menu item.
  - `Action::MoveUp` / `MoveDown` navigates the menu with existing audio feedback.
- [ ] `[PRST]`: Cursor Reset on Open — Reset `m_pause_menu.set_selected_item(PauseMenuItem::Resume)` every time the pause menu is opened so the player always starts on "Resume".

### [PH-SIMA]: Phase 4 - Audio & Simulation Hygiene
- [ ] `[AUDP]`: Background Music & Audio Hygiene — Call `Audio::pause_music()` upon pausing and `Audio::resume_music()` upon unpausing. Navigation SFX remain active.
- [ ] `[SIMH]`: Clean Simulation Tick Freeze — Ensure all world state updates (player, camera, enemy manager, particles, telemetry, twilight metrics) are fully frozen during pause while HUD and overlay continue to render.
