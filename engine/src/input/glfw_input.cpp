/**
 * @file glfw_input.cpp
 * @brief GLFW → InputManager Callback-Verdrahtung.
 */
#include <aether/input/input_manager.hpp>
#include <aether/core/logger.hpp>

#if defined(AETHER_WITH_GLFW)
#  include <GLFW/glfw3.h>
#endif

namespace aether::input {
namespace {

#if defined(AETHER_WITH_GLFW)

Key glfw_key_to_aether(int key) {
    if (key >= GLFW_KEY_A && key <= GLFW_KEY_Z) {
        return static_cast<Key>(static_cast<int>(Key::A) + (key - GLFW_KEY_A));
    }
    if (key >= GLFW_KEY_0 && key <= GLFW_KEY_9) {
        return static_cast<Key>(static_cast<int>(Key::Num0) + (key - GLFW_KEY_0));
    }
    if (key >= GLFW_KEY_F1 && key <= GLFW_KEY_F12) {
        return static_cast<Key>(static_cast<int>(Key::F1) + (key - GLFW_KEY_F1));
    }
    switch (key) {
    case GLFW_KEY_SPACE: return Key::Space;
    case GLFW_KEY_ESCAPE: return Key::Escape;
    case GLFW_KEY_ENTER: return Key::Enter;
    case GLFW_KEY_TAB: return Key::Tab;
    case GLFW_KEY_BACKSPACE: return Key::Backspace;
    case GLFW_KEY_INSERT: return Key::Insert;
    case GLFW_KEY_DELETE: return Key::Delete;
    case GLFW_KEY_RIGHT: return Key::Right;
    case GLFW_KEY_LEFT: return Key::Left;
    case GLFW_KEY_DOWN: return Key::Down;
    case GLFW_KEY_UP: return Key::Up;
    case GLFW_KEY_PAGE_UP: return Key::PageUp;
    case GLFW_KEY_PAGE_DOWN: return Key::PageDown;
    case GLFW_KEY_HOME: return Key::Home;
    case GLFW_KEY_END: return Key::End;
    case GLFW_KEY_LEFT_SHIFT: return Key::LeftShift;
    case GLFW_KEY_LEFT_CONTROL: return Key::LeftControl;
    case GLFW_KEY_LEFT_ALT: return Key::LeftAlt;
    case GLFW_KEY_RIGHT_SHIFT: return Key::RightShift;
    case GLFW_KEY_RIGHT_CONTROL: return Key::RightControl;
    case GLFW_KEY_RIGHT_ALT: return Key::RightAlt;
    case GLFW_KEY_MINUS: return Key::Minus;
    case GLFW_KEY_EQUAL: return Key::Equal;
    case GLFW_KEY_COMMA: return Key::Comma;
    case GLFW_KEY_PERIOD: return Key::Period;
    case GLFW_KEY_SLASH: return Key::Slash;
    case GLFW_KEY_SEMICOLON: return Key::Semicolon;
    case GLFW_KEY_APOSTROPHE: return Key::Apostrophe;
    case GLFW_KEY_LEFT_BRACKET: return Key::LeftBracket;
    case GLFW_KEY_RIGHT_BRACKET: return Key::RightBracket;
    case GLFW_KEY_BACKSLASH: return Key::Backslash;
    case GLFW_KEY_GRAVE_ACCENT: return Key::GraveAccent;
    default: return Key::Unknown;
    }
}

Modifiers glfw_mods(int mods) {
    Modifiers m;
    m.shift = (mods & GLFW_MOD_SHIFT) != 0;
    m.control = (mods & GLFW_MOD_CONTROL) != 0;
    m.alt = (mods & GLFW_MOD_ALT) != 0;
    m.super = (mods & GLFW_MOD_SUPER) != 0;
    return m;
}

Action glfw_action(int action) {
    if (action == GLFW_PRESS) return Action::Press;
    if (action == GLFW_RELEASE) return Action::Release;
    return Action::Repeat;
}

InputManager* g_bound_input = nullptr;

void on_key(GLFWwindow*, int key, int /*scancode*/, int action, int mods) {
    if (!g_bound_input) return;
    const Key k = glfw_key_to_aether(key);
    if (k == Key::Unknown) return;
    g_bound_input->feed_key(k, glfw_action(action), glfw_mods(mods));
}

void on_mouse_button(GLFWwindow*, int button, int action, int mods) {
    if (!g_bound_input) return;
    MouseButton b = MouseButton::Left;
    if (button == GLFW_MOUSE_BUTTON_RIGHT) b = MouseButton::Right;
    else if (button == GLFW_MOUSE_BUTTON_MIDDLE) b = MouseButton::Middle;
    else if (button == GLFW_MOUSE_BUTTON_4) b = MouseButton::Button4;
    else if (button == GLFW_MOUSE_BUTTON_5) b = MouseButton::Button5;
    else if (button != GLFW_MOUSE_BUTTON_LEFT) return;
    g_bound_input->feed_mouse_button(b, glfw_action(action), glfw_mods(mods));
}

void on_cursor(GLFWwindow*, double x, double y) {
    if (!g_bound_input) return;
    g_bound_input->feed_mouse_move(x, y);
}

void on_scroll(GLFWwindow*, double xoff, double yoff) {
    if (!g_bound_input) return;
    g_bound_input->feed_scroll(xoff, yoff);
}

void on_char(GLFWwindow*, unsigned int codepoint) {
    if (!g_bound_input) return;
    g_bound_input->feed_text(codepoint);
}

#endif // AETHER_WITH_GLFW

} // namespace

// Implemented in this TU to keep GLFW out of the main cpp when disabled.
void input_attach_glfw_callbacks(InputManager* input, void* native_glfw_window) {
#if defined(AETHER_WITH_GLFW)
    g_bound_input = input;
    auto* win = static_cast<GLFWwindow*>(native_glfw_window);
    if (!win) {
        return;
    }
    glfwSetKeyCallback(win, on_key);
    glfwSetMouseButtonCallback(win, on_mouse_button);
    glfwSetCursorPosCallback(win, on_cursor);
    glfwSetScrollCallback(win, on_scroll);
    glfwSetCharCallback(win, on_char);
    core::log_info("Input", "GLFW callbacks attached");
#else
    (void)input;
    (void)native_glfw_window;
#endif
}

void input_detach_glfw_callbacks(void* native_glfw_window) {
#if defined(AETHER_WITH_GLFW)
    g_bound_input = nullptr;
    auto* win = static_cast<GLFWwindow*>(native_glfw_window);
    if (!win) return;
    glfwSetKeyCallback(win, nullptr);
    glfwSetMouseButtonCallback(win, nullptr);
    glfwSetCursorPosCallback(win, nullptr);
    glfwSetScrollCallback(win, nullptr);
    glfwSetCharCallback(win, nullptr);
#else
    (void)native_glfw_window;
#endif
}

} // namespace aether::input
