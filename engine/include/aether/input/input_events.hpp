/**
 * @file input_events.hpp
 * @brief Input-Events für den EventBus.
 */
#pragma once

#include <aether/input/keys.hpp>

namespace aether::input {

struct KeyEvent {
    Key key = Key::Unknown;
    Action action = Action::Press;
    Modifiers mods{};
    i32 scancode = 0;
};

struct MouseButtonEvent {
    MouseButton button = MouseButton::Left;
    Action action = Action::Press;
    Modifiers mods{};
    f64 x = 0.0;
    f64 y = 0.0;
};

struct MouseMoveEvent {
    f64 x = 0.0;
    f64 y = 0.0;
    f64 dx = 0.0;
    f64 dy = 0.0;
};

struct MouseScrollEvent {
    f64 x_offset = 0.0;
    f64 y_offset = 0.0;
};

struct TextInputEvent {
    u32 codepoint = 0;
};

} // namespace aether::input
