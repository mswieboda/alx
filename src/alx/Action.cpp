#include "alx/Action.h"
#include <unordered_map>
#include <algorithm>
#include "core/Log.h"
#include "minigamepad.h"

namespace alx {

namespace Action {

static const std::unordered_map<std::string, Type> s_string_to_action_map = {
    {"move_up",        MoveUp},
    {"move_down",      MoveDown},
    {"move_left",      MoveLeft},
    {"move_right",     MoveRight},
    {"action",         ActionBtn},
    {"tool",           ActionBtn},
    {"attack",         ActionBtn},
    {"confirm",        ActionBtn},
    {"a",              ActionBtn},
    {"build",          Build},
    {"build_tile",     BuildTile},
    {"cancel",         Cancel},
    {"b",              Cancel},
    {"build_cycle",    BuildCycle},
    {"pan_mode",       PanMode},
    {"menu",           Menu},
    {"map",            Map},
    {"debug_resource", DebugResource},
    {"debug_enemy_wave", DebugEnemyWave}
};

Type string_to_type(const std::string& action_str) {
    auto it = s_string_to_action_map.find(action_str);
    if (it != s_string_to_action_map.end()) {
        return it->second;
    }
    return Count;
}

std::string type_to_string(Type type) {
    switch (type) {
        case MoveUp:        return "move_up";
        case MoveDown:      return "move_down";
        case MoveLeft:      return "move_left";
        case MoveRight:     return "move_right";
        case ActionBtn:     return "action";
        case Build:         return "build";
        case BuildTile:     return "build_tile";
        case Cancel:        return "cancel";
        case BuildCycle:    return "build_cycle";
        case PanMode:       return "pan_mode";
        case Menu:          return "menu";
        case Map:           return "map";
        case DebugResource: return "debug_resource";
        case DebugEnemyWave: return "debug_enemy_wave";
        default:            return "unknown";
    }
}

// =========================================================================
// GBA CONTROLLER HARDWARE MAPPING SCHEME
// =========================================================================
// Keyboard inputs strictly emulate a 2D GBA layout (A, B, L, R, D-Pad):
// - D-Pad (Up/Down/Left/Right): W / A / S / D  or  Arrow Keys
// - Button A: J or Z (Primary Attack / Action)
// - Button B: K or X (Cancel / Secondary Action)
// - L-Shoulder (L): Q (Pan Mode / Camera Scouting)
// - R-Shoulder (R): O (Modifier for Build mode)
//
// Combos via R-Shoulder (O):
// - R-Shoulder + Button A (J/Z)  -> Build Tile
// - R-Shoulder + Button B (K/X)  -> Remove Tile
// - R-Shoulder + L-Shoulder (Q)  -> Cycle Build Type
// =========================================================================

static std::vector<int> s_bindings[static_cast<size_t>(Count)];
static bool s_initialized = false;

static void ensure_initialized() {
    if (s_initialized) return;
    reset_default_bindings();
    s_initialized = true;
}

void reset_default_bindings() {
    for (auto& vec : s_bindings) {
        vec.clear();
    }

    s_bindings[static_cast<size_t>(MoveUp)]        = GBA::DPAD_UP;
    s_bindings[static_cast<size_t>(MoveDown)]      = GBA::DPAD_DOWN;
    s_bindings[static_cast<size_t>(MoveLeft)]      = GBA::DPAD_LEFT;
    s_bindings[static_cast<size_t>(MoveRight)]     = GBA::DPAD_RIGHT;

    s_bindings[static_cast<size_t>(ActionBtn)]     = GBA::BUTTON_A;
    s_bindings[static_cast<size_t>(BuildTile)]     = GBA::BUTTON_A;

    s_bindings[static_cast<size_t>(Cancel)]        = GBA::BUTTON_B;

    s_bindings[static_cast<size_t>(PanMode)]       = GBA::L_SHOULDER;
    s_bindings[static_cast<size_t>(Build)]         = GBA::R_SHOULDER;
    s_bindings[static_cast<size_t>(BuildCycle)]    = GBA::L_SHOULDER;

    s_bindings[static_cast<size_t>(Menu)]          = GBA::START;
    s_bindings[static_cast<size_t>(Map)]           = GBA::SELECT;

    s_bindings[static_cast<size_t>(DebugResource)] = { MFB_KB_KEY_5 };
    s_bindings[static_cast<size_t>(DebugEnemyWave)] = { MFB_KB_KEY_6 };
    s_bindings[static_cast<size_t>(DebugTwUp)]     = { MFB_KB_KEY_EQUAL };
    s_bindings[static_cast<size_t>(DebugTwDown)]   = { MFB_KB_KEY_MINUS };
}

void bind_key(Type type, int key) {
    ensure_initialized();
    if (type == Count) return;

    auto& vec = s_bindings[static_cast<size_t>(type)];
    if (std::find(vec.begin(), vec.end(), key) == vec.end()) {
        vec.push_back(key);
    }
}

void unbind_key(Type type, int key) {
    ensure_initialized();
    if (type == Count) return;

    auto& vec = s_bindings[static_cast<size_t>(type)];
    vec.erase(std::remove(vec.begin(), vec.end(), key), vec.end());
}

const std::vector<int>& bound_keys(Type type) {
    ensure_initialized();
    if (type == Count) {
        static const std::vector<int> empty_vec;
        return empty_vec;
    }
    return s_bindings[static_cast<size_t>(type)];
}

static bool is_gamepad_action_pressed(Type type) {
    if (!::Input::is_gamepad_connected()) return false;

    switch (type) {
        case MoveUp:
            return ::Input::is_gamepad_button_pressed(MG_BUTTON_DPAD_UP) ||
                   ::Input::is_gamepad_stick_dir_pressed(0);
        case MoveDown:
            return ::Input::is_gamepad_button_pressed(MG_BUTTON_DPAD_DOWN) ||
                   ::Input::is_gamepad_stick_dir_pressed(1);
        case MoveLeft:
            return ::Input::is_gamepad_button_pressed(MG_BUTTON_DPAD_LEFT) ||
                   ::Input::is_gamepad_stick_dir_pressed(2);
        case MoveRight:
            return ::Input::is_gamepad_button_pressed(MG_BUTTON_DPAD_RIGHT) ||
                   ::Input::is_gamepad_stick_dir_pressed(3);
        case ActionBtn:
        case BuildTile:
            return ::Input::is_gamepad_button_pressed(MG_BUTTON_SOUTH);
        case Cancel:
            return ::Input::is_gamepad_button_pressed(MG_BUTTON_EAST);
        case Build:
            return ::Input::is_gamepad_button_pressed(MG_BUTTON_RIGHT_SHOULDER);
        case PanMode:
        case BuildCycle:
            return ::Input::is_gamepad_button_pressed(MG_BUTTON_LEFT_SHOULDER);
        case Menu:
            return ::Input::is_gamepad_button_pressed(MG_BUTTON_START);
        case Map:
            return ::Input::is_gamepad_button_pressed(MG_BUTTON_BACK) ||
                   ::Input::is_gamepad_button_pressed(MG_BUTTON_TOUCHPAD) ||
                   ::Input::is_gamepad_button_pressed(MG_BUTTON_MISC1);
        default:
            return false;
    }
}

static bool is_gamepad_action_just_pressed(Type type) {
    if (!::Input::is_gamepad_connected()) return false;

    switch (type) {
        case MoveUp:
            return ::Input::is_gamepad_button_just_pressed(MG_BUTTON_DPAD_UP) ||
                   ::Input::is_gamepad_stick_dir_just_pressed(0);
        case MoveDown:
            return ::Input::is_gamepad_button_just_pressed(MG_BUTTON_DPAD_DOWN) ||
                   ::Input::is_gamepad_stick_dir_just_pressed(1);
        case MoveLeft:
            return ::Input::is_gamepad_button_just_pressed(MG_BUTTON_DPAD_LEFT) ||
                   ::Input::is_gamepad_stick_dir_just_pressed(2);
        case MoveRight:
            return ::Input::is_gamepad_button_just_pressed(MG_BUTTON_DPAD_RIGHT) ||
                   ::Input::is_gamepad_stick_dir_just_pressed(3);
        case ActionBtn:
        case BuildTile:
            return ::Input::is_gamepad_button_just_pressed(MG_BUTTON_SOUTH);
        case Cancel:
            return ::Input::is_gamepad_button_just_pressed(MG_BUTTON_EAST);
        case Build:
            return ::Input::is_gamepad_button_just_pressed(MG_BUTTON_RIGHT_SHOULDER);
        case PanMode:
        case BuildCycle:
            return ::Input::is_gamepad_button_just_pressed(MG_BUTTON_LEFT_SHOULDER);
        case Menu:
            return ::Input::is_gamepad_button_just_pressed(MG_BUTTON_START);
        case Map:
            return ::Input::is_gamepad_button_just_pressed(MG_BUTTON_BACK) ||
                   ::Input::is_gamepad_button_just_pressed(MG_BUTTON_TOUCHPAD) ||
                   ::Input::is_gamepad_button_just_pressed(MG_BUTTON_MISC1);
        default:
            return false;
    }
}

bool is_pressed(Type type) {
    ensure_initialized();
    if (type == Count) return false;

    for (int key : s_bindings[static_cast<size_t>(type)]) {
        if (::Input::is_key_pressed(key)) return true;
    }
    return is_gamepad_action_pressed(type);
}

bool is_just_pressed(Type type) {
    ensure_initialized();
    if (type == Count) return false;

    for (int key : s_bindings[static_cast<size_t>(type)]) {
        if (::Input::is_key_just_pressed(key)) {
            return true;
        }
    }
    if (is_gamepad_action_just_pressed(type)) {
        return true;
    }
    return false;
}

bool is_attack() {
    return !is_pressed(Build) && is_just_pressed(ActionBtn);
}

bool is_build_tile() {
    return is_pressed(Build) && is_just_pressed(ActionBtn);
}

bool is_remove_tile() {
    return is_pressed(Build) && is_just_pressed(Cancel);
}

bool is_build_cycle() {
    return is_pressed(Build) && is_just_pressed(BuildCycle);
}

bool is_pan_mode_active() {
    return !is_pressed(Build) && is_pressed(PanMode);
}

bool is_pressed(const std::string& action_str) {
    return is_pressed(string_to_type(action_str));
}

bool is_just_pressed(const std::string& action_str) {
    return is_just_pressed(string_to_type(action_str));
}

} // namespace Action

} // namespace alx
