Here is our consolidated design specification and C++ architectural blueprint for **`[TWBAR]`** based on everything we locked in.

---

## 1. C++ Enum & Struct Naming Proposal

In modern C++20, an `enum class` with an explicit `uint8_t` underlying type keeps memory tight and typesafe.

### Option 1: Flattened Expressive Tier Enum (Recommended)
This is the cleanest and most direct for `switch` statements in rendering and sound hooks:

```cpp
enum class MomentumTier : uint8_t {
    HeavyLight,
    ModerateLight,
    SlightLight,
    Equilibrium,
    SlightTwilight,
    ModerateTwilight,
    HeavyTwilight
};
```
* **Why this is great**: It reads completely naturally in code (e.g. `if (tier == MomentumTier::HeavyLight)`), avoids awkward abbreviations like `Tw`/`Lt`, and has zero runtime overhead.

### Option 2: Compact Direction + Intensity (Two Small Enums)
If you prefer separating "which way" from "how strong":

```cpp
enum class MomentumDir : uint8_t {
    Light,
    Neutral,
    Twilight
};

enum class MomentumStrength : uint8_t {
    None,
    Slight,
    Moderate,
    Heavy
};
```

---

## 2. Complete Visual & Kinetic Specification (`[TWBAR]`)

```
   [ ================== 66% ================== ]
              <<< [Light]           (Heavy Light)
                 [ ◇ ]              (Equilibrium)
                [Twilight] >>>      (Heavy Twilight)
```

### 1. Layout & Anchoring (`[FLOW-ANCHOR]`)
* **Position**: Centered horizontally 2–3 pixels directly below the top pill bar.
* **Center Anchor**:
  * Purifying $\rightarrow$ Current Light Glyph / Icon
  * Equilibrium $\rightarrow$ 5x5 Hollow Diamond (`◇`)
  * Corrupting $\rightarrow$ Current Twilight Glyph / Icon
* **Chevrons**:
  * Left flank when Purifying (`<`, `<<`, `<<<` pointing Left)
  * Right flank when Corrupting (`>`, `>>`, `>>>` pointing Right)
  * Empty flanks when in Equilibrium.

---

### 2. Kinetic FX & Juice

* **`[PULSE-BEAT]` (Luminance + Spacing Breathing)**:
  * Frequency maps to tier: $0.5\text{ Hz}$ (Equilibrium) $\to 1.0\text{ Hz}$ (Slight) $\to 2.0\text{ Hz}$ (Moderate) $\to 4.0\text{ Hz}$ (Heavy).
  * Smooth sine modulation on RGB brightness: $(1.0 \pm 0.25 \cdot \sin(2\pi f t))$.
  * On peak beat, chevron spacing breathes from 1px $\to$ 2px.
* **`[TIER-KICK]` (State Shift Micro-Bump)**:
  * When jumping to a higher or different tier, the entire indicator applies a **1–2px horizontal nudge** toward the winning direction for 2–3 frames (~50ms) before snapping back.
* **`[SNAP-FLASH]` (Direction Inversion Flash)**:
  * When crossing equilibrium (Light $\leftrightarrow$ Twilight flip), the indicator flashes solid **Ice-White** (for Light) or **Dark Mana Violet / Neon Purple** (for Twilight) for 3 frames before settling into its pulsing palette.

---

### 3. Palette Tokens

* **Light Spectrum**:
  * Flash: Pure White (`#FFFFFF`)
  * Heavy/Moderate: Vivid Ice-Cyan (`#00E5FF`)
  * Slight: Muted Cyan (`#40A8C0`)
* **Equilibrium**:
  * Neutral Slate Teal (`#4A6B82`)
* **Twilight Spectrum**:
  * Flash/Heavy: Neon Magenta-Violet (`#D154FF`)
  * Moderate: Dark Mana Violet (`#9B30FF`)
  * Slight: Muted Violet (`#7A4B9E`)

---

## 3. Momentum State Representation (Blueprint Sketch)

```cpp
struct MomentumState {
    float rolling_delta_15s{0.0f};       // Computed rolling rate (% change / 15s)
    MomentumTier current_tier{MomentumTier::Equilibrium};
    MomentumTier previous_tier{MomentumTier::Equilibrium};

    // Animation & Feedback timers
    float pulse_phase{0.0f};             // Running oscillator for sine breathing
    float kick_offset_x{0.0f};           // Current 1-2px nudge offset
    float kick_timer{0.0f};              // Remaining kick duration
    float flash_timer{0.0f};             // Remaining snap-flash duration
};
```

---

We have a clear and cohesive design ready. Whenever you'd like to take the next step—whether iterating on more details or moving into implementation planning—just let me know!