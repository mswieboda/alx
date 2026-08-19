# [EP-CTXP]: Contextual Text Prompts & Toast Overlay System Plan

This document outlines the phased roadmap for implementing the **Contextual Text Prompts & Toast Overlay System** (`[EP-CTXP]`) in *Aetherlux* (`alx`). It provides a lightweight, non-intrusive banner overlay for player guidance, resource warnings, and threat alerts without interrupting movement, combat, or automation flow.

---

## Architecture & Data Structures Overview

```cpp
namespace alx::ui::prompt {

// Tiers defining visual borders and preemption priority
enum class PromptType : uint8_t {
    info = 0, // Tier 0: Guidance, discovery, first-time control hints
    warning,  // Tier 1: Low alloy, unlinked pipes, stalled flow
    alert    // Tier 2: Spire attacked, Twilight surge incoming
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
    PromptType type{PromptType::info};
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
- [x] `[BLTP-STK]`: **Sticky Prompt Support**
  - Add `is_sticky` support allowing prompts to remain active indefinitely until explicitly dismissed via player action fulfillment or `dismiss_if_matching(PromptId)`.

---

### `[PH-CTXP-QUEUE]`: Phase 2 - Lightweight Toast Queue Engine (`[LTQE]`) (COMPLETED)
*Focus: Zero-allocation queue manager, priority preemption, once-only tracking, and action-fulfillment dismissal.*

- [x] `[LTQE-MGR]`: **Dual-Slot Queue Controller & Accelerated Preemption**
  - Maintain a static 4-slot ring buffer for prompt sequencing.
  - Implement accelerated preemption: incoming Tier 2 (Threat) immediately interrupts active Tier 0/1 prompts, pushing the interrupted prompt back to the front of the queue, and enters active display instantly.
- [x] `[LTQE-DISM]`: **Action-Fulfillment Instant Dismissal**
  - Implement `dismiss_if_matching(PromptId id)` to transition state instantly to `fade_out` when the player executes the prompted action.
- [x] `[LTQE-COOL]`: **Cooldowns & Hybrid Once-Only History Tracking**
  - Implement a persistent run-scoped bitset (`PromptHistoryBitset`) for seen-once onboarding hints (`mine_alloy_hint`, `place_pipe_hint`, etc.).
  - Implement per-room cooldown array (`PromptCooldownTracker`) enforcing 5-15s repeat intervals on warnings and alerts.
- [x] `[LTQE-TOK]`: **Dual Action Token & Text Formatter Engine**
  - Implement lightweight parser replacing `{ATTACK}`, `{PLACE}`, `{CYCLE}`, `{PAN}`, `{SPARK}` with current active bindings (e.g., `[J]`, `[L+U]`, `[Q]`, `[R]`), while seamlessly accepting static string literals.


---

### `[PH-CTXP-TRIG]`: Phase 3 - Contextual Sensor & Systemic Hooks Architecture (`[TGSA]`) (COMPLETED)
*Focus: Generic condition-sensing infrastructure, proximity detection, and systemic event dispatch without hardcoded copy.*

- [x] `[TGSA-PROX]`: **Contextual Proximity & State Sensor Engine**
  - Implement proximity detection ($d \le 32\text{px}$) to interactive points (unlinked fixtures, empty spires, alloy nodes).
  - Gate trigger checks against progression flags (e.g., `level.can_build`, inventory thresholds, combat state).
- [x] `[TGSA-HOOK]`: **Systemic Event Dispatch Wiring**
  - Integrate dispatches into [`Player.cpp`](file:///Users/matt/code/cpp/alx/src/alx/Player.cpp), [`Network.cpp`](file:///Users/matt/code/cpp/alx/src/alx/Network.cpp), [`EnemyManager.cpp`](file:///Users/matt/code/cpp/alx/src/alx/EnemyManager.cpp), and [`MainScene.cpp`](file:///Users/matt/code/cpp/alx/src/alx/MainScene.cpp).
  - Connect player verbs to action-fulfillment events via `dismiss_if_matching(PromptId)`.


---

### `[PH-CTXP-JUICE]`: Phase 4 - Audio Blip & Visual Polish (`[PJUICE]`)
*Focus: Retro sound cues, border pulse animations, and visual polish.*

- [ ] `[SND-BLIP]`: **Subtle Retro Blip Audio Cue**
  - Play a short, crisp chiptune blip when Tier 0/1 prompts appear.
  - Play an urgent alert tone when Tier 2 threats trigger.
- [ ] `[VFX-SHIM]`: **Warning & Threat Border Shimmer**
  - Apply subtle sine-wave palette oscillation on Electric Lavender and Crimson Quartz borders during active hold.

---

### `[PH-CTXP-CURA]`: Phase 5 - Progression-Gated Content Curation (`[MCUR]`) *(Deferred to End)*
*Focus: Curating, balancing, and testing actual in-game copy and level-specific tutorial flows.*

- [ ] `[MCUR-LV1]`: **Level 1 Tutorial Script**
  - Basic movement, sword sweep `{ATTACK}`, camera scouting `{PAN}`, surviving twilight waves.
- [ ] `[MCUR-LV2]`: **Level 2 Logistics Script**
  - Alloy pickup (`"Pickup Alloy To Build"`), pipe placement `{PLACE}`, refiner conversion, spire defense.
- [ ] `[MCUR-LV3]`: **Level 3+ Threats Script**
  - Dark tower emergence, surge warnings, low HP alerts.
- [ ] `[MCUR-FIT]`: **Screen Width & Pacing Pass**
  - Ensure all curated copy fits inside 16px-tall banners and respects readable hold durations across 240x160 GBA resolution.
