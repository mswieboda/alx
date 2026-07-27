#include "alx/Action.h"
#include <unordered_map>
#include <algorithm>

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
    {"cycle_right",    CycleNext},
    {"cycle_next",     CycleNext},
    {"cycle_prev",     CyclePrev},
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
        case CycleNext:     return "cycle_next";
        case CyclePrev:     return "cycle_prev";
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
// - L-Shoulder (L): Left Shift or Q (Pan Mode / Camera Scouting)
// - R-Shoulder (R): Right Shift or E (Modifier for Build / Cycle commands)
//
// Combos via R-Shoulder (Right Shift / E):
// - R-Shoulder + Button A (J/Z)  -> Build Tile
// - R-Shoulder + Right / D       -> Cycle Build Type Forward
// - R-Shoulder + Left / A        -> Cycle Build Type Backward
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
    s_bindings[static_cast<size_t>(CycleNext)]     = GBA::DPAD_RIGHT;
    s_bindings[static_cast<size_t>(CyclePrev)]     = GBA::DPAD_LEFT;

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

bool is_pressed(Type type) {
    ensure_initialized();
    if (type == Count) return false;

    for (int key : s_bindings[static_cast<size_t>(type)]) {
        if (::Input::is_key_pressed(key)) return true;
    }
    return false;
}

bool is_just_pressed(Type type) {
    ensure_initialized();
    if (type == Count) return false;

    for (int key : s_bindings[static_cast<size_t>(type)]) {
        if (::Input::is_key_just_pressed(key)) return true;
    }
    return false;
}

bool is_attack() {
    return !is_pressed(Build) && is_just_pressed(ActionBtn);
}

bool is_build_tile() {
    return is_pressed(Build) && is_just_pressed(ActionBtn);
}

bool is_cycle_right() {
    return is_pressed(Build) && (is_just_pressed(MoveRight));
}

bool is_cycle_left() {
    return is_pressed(Build) && is_just_pressed(MoveLeft);
}

bool is_pressed(const std::string& action_str) {
    return is_pressed(string_to_type(action_str));
}

bool is_just_pressed(const std::string& action_str) {
    return is_just_pressed(string_to_type(action_str));
}

} // namespace Action

} // namespace alx
