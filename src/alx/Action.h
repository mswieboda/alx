#pragma once
#include <string>
#include <vector>
#include "core/Input.h"

namespace alx {

// =========================================================================
// GBA CONTROLLER HARDWARE MAPPING SCHEME
// =========================================================================
// GBA CONTROLLER HARDWARE MAPPING SCHEME
// =========================================================================
// Keyboard inputs strictly emulate a 2D GBA layout (A, B, L, R, D-Pad):
// - D-Pad (Up/Down/Left/Right): W / A / S / D  or  Arrow Keys
// - Button A: J or Z (Primary Attack / Action)
// - Button B: K or X (Cancel / Secondary Action)
// - L-Shoulder (L): Q (Pan Mode / Camera Scouting)
// - R-Shoulder (R): O (Modifier for Build commands)
//
// Combos via R-Shoulder (O):
// - R-Shoulder + Button A (J/Z)  -> Build Tile
// - R-Shoulder + Button B (K/X)  -> Remove Tile
// - R-Shoulder + L-Shoulder (Q)  -> Cycle Build Type
// =========================================================================

namespace GBA {
    inline const std::vector<int> DPAD_UP    = { MFB_KB_KEY_W, MFB_KB_KEY_UP };
    inline const std::vector<int> DPAD_DOWN  = { MFB_KB_KEY_S, MFB_KB_KEY_DOWN };
    inline const std::vector<int> DPAD_LEFT  = { MFB_KB_KEY_A, MFB_KB_KEY_LEFT };
    inline const std::vector<int> DPAD_RIGHT = { MFB_KB_KEY_D, MFB_KB_KEY_RIGHT };

    inline const std::vector<int> BUTTON_A   = { MFB_KB_KEY_J, MFB_KB_KEY_Z };
    inline const std::vector<int> BUTTON_B   = { MFB_KB_KEY_K, MFB_KB_KEY_X };
    inline const std::vector<int> L_SHOULDER = { MFB_KB_KEY_Q };
    inline const std::vector<int> R_SHOULDER = { MFB_KB_KEY_O };
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
        BuildTile,  // R-Shoulder (O) + Button A (J / Z): Place buildable tile
        Cancel,     // Button B (K / X): Cancel / Drain / Demolish
        Build,      // R-Shoulder (O): Modifier for Build commands
        BuildCycle, // R-Shoulder (O) + L-Shoulder (Q): Cycle build type forward
        PanMode,    // L-Shoulder (Q): Hold to pan camera scouting
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

    // Semantic Combo Helpers (GBA R-Shoulder + D-Pad / Button A / L-Shoulder)
    bool is_attack();           // Button A without R-Shoulder (Build) held
    bool is_attack_held();      // Button A held without R-Shoulder (Build) held
    bool is_build_tile();       // R-Shoulder (Build) held + Button A
    bool is_remove_tile();      // R-Shoulder (Build) held + Button B (Cancel)
    bool is_build_cycle();      // R-Shoulder (Build) held + L-Shoulder (PanMode)
    bool is_pan_mode_active();  // L-Shoulder held without R-Shoulder (Build) held

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
