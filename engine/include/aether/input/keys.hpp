/**
 * @file keys.hpp
 * @brief Tasten-, Maus- und Gamepad-Codes (backend-neutral).
 */
#pragma once

#include <aether/core/types.hpp>

namespace aether::input {

/**
 * @brief Tastencodes – an gängige Layouts angelehnt, eigene Nummern.
 */
enum class Key : i32 {
    Unknown = -1,

    Space = 32,
    Apostrophe = 39,
    Comma = 44,
    Minus = 45,
    Period = 46,
    Slash = 47,

    Num0 = 48, Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9,

    Semicolon = 59,
    Equal = 61,

    A = 65, B, C, D, E, F, G, H, I, J, K, L, M,
    N, O, P, Q, R, S, T, U, V, W, X, Y, Z,

    LeftBracket = 91,
    Backslash = 92,
    RightBracket = 93,
    GraveAccent = 96,

    Escape = 256,
    Enter,
    Tab,
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

    CapsLock = 280,
    ScrollLock,
    NumLock,
    PrintScreen,
    Pause,

    F1 = 290, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,

    LeftShift = 340,
    LeftControl,
    LeftAlt,
    LeftSuper,
    RightShift,
    RightControl,
    RightAlt,
    RightSuper,
    Menu,

    Count = 512,
};

enum class MouseButton : i32 {
    Left = 0,
    Right = 1,
    Middle = 2,
    Button4 = 3,
    Button5 = 4,
    Count = 8,
};

enum class GamepadButton : i32 {
    A = 0,
    B,
    X,
    Y,
    LeftBumper,
    RightBumper,
    Back,
    Start,
    Guide,
    LeftThumb,
    RightThumb,
    DpadUp,
    DpadRight,
    DpadDown,
    DpadLeft,
    Count,
};

enum class GamepadAxis : i32 {
    LeftX = 0,
    LeftY,
    RightX,
    RightY,
    LeftTrigger,
    RightTrigger,
    Count,
};

enum class Action : i32 {
    Press = 1,
    Release = 0,
    Repeat = 2,
};

struct Modifiers {
    bool shift = false;
    bool control = false;
    bool alt = false;
    bool super = false;
};

} // namespace aether::input
