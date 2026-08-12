#pragma once

#include <string>
#include <vector>
#include "core/KeyCodes.h"
#include "core/Input.h"

namespace alx {

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
    inline const std::vector<KeyCode> DPAD_UP    = { KeyCode::W };
    inline const std::vector<KeyCode> DPAD_DOWN  = { KeyCode::S };
    inline const std::vector<KeyCode> DPAD_LEFT  = { KeyCode::A };
    inline const std::vector<KeyCode> DPAD_RIGHT = { KeyCode::D };

    inline const std::vector<KeyCode> BUTTON_A   = { KeyCode::J };
    inline const std::vector<KeyCode> BUTTON_B   = { KeyCode::K };
    inline const std::vector<KeyCode> BUTTON_X   = { KeyCode::U };
    inline const std::vector<KeyCode> BUTTON_Y   = { KeyCode::I };
    inline const std::vector<KeyCode> L_SHOULDER = { KeyCode::Tab, KeyCode::LeftShift };
    inline const std::vector<KeyCode> R_SHOULDER = { KeyCode::L };
    inline const std::vector<KeyCode> L_TRIGGER  = { KeyCode::Z, KeyCode::Space };
    inline const std::vector<KeyCode> R_TRIGGER  = { KeyCode::Semicolon };
    inline const std::vector<KeyCode> START      = { KeyCode::Enter };
    inline const std::vector<KeyCode> SELECT     = { KeyCode::Backspace };
}

namespace Action {
    enum Type {
        MoveUp,
        MoveDown,
        MoveLeft,
        MoveRight,
        ActionBtn,        // Button A (J / Z): Primary Attack / Action
        Attack = ActionBtn, // Alias for Attack
        Place,            // West (X / U) or R-Shoulder (O) + Button A: Place fixture foundation/tile
        Cancel,           // Button B (K / X) or East (B): Cancel / Demolish
        BuildMode,        // R-Shoulder (O): Modifier for Build mode
        BuildCycle,       // R-Shoulder (O) + L-Shoulder (Q) or North (Y / I): Cycle build type forward
        PanMode,          // L-Shoulder (Q) or L1 Bumper: Hold to pan camera scouting
        Menu,             // Enter (Start): Menu / Pause
        ManaSpark,        // Right Trigger (R2) or P: Charge/fire mana spark
        BuildFoundation,  // Future action to build/energize foundation to completion
        DebugResource,    // 5: Temporary debug cheat +10 alloy
        DebugEnemyWave,   // 6: Temporary debug cheat to spawn 2 enemies
        DebugTwUp,        // plus key
        DebugTwDown,      // minus key
        Count,

        // Backwards compatibility aliases
        Build = BuildMode,
        BuildTile = Place
    };

    Type string_to_type(const std::string& action_str);
    std::string type_to_string(Type type);

    // Primary Enum API (Fast, O(1), Type-safe)
    bool is_pressed(Type type);
    bool is_just_pressed(Type type);

    // Semantic Combo Helpers (GBA R-Shoulder + D-Pad / Button A / L-Shoulder)
    bool is_attack();              // Button A (J / Z) primary attack (unconditional)
    bool is_attack_held();         // Button A (J / Z) held (unconditional)
    bool is_place_fixture();       // R-Shoulder (BuildMode) held + West (X / U)
    bool is_place_fixture_held();  // R-Shoulder (BuildMode) held + West (X / U) held down
    bool is_remove_fixture();      // R-Shoulder (BuildMode) held + Button B (Cancel)
    bool is_build_cycle();         // North (Y / I) + Button Y
    bool is_pan_mode_active();     // L-Shoulder held + R-Shoulder (BuildMode) not held
    bool is_build_foundation();    // BuildFoundation action trigger

    // String API (Convenience overloads)
    bool is_pressed(const std::string& action_str);
    bool is_just_pressed(const std::string& action_str);

    // Dynamic Binding API
    void bind_key(Type type, KeyCode key);
    void bind_key(Type type, int key);
    void unbind_key(Type type, KeyCode key);
    void unbind_key(Type type, int key);
    void reset_default_bindings();
    const std::vector<KeyCode>& bound_keys(Type type);
}

} // namespace alx
