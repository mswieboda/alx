# [EP-CTXP] Phase 5 Implementation Plan: Level 1 Tutorial Script & Curation (`[PH-CTXP-LV1]`)

This document details the refined plan to curate, wire up, and pace the **Level 1 player onboarding and tutorial sequence** (`[PH-CTXP-LV1]`) in *Aetherlux* (`alx`).

---

## 1. Architecture & Onboarding State Machine

```mermaid
flowchart TD
    A[Level 1 Starts at Spawn {9,9}] -->|Delay 0.75s| B["Step 1: 'Press {ATTACK} to Strike' (Sticky)"]
    B -->|Player Swings Sword| C[Auto-Dismiss sword_attack_hint]
    C -->|Delay 3.0s| D["Step 2: 'Hold {PAN} to Scout Ahead' (Sticky)"]
    D -->|Player Holds Pan| E[Auto-Dismiss camera_pan_hint]
    E -->|Delay 3.0s| F["Step 3: 'Protect the Mana Network' (Hold 3.5s)"]
    F -->|Delay 5.0s| G["Step 4: 'Hold {SPARK} to Fire Spark' (Sticky)"]
    G -->|Player Fires Mana Spark| H[Auto-Dismiss mana_spark_hint]
    H --> I[Proximity Sensor Engine Active]
    
    I -->|Player near Refiner <= 48px| J["Hint: 'Refiner: Purifies Dark Mana' (Run-Once)"]
    I -->|Player near Spire <= 48px| K["Hint: 'Spire: Burns Light Mana' -> 'Light Mana Clears Twilight' (Run-Once)"]
    I -->|Player near Dark Tower <= 48px| L["Warning: 'Strike Tower with {ATTACK}' (Cooldown 20s)"]
    I -->|Player HP == 1| M["Alert: 'WARNING: Low health!' (Cooldown 15s)"]
    I -->|Dark Tower Emerges| N["Alert: 'ALERT: Dark Tower emerged!'"]
    I -->|All Twilight Cleared| O["Info: 'Room Purified - Cleared!'"]
```

---

## 2. Curated Copy & Screen Fit Analysis (`[MCUR-FIT1]`)

All messages fit strictly within the 240px GBA screen width (assuming standard 8px glyph spacing, 16px horizontal padding, 8px margin &rarr; max safe width 224px / ~28 characters):

| ID | Prompt String | Resolved Keyboard | Resolved Gamepad | Max Width | Screen Margin Safe? |
| :--- | :--- | :--- | :--- | :--- | :--- |
| `sword_attack_hint` | `"Press {ATTACK} to Strike"` | `"Press [J] to Strike"` (19 ch) | `"Press [A] to Strike"` (19 ch) | 162px |  **Yes** (+62px safe) |
| `camera_pan_hint` | `"Hold {PAN} to Scout Ahead"` | `"Hold [Tab] to Scout Ahead"` (25 ch) | `"Hold [L] to Scout Ahead"` (23 ch) | 208px |  **Yes** (+16px safe) |
| `protect_network_hint` | `"Protect the Mana Network"` | `"Protect the Mana Network"` (24 ch) | `"Protect the Mana Network"` (24 ch) | 202px |  **Yes** (+22px safe) |
| `mana_spark_hint` | `"Hold {SPARK} to Fire Spark"` | `"Hold [;] to Fire Spark"` (23 ch) | `"Hold [ZR] to Fire Spark"` (23 ch) | 194px |  **Yes** (+30px safe) |
| `refiner_info` | `"Refiner: Purifies Dark Mana"` | `"Refiner: Purifies Dark Mana"` (27 ch) | `"Refiner: Purifies Dark Mana"` (27 ch) | 226px |  **Yes** (+14px safe) |
| `spire_info` (Slot 1) | `"Spire: Burns Light Mana"` | `"Spire: Burns Light Mana"` (23 ch) | `"Spire: Burns Light Mana"` (23 ch) | 194px |  **Yes** (+30px safe) |
| `spire_info` (Slot 2) | `"Light Mana Clears Twilight"` | `"Light Mana Clears Twilight"` (26 ch) | `"Light Mana Clears Twilight"` (26 ch) | 218px |  **Yes** (+6px safe) |
| `dark_tower_hint` | `"Strike Tower with {ATTACK}"` | `"Strike Tower with [J]"` (21 ch) | `"Strike Tower with [A]"` (21 ch) | 178px |  **Yes** (+46px safe) |
| `player_low_hp_alert` | `"WARNING: Low health!"` | `"WARNING: Low health!"` (20 ch) | `"WARNING: Low health!"` (20 ch) | 170px |  **Yes** (+54px safe) |
| `tower_emerged_alert`| `"ALERT: Dark Tower emerged!"` | `"ALERT: Dark Tower emerged!"` (26 ch) | `"ALERT: Dark Tower emerged!"` (26 ch) | 218px |  **Yes** (+6px safe) |
| `room_purified_info` | `"Room Purified - Cleared!"` | `"Room Purified - Cleared!"` (24 ch) | `"Room Purified - Cleared!"` (24 ch) | 202px |  **Yes** (+22px safe) |

---

## 3. Proposed Changes

### Prompts & Enums Layer

#### [MODIFY] [`src/alx/PromptOverlay.h`](file:///Users/matt/code/cpp/alx/src/alx/PromptOverlay.h)
- Add new Level 1 prompt IDs to `enum class PromptId`:
  - `sword_attack_hint`
  - `protect_network_hint`
  - `refiner_info`
  - `spire_info`
  - `dark_tower_hint`

#### [MODIFY] [`src/alx/PromptOverlay.cpp`](file:///Users/matt/code/cpp/alx/src/alx/PromptOverlay.cpp)
- Add alias support for `{MANA_SPARK}` in `format_tokens`:
  ```cpp
  if (token == "SPARK" || token == "MANA_SPARK") return is_gamepad ? "[ZR]" : "[;]";
  ```

---

### Context Sensor Layer

#### [MODIFY] [`src/alx/PlayerContextSensor.h`](file:///Users/matt/code/cpp/alx/src/alx/PlayerContextSensor.h)
- Add timing and state tracking for the Level 1 sequence:
  ```cpp
  float m_level_elapsed_sec{0.0f};
  float m_post_attack_delay_sec{0.0f};
  float m_post_network_delay_sec{0.0f};
  bool m_network_hint_queued{false};
  bool m_spark_hint_queued{false};
  ```

#### [MODIFY] [`src/alx/PlayerContextSensor.cpp`](file:///Users/matt/code/cpp/alx/src/alx/PlayerContextSensor.cpp)
- In `PlayerContextSensor::update(...)`:
  1. **Step 1 (Attack)**: At `m_level_elapsed_sec >= 0.75f` &rarr; show sticky `"Press {ATTACK} to Strike"`.
  2. **Step 2 (Camera Pan)**: Once attack is dismissed, wait `3.0s` delay &rarr; show sticky `"Hold {PAN} to Scout Ahead"`.
  3. **Step 3 (Network Goal)**: Once pan is dismissed &rarr; show `"Protect the Mana Network"` (hold 3.5s).
  4. **Step 4 (Mana Spark)**: 5.0s after network goal &rarr; show sticky `"Hold {SPARK} to Fire Spark"`.
  5. **Step 5 (Refiner Proximity)**: Near Refiner ($d \le 48\text{px}$) &rarr; `overlay.try_show_once("Refiner: Purifies Dark Mana", PromptType::info, PromptId::refiner_info)`.
  6. **Step 6 (Spire Proximity)**: Near Spire ($d \le 48\text{px}$) &rarr; `overlay.try_show_once("Spire: Burns Light Mana", PromptType::info, PromptId::spire_info)` + queue `"Light Mana Clears Twilight"`.
  7. **Step 7 (Dark Tower Proximity)**: Near Dark Tower ($d \le 48\text{px}$) &rarr; `overlay.try_show_cooldown("Strike Tower with {ATTACK}", PromptType::warning, PromptId::dark_tower_hint, 3.5f, false, 20.0f)`.

---

### Player & Scene Dispatches

#### [MODIFY] [`src/alx/Player.cpp`](file:///Users/matt/code/cpp/alx/src/alx/Player.cpp)
- When `attack_just_pressed`: `prompt_overlay->dismiss_if_matching(PromptId::sword_attack_hint);`

#### [MODIFY] [`src/alx/WorldStructure.cpp`](file:///Users/matt/code/cpp/alx/src/alx/WorldStructure.cpp) (or hit routing)
- When a Dark Tower takes damage from player sword: `prompt_overlay->dismiss_if_matching(PromptId::dark_tower_hint);`

#### [MODIFY] [`src/alx/MainScene.cpp`](file:///Users/matt/code/cpp/alx/src/alx/MainScene.cpp)
- Update room clear prompt text to `"Room Purified - Cleared!"`.

---

### Plan Tracking

#### [MODIFY] [`plans/phases/prompt-sys.md`](file:///Users/matt/code/cpp/alx/plans/phases/prompt-sys.md)
- Update Phase 5 status: `### [PH-CTXP-LV1]: Phase 5 - Level 1 Tutorial Script & Curation ([CLV1]) (COMPLETED)`.
- Mark subtasks `[MCUR-LV1]` and `[MCUR-FIT1]` as completed (`- [x]`).

---

## 4. Verification Plan

### Automated Tests / Compilation
1. Run build verification:
   ```bash
   task build
   task build-release
   ```
2. Verify binary size constraint:
   ```bash
   task size-release
   ```

### Manual Verification
1. Launch game (`task run` or inspect runtime) and verify the sequential flow:
   - 0.75s: `"Press [J] to Strike"` slides up with tactile bass blip.
   - Strike with sword: prompt dismisses instantly.
   - 3.0s later: `"Hold [Tab] to Scout Ahead"` slides up.
   - Hold Tab: camera pans and prompt dismisses instantly.
   - `"Protect the Mana Network"` displays for 3.5s.
   - 5.0s later: `"Hold [;] to Fire Spark"` slides up.
   - Hold `;` to charge and release spark: fires spark and prompt dismisses.
   - Walk near the Refiner at {10,8}: `"Refiner: Purifies Dark Mana"` displays.
   - Walk near the Spire at {6,6}: `"Spire: Burns Light Mana"` displays.
   - Walk near an active Dark Tower: `"Strike Tower with [J]"` displays with warning shimmer.
   - Striking the Dark Tower damages it and dismisses the hint.
   - All banners fit within 240&times;160 resolution without overflowing screen margins.
