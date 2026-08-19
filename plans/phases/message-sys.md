# [EP-CTXP]: Contextual Text Prompts & Toast Overlay System Plan

This document outlines the phased roadmap for implementing the **Contextual Text Prompts & Toast Overlay System** (`[EP-CTXP]`) in *Aetherlux* (`alx`). It provides a lightweight, non-intrusive banner overlay for player guidance, resource warnings, and threat alerts without interrupting movement, combat, or automation flow.

---

## Architecture & Data Structures Overview

```cpp
namespace alx::ui::prompt {

// Severity tiers defining visual borders and preemption priority
enum class PromptSeverity : uint8_t {
    info_tutorial = 0, // Tier 0: Guidance, discovery, first-time control hints
    resource_warning,  // Tier 1: Low alloy, unlinked pipes, stalled flow
    critical_threat    // Tier 2: Spire attacked, Twilight surge incoming
};

// Internal animation lifecycle state machine
enum class PromptState : uint8_t {
    inactive = 0,
    fade_in,
    active_hold,
    fade_out
};

// Unique identifiers for game triggers, once-only tracking, and action dismissal
enum class PromptId : uint16_t {
    none = 0,
    mine_alloy_hint,
    place_pipe_hint,
    link_spire_hint,
    low_alloy_warning,
    pipe_unlinked_warning,
    spire_attacked_alert,
    surge_incoming_alert
};

// Lightweight, zero-heap prompt definition
struct PromptMessage {
    std::string_view text_content{};
    PromptId id{PromptId::none};
    PromptSeverity severity{PromptSeverity::info_tutorial};
    float hold_duration_sec{3.0f};
    bool dismiss_on_action{true};
};

// Active overlay state controller
struct PromptController {
    PromptMessage current_prompt{};
    PromptMessage pending_prompt{};
    PromptState state{PromptState::inactive};
    float state_timer_sec{0.0f};
    float alpha{0.0f};
    float slide_offset_y{0.0f};
    bool has_pending{false};
};

// Layout & visual styling constants
inline constexpr int box_height_px = 16; // 4px top padding + 8px font + 4px bottom padding
inline constexpr float corner_radius = 3.0f;
inline constexpr int border_thickness_px = 1;
inline constexpr int padding_x_px = 8;
inline constexpr int margin_left_px = 8;
inline constexpr int margin_bottom_px = 8;
inline constexpr float slide_distance_px = 4.0f;
inline constexpr float dismiss_slide_distance_px = 2.0f;

inline constexpr float fade_in_duration_sec = 0.15f;
inline constexpr float fade_out_duration_sec = 0.20f;
inline constexpr float default_hold_duration_sec = 3.00f;
inline constexpr float prompt_repeat_cooldown_sec = 10.0f;

// Color Palette Constants (Strictly no amber/gold)
inline constexpr uint32_t prompt_bg_color_rgba = 0xD80F131D;          // Translucent dark slate backing
inline constexpr uint32_t prompt_border_tier0_rgba = 0xFFA8C0D8;      // Resonant silver / slate
inline constexpr uint32_t prompt_border_tier1_elav_rgba = 0xFF9D68EE; // Electric lavender (default)
inline constexpr uint32_t prompt_border_tier1_hvtl_rgba = 0xFF38E2D8; // High-voltage teal (alternative)
inline constexpr uint32_t prompt_border_tier2_rgba = 0xFFD82850;      // Crimson quartz

} // namespace alx::ui::prompt
```

---

## Phased Implementation Plan

### `[PH-CTXP-CORE]`: Phase 1 - Core Toast Overlay & Minimal Renderer (COMPLETED)
*Focus: Rounded-rect Bottom-Left Toast Pill (`[BLTP]`) rendering, alpha transitions, and micro-slide animation.*

- [x] `[BLTP-RND]`: **Rounded-Rect Banner Render Pass**
  - Implement rendering pipeline in UI drawing routines for a 16px-tall rounded-rect toast pill (`corner_radius = 3.0f`).
  - Calculate dynamic width based on `text_pixel_width + 16px` padding.
  - Position banner anchored to the bottom-left with an 8px screen edge margin.
- [x] `[BLTP-ANI]`: **Alpha Fade & Micro-Slide Animation**
  - Implement `PromptState` transition logic (`inactive` $\rightarrow$ `fade_in` $\rightarrow$ `active_hold` $\rightarrow$ `fade_out`).
  - Animate alpha (`0.0f` to `1.0f`) and vertical slide displacement (4px upward glide on entrance).
- [x] `[BLTP-PAL]`: **3-Tier Theme Palette Integration**
  - Apply distinct border styling for Tier 0 (Resonant Silver), Tier 1 (Electric Lavender `[ELAV]`), and Tier 2 (Crimson Quartz).
  - Render crisp 1-pixel rounded borders and dark translucent backing (`0xD80F131D`).

---

### `[PH-CTXP-QUEUE]`: Phase 2 - Lightweight Toast Queue Engine (`[LTQE]`)
*Focus: Zero-allocation queue manager, priority preemption, once-only tracking, and action-fulfillment dismissal.*

- [ ] `[LTQE-MGR]`: **Dual-Slot Queue Controller & Preemption**
  - Implement `PromptController` maintaining 4-slot ring buffer.
  - Support priority preemption: incoming Tier 2 (Threat) immediately interrupts active Tier 0/1 prompts with an accelerated fade-out / immediate entrance.
- [ ] `[LTQE-DISM]`: **Action-Fulfillment Instant Dismissal**
  - Implement `dismiss_if_matching(PromptId id)` to transition state instantly to `fade_out` when the player executes the prompted action.
- [ ] `[LTQE-COOL]`: **Cooldowns & Once-Only History Tracking**
  - Implement bitset / lookup table for `PromptId` to track seen-once tutorial hints across level sessions.
  - Enforce a 10s cooldown timer between repeated operational warnings.

---

### `[PH-CTXP-TRIG]`: Phase 3 - Collaborative Trigger Brainstorming & Systemic Game Hooks (`[TPMV]`)
*Focus: Brainstorming comprehensive trigger points with the user, curating concise vocabulary, and hooking triggers into gameplay systems.*

- [ ] `[TRIG-BRAIN]`: **Collaborative Trigger & Message Brainstorming**
  - Brainstorm and align with the user on all specific message trigger moments, conditions, and text strings.
  - Scope conditions across:
    - Early-game onboarding / controls (mining alloy, placing pipes, camera scouting).
    - Network automation & logistics (connecting refiners to spires, unlinked pipe flow warnings, mana pressure).
    - Economy & resources (insufficient alloy, full mana capacity).
    - Twilight threats & combat (Dark Tower emergence alerts, Spire damage alerts, Twilight surge waves, low HP warnings).
  - Define priority tiers, repeat cooldowns, and auto-dismissal actions for each brainstormed trigger.
- [ ] `[TRIG-REG]`: **Systemic Trigger Dispatch Implementation**
  - Hook the curated trigger list into gameplay systems (`Player.cpp`, `Network.cpp`, `EnemyManager.cpp`, `MainScene.cpp`).
  - Wire action-fulfillment events (`dismiss_if_matching`) on player verb execution.

---

### `[PH-CTXP-JUICE]`: Phase 4 - Audio Blip, Border Shimmer & Input Action Mapping (`[PJUICE]`)
*Focus: Retro sound cues, border pulse animations, and extensible action label hooks.*

- [ ] `[SND-BLIP]`: **Subtle Retro Blip Audio Cue**
  - Play a short, crisp chiptune blip when Tier 0/1 prompts appear.
  - Play an urgent alert tone when Tier 2 threats trigger.
- [ ] `[VFX-SHIM]`: **Warning & Threat Border Shimmer**
  - Apply subtle sine-wave palette oscillation on Electric Lavender and Crimson Quartz borders during active hold.
- [ ] `[INP-MAP]`: **Extensible Action Label Tokens**
  - Structure prompt text formatters to query action mappings cleanly from `Action.h` / input systems, ensuring future keyboard vs gamepad button label swap readiness.
