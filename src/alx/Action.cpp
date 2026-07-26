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
    {"cancel",         Cancel},
    {"b",              Cancel},
    {"cycle",          Cycle},
    {"cycle_right",    Cycle},
    {"pan_mode",       PanMode},
    {"menu",           Menu},
    {"map",            Map},
    {"debug_resource", DebugResource}
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
        case Cancel:        return "cancel";
        case Cycle:         return "cycle";
        case PanMode:       return "pan_mode";
        case Menu:          return "menu";
        case Map:           return "map";
        case DebugResource: return "debug_resource";
        default:            return "unknown";
    }
}

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

    s_bindings[static_cast<size_t>(MoveUp)]        = { MFB_KB_KEY_W, MFB_KB_KEY_UP };
    s_bindings[static_cast<size_t>(MoveDown)]      = { MFB_KB_KEY_S, MFB_KB_KEY_DOWN };
    s_bindings[static_cast<size_t>(MoveLeft)]      = { MFB_KB_KEY_A, MFB_KB_KEY_LEFT };
    s_bindings[static_cast<size_t>(MoveRight)]     = { MFB_KB_KEY_D, MFB_KB_KEY_RIGHT };

    s_bindings[static_cast<size_t>(ActionBtn)]     = { MFB_KB_KEY_J, MFB_KB_KEY_Z };
    s_bindings[static_cast<size_t>(Cancel)]        = { MFB_KB_KEY_K, MFB_KB_KEY_X };
    s_bindings[static_cast<size_t>(Cycle)]         = { MFB_KB_KEY_RIGHT_SHIFT, MFB_KB_KEY_E };
    s_bindings[static_cast<size_t>(PanMode)]       = { MFB_KB_KEY_LEFT_SHIFT, MFB_KB_KEY_Q };
    s_bindings[static_cast<size_t>(Menu)]          = { MFB_KB_KEY_ENTER };
    s_bindings[static_cast<size_t>(Map)]           = { MFB_KB_KEY_TAB, MFB_KB_KEY_SPACE };
    s_bindings[static_cast<size_t>(DebugResource)] = { MFB_KB_KEY_5 };
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

const std::vector<int>& get_bound_keys(Type type) {
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

bool is_pressed(const std::string& action_str) {
    return is_pressed(string_to_type(action_str));
}

bool is_just_pressed(const std::string& action_str) {
    return is_just_pressed(string_to_type(action_str));
}

} // namespace Action

} // namespace alx
