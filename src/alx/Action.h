#pragma once
#include <string>
#include <vector>
#include "core/Input.h"

namespace alx {

namespace Action {
    enum Type {
        MoveUp,
        MoveDown,
        MoveLeft,
        MoveRight,
        Tool,       // Button A (J / E / Space): Tool / Attack / Confirm
        Cancel,     // Button B (K / X): Cancel / Drain / Demolish
        CycleLeft,  // Q: Cycle left
        CycleRight, // E: Cycle right
        Menu,           // Enter: Menu / Pause
        Map,            // Tab: Map
        DebugResource,  // 5: Temporary debug cheat +10 alloy
        Count
    };

    Type string_to_type(const std::string& action_str);
    std::string type_to_string(Type type);

    // Primary Enum API (Fast, O(1), Type-safe)
    bool is_pressed(Type type);
    bool is_just_pressed(Type type);

    // String API (Convenience overloads)
    bool is_pressed(const std::string& action_str);
    bool is_just_pressed(const std::string& action_str);

    // Dynamic Binding API
    void bind_key(Type type, int key);
    void unbind_key(Type type, int key);
    void reset_default_bindings();
    const std::vector<int>& get_bound_keys(Type type);
}

} // namespace alx
