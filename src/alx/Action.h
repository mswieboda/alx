#pragma once
#include <string>
#include <vector>
#include "core/Input.h"

namespace alx {

// =========================================================================
// GBA CONTROLLER HARDWARE MAPPING SCHEME
// =========================================================================
// Keyboard inputs strictly emulate a 2D GBA layout (A, B, L, R, D-Pad):
// - D-Pad (Up/Down/Left/Right): W / A / S / D  or  Arrow Keys
// - Button A: J or Z (Primary Attack / Action)
// - Button B: K or X (Cancel / Secondary Action)
// - L-Shoulder (L): Left Shift or Q (Pan Mode / Camera Scouting)
// - R-Shoulder (R): Right Shift or E (Modifier for Build / Cycle commands)
//
// Combos via R-Shoulder (Right Shift / E):
// - R-Shoulder + Button A (J/Z)  -> Build Tile
// - R-Shoulder + Right / D       -> Cycle Build Type Forward
// - R-Shoulder + Left / A        -> Cycle Build Type Backward
// =========================================================================

namespace GBA {
    inline const std::vector<int> DPAD_UP    = { MFB_KB_KEY_W, MFB_KB_KEY_UP };
    inline const std::vector<int> DPAD_DOWN  = { MFB_KB_KEY_S, MFB_KB_KEY_DOWN };
    inline const std::vector<int> DPAD_LEFT  = { MFB_KB_KEY_A, MFB_KB_KEY_LEFT };
    inline const std::vector<int> DPAD_RIGHT = { MFB_KB_KEY_D, MFB_KB_KEY_RIGHT };

    inline const std::vector<int> BUTTON_A   = { MFB_KB_KEY_J, MFB_KB_KEY_Z };
    inline const std::vector<int> BUTTON_B   = { MFB_KB_KEY_K, MFB_KB_KEY_X };
    inline const std::vector<int> L_SHOULDER = { MFB_KB_KEY_LEFT_SHIFT, MFB_KB_KEY_Q };
    inline const std::vector<int> R_SHOULDER = { MFB_KB_KEY_RIGHT_SHIFT, MFB_KB_KEY_E };
    inline const std::vector<int> START      = { MFB_KB_KEY_ENTER };
    inline const std::vector<int> SELECT     = { MFB_KB_KEY_TAB, MFB_KB_KEY_SPACE };
}

namespace Action {
    enum Type {
        MoveUp,
        MoveDown,
        MoveLeft,
        MoveRight,
        ActionBtn,  // Button A (J / Z): Primary Attack / Action
        Attack = ActionBtn, // Alias for Attack
        BuildTile,  // R-Shoulder (Right Shift / E) + Button A (J / Z): Place buildable tile
        Cancel,     // Button B (K / X): Cancel / Drain / Demolish
        Build,      // R-Shoulder (Right Shift / E): Modifier for Build / Cycle commands
        CycleNext,  // R-Shoulder (Right Shift / E) + D-Pad Right (D / Right): Cycle build type forward
        CyclePrev,  // R-Shoulder (Right Shift / E) + D-Pad Left (A / Left): Cycle build type backward
        PanMode,    // L-Shoulder (Left Shift / Q): Hold to pan camera scouting
        Menu,       // Enter (Start): Menu / Pause
        Map,        // Tab / Space (Select): Map
        DebugResource,  // 5: Temporary debug cheat +10 alloy
        DebugEnemyWave, // 6: Temporary debug cheat to spawn 2 enemies
        DebugTwUp,  // plus key
        DebugTwDown, // minus key
        Count
    };

    Type string_to_type(const std::string& action_str);
    std::string type_to_string(Type type);

    // Primary Enum API (Fast, O(1), Type-safe)
    bool is_pressed(Type type);
    bool is_just_pressed(Type type);

    // Semantic Combo Helpers (GBA R-Shoulder + D-Pad / Button A)
    bool is_attack();           // Button A without R-Shoulder (Build) held
    bool is_build_tile();       // R-Shoulder (Build) held + Button A
    bool is_cycle_right();      // R-Shoulder (Build) held + D-Pad Right
    bool is_cycle_left();       // R-Shoulder (Build) held + D-Pad Left

    // String API (Convenience overloads)
    bool is_pressed(const std::string& action_str);
    bool is_just_pressed(const std::string& action_str);

    // Dynamic Binding API
    void bind_key(Type type, int key);
    void unbind_key(Type type, int key);
    void reset_default_bindings();
    const std::vector<int>& bound_keys(Type type);
}

} // namespace alx
