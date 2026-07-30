#include "Input.h"
#include <cstring>
#include <MiniFB.h>
#include <algorithm>

#define MG_IMPLEMENTATION
#include "minigamepad.h"

namespace Input {
    bool current_keys[MAX_KEYS] = { false };
    bool just_pressed_keys[MAX_KEYS] = { false };

    static mfb_key_mod current_mods = static_cast<mfb_key_mod>(0);

    // Minigamepad state
    static mg_gamepads s_gamepads = {};
    static bool s_gamepad_initialized = false;

    static bool current_gamepad_buttons[MG_BUTTON_COUNT] = { false };
    static bool just_pressed_gamepad_buttons[MG_BUTTON_COUNT] = { false };
    static bool prev_gamepad_buttons[MG_BUTTON_COUNT] = { false };

    // Stick direction state (0: Up, 1: Down, 2: Left, 3: Right)
    static bool current_stick_dirs[4] = { false };
    static bool just_pressed_stick_dirs[4] = { false };
    static bool prev_stick_dirs[4] = { false };

    void init() {
        if (!s_gamepad_initialized) {
            mg_gamepads_init(&s_gamepads);
            s_gamepad_initialized = true;
        }
    }

    void cleanup() {
        if (s_gamepad_initialized) {
            mg_gamepads_free(&s_gamepads);
            s_gamepad_initialized = false;
        }
    }

    void update_input_state(struct mfb_window *window) {
        if (!s_gamepad_initialized) {
            init();
        }

        if (window && !mfb_is_window_active(window)) {
            force_clear_all_inputs();
            return;
        }

        // Poll minigamepad
        mg_gamepads_poll(&s_gamepads);

        // Update previous gamepad button and stick states
        for (int i = 0; i < MG_BUTTON_COUNT; ++i) {
            prev_gamepad_buttons[i] = current_gamepad_buttons[i];
            current_gamepad_buttons[i] = false;
        }
        for (int i = 0; i < 4; ++i) {
            prev_stick_dirs[i] = current_stick_dirs[i];
            current_stick_dirs[i] = false;
        }

        // Poll all connected gamepads
        for (mg_gamepad* pad = s_gamepads.list.head; pad; pad = pad->next) {
            // Digital buttons
            for (int b = 0; b < MG_BUTTON_COUNT; ++b) {
                if (mg_gamepad_button_is_pressed(pad, static_cast<mg_button>(b))) {
                    current_gamepad_buttons[b] = true;
                }
            }

            // Left Stick Axes (Emulate D-Pad)
            float ly = mg_gamepad_axis_value(pad, MG_AXIS_LEFT_Y);
            float lx = mg_gamepad_axis_value(pad, MG_AXIS_LEFT_X);

            if (ly < -0.5f) current_stick_dirs[0] = true; // Up
            if (ly > 0.5f)  current_stick_dirs[1] = true; // Down
            if (lx < -0.5f) current_stick_dirs[2] = true; // Left
            if (lx > 0.5f)  current_stick_dirs[3] = true; // Right
        }

        // Detect rising edge (just pressed)
        for (int i = 0; i < MG_BUTTON_COUNT; ++i) {
            if (current_gamepad_buttons[i] && !prev_gamepad_buttons[i]) {
                just_pressed_gamepad_buttons[i] = true;
            }
        }
        for (int i = 0; i < 4; ++i) {
            if (current_stick_dirs[i] && !prev_stick_dirs[i]) {
                just_pressed_stick_dirs[i] = true;
            }
        }
    }

    void clear_just_pressed() {
        std::fill(std::begin(just_pressed_keys), std::end(just_pressed_keys), false);
        std::fill(std::begin(just_pressed_gamepad_buttons), std::end(just_pressed_gamepad_buttons), false);
        std::fill(std::begin(just_pressed_stick_dirs), std::end(just_pressed_stick_dirs), false);
    }

    void keyboard_callback(struct mfb_window *window, mfb_key key, mfb_key_mod mod, bool is_pressed) {
        const int key_code = static_cast<int>(key);

        current_mods = mod;

        if (key_code >= 0 && key_code < MAX_KEYS) {
            if (is_pressed && !current_keys[key_code]) {
                just_pressed_keys[key_code] = true;
            }

            current_keys[key_code] = is_pressed;
        }
    }

    void window_active_callback(struct mfb_window *window, bool is_active) {
        force_clear_all_inputs();
    }

    void force_clear_all_inputs() {
        std::fill(std::begin(current_keys), std::end(current_keys), false);
        std::fill(std::begin(just_pressed_keys), std::end(just_pressed_keys), false);

        std::fill(std::begin(current_gamepad_buttons), std::end(current_gamepad_buttons), false);
        std::fill(std::begin(just_pressed_gamepad_buttons), std::end(just_pressed_gamepad_buttons), false);
        std::fill(std::begin(prev_gamepad_buttons), std::end(prev_gamepad_buttons), false);

        std::fill(std::begin(current_stick_dirs), std::end(current_stick_dirs), false);
        std::fill(std::begin(just_pressed_stick_dirs), std::end(just_pressed_stick_dirs), false);
        std::fill(std::begin(prev_stick_dirs), std::end(prev_stick_dirs), false);

        current_mods = static_cast<mfb_key_mod>(0);
    }

    bool is_key_pressed(int key) {
        if (key < 0 || key >= MAX_KEYS) return false;
        return current_keys[key];
    }

    bool is_key_just_pressed(int key) {
        if (key < 0 || key >= MAX_KEYS) return false;
        return just_pressed_keys[key];
    }

    bool is_modifier_held(mfb_key_mod modifier_bit) {
        return (current_mods & modifier_bit) != 0;
    }

    bool is_gamepad_connected() {
        return s_gamepad_initialized && (s_gamepads.list.head != nullptr);
    }

    bool is_gamepad_button_pressed(int button) {
        if (button < 0 || button >= MG_BUTTON_COUNT) return false;
        return current_gamepad_buttons[button];
    }

    bool is_gamepad_button_just_pressed(int button) {
        if (button < 0 || button >= MG_BUTTON_COUNT) return false;
        return just_pressed_gamepad_buttons[button];
    }

    bool is_gamepad_stick_dir_pressed(int dir) {
        if (dir < 0 || dir >= 4) return false;
        return current_stick_dirs[dir];
    }

    bool is_gamepad_stick_dir_just_pressed(int dir) {
        if (dir < 0 || dir >= 4) return false;
        return just_pressed_stick_dirs[dir];
    }
}
