#pragma once

#include <cstdint>

enum class KeyCode : uint16_t {
    Unknown = 0,

    // Letters
    A, B, C, D, E, F, G, H, I, J, K, L, M,
    N, O, P, Q, R, S, T, U, V, W, X, Y, Z,

    // Numbers
    Key0, Key1, Key2, Key3, Key4, Key5, Key6, Key7, Key8, Key9,

    // Navigation & Controls
    Escape,
    Enter,
    Tab,
    Space,
    Backspace,
    Insert,
    Delete,
    Right,
    Left,
    Down,
    Up,
    PageUp,
    PageDown,
    Home,
    End,
    CapsLock,
    ScrollLock,
    NumLock,
    PrintScreen,
    Pause,
    Menu,

    // Keypad
    KP0, KP1, KP2, KP3, KP4, KP5, KP6, KP7, KP8, KP9,
    KPDecimal,
    KPDivide,
    KPMultiply,
    KPSubtract,
    KPAdd,
    KPEnter,
    KPEqual,

    // Symbols
    Equal,
    Minus,
    Plus,
    Comma,
    Period,
    Slash,
    Semicolon,
    Apostrophe,
    LeftBracket,
    RightBracket,
    Backslash,
    GraveAccent,

    // Modifiers
    LeftShift,
    RightShift,
    LeftControl,
    RightControl,
    LeftAlt,
    RightAlt,
    LeftSuper,
    RightSuper,

    // Function Keys
    F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,

    Count
};

enum class KeyModifier : uint8_t {
    None    = 0,
    Shift   = 1 << 0,
    Control = 1 << 1,
    Alt     = 1 << 2,
    Super   = 1 << 3
};

constexpr inline KeyModifier operator|(KeyModifier lhs, KeyModifier rhs) {
    return static_cast<KeyModifier>(static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs));
}

constexpr inline KeyModifier operator&(KeyModifier lhs, KeyModifier rhs) {
    return static_cast<KeyModifier>(static_cast<uint8_t>(lhs) & static_cast<uint8_t>(rhs));
}

constexpr inline KeyModifier operator^(KeyModifier lhs, KeyModifier rhs) {
    return static_cast<KeyModifier>(static_cast<uint8_t>(lhs) ^ static_cast<uint8_t>(rhs));
}

constexpr inline KeyModifier operator~(KeyModifier rhs) {
    return static_cast<KeyModifier>(~static_cast<uint8_t>(rhs));
}

constexpr inline KeyModifier& operator|=(KeyModifier& lhs, KeyModifier rhs) {
    lhs = lhs | rhs;
    return lhs;
}

constexpr inline KeyModifier& operator&=(KeyModifier& lhs, KeyModifier rhs) {
    lhs = lhs & rhs;
    return lhs;
}

constexpr inline KeyModifier& operator^=(KeyModifier& lhs, KeyModifier rhs) {
    lhs = lhs ^ rhs;
    return lhs;
}
