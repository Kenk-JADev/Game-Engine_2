/**
 * @file input_manager.cpp
 */
#include <aether/input/input_manager.hpp>
#include <aether/core/logger.hpp>

#include <algorithm>

namespace aether::input {

// from glfw_input.cpp
void input_attach_glfw_callbacks(InputManager* input, void* native_glfw_window);
void input_detach_glfw_callbacks(void* native_glfw_window);

namespace {

usize key_index(Key k) {
    const auto i = static_cast<i32>(k);
    if (i < 0 || i >= static_cast<i32>(Key::Count)) {
        return static_cast<usize>(Key::Count) - 1;
    }
    return static_cast<usize>(i);
}

usize mouse_index(MouseButton b) {
    const auto i = static_cast<i32>(b);
    if (i < 0 || i >= static_cast<i32>(MouseButton::Count)) {
        return 0;
    }
    return static_cast<usize>(i);
}

} // namespace

InputManager::InputManager(core::EventBus* events) : events_(events) {}

InputManager::~InputManager() {
    detach_window();
}

void InputManager::attach_window(window::Window* window) {
    detach_window();
    window_ = window;
    if (!window_) {
        return;
    }
    if (window_->backend() == window::WindowBackend::Glfw && window_->native_handle()) {
        input_attach_glfw_callbacks(this, window_->native_handle());
    } else {
        core::log_debug("Input", "Attached without GLFW callbacks (feed_* mode)");
    }
}

void InputManager::detach_window() {
    if (window_ && window_->backend() == window::WindowBackend::Glfw &&
        window_->native_handle()) {
        input_detach_glfw_callbacks(window_->native_handle());
    }
    window_ = nullptr;
}

void InputManager::update_button(ButtonState& state, bool down) {
    if (down && !state.down) {
        state.pressed = true;
    } else if (!down && state.down) {
        state.released = true;
    }
    state.down = down;
}

void InputManager::begin_frame() {
    for (auto& k : keys_) {
        k.pressed = false;
        k.released = false;
    }
    for (auto& m : mouse_) {
        m.pressed = false;
        m.released = false;
    }
    for (auto& p : pad_) {
        p.pressed = false;
        p.released = false;
    }
    mouse_dx_ = 0.0;
    mouse_dy_ = 0.0;
    scroll_x_ = 0.0;
    scroll_y_ = 0.0;
    axis_lx_ = 0.0f;
    axis_ly_ = 0.0f;
}

void InputManager::end_frame() {
    // derzeit nichts zusätzliches
}

void InputManager::poll_gamepads() {
#if defined(AETHER_WITH_GLFW)
    extern void input_poll_gamepads(
        f32 deadzone, f32& lx, f32& ly,
        std::array<ButtonState, static_cast<usize>(GamepadButton::Count)>& pad,
        void (*update_btn)(ButtonState&, bool));
    // Use member update via lambda bridge
    auto* self = this;
    auto bridge = [](ButtonState& st, bool down) {
        // free function can't call member; do inline edge logic
        if (down && !st.down) st.pressed = true;
        else if (!down && st.down) st.released = true;
        st.down = down;
    };
    (void)self;
    input_poll_gamepads(stick_deadzone_, axis_lx_, axis_ly_, pad_, bridge);

    // Stick as digital D-pad (OR with physical dpad already in pad_)
    const bool su = axis_ly_ < -stick_deadzone_;
    const bool sd = axis_ly_ > stick_deadzone_;
    const bool sl = axis_lx_ < -stick_deadzone_;
    const bool sr = axis_lx_ > stick_deadzone_;
    if (su) update_button(pad_[static_cast<usize>(GamepadButton::DpadUp)], true);
    if (sd) update_button(pad_[static_cast<usize>(GamepadButton::DpadDown)], true);
    if (sl) update_button(pad_[static_cast<usize>(GamepadButton::DpadLeft)], true);
    if (sr) update_button(pad_[static_cast<usize>(GamepadButton::DpadRight)], true);
#else
    axis_lx_ = 0.0f;
    axis_ly_ = 0.0f;
#endif
}

void InputManager::feed_key(Key key, Action action, Modifiers mods) {
    if (key == Key::Unknown) {
        return;
    }
    const bool down = (action == Action::Press || action == Action::Repeat);
    // Repeat soll kein neues pressed feuern, wenn bereits down
    auto& st = keys_[key_index(key)];
    if (action == Action::Repeat) {
        st.down = true;
    } else {
        update_button(st, down);
    }
    if (events_) {
        events_->publish(KeyEvent{key, action, mods, 0});
    }
}

void InputManager::feed_mouse_button(MouseButton button, Action action, Modifiers mods) {
    const bool down = (action == Action::Press);
    update_button(mouse_[mouse_index(button)], down);
    if (events_) {
        events_->publish(MouseButtonEvent{button, action, mods, mouse_x_, mouse_y_});
    }
}

void InputManager::feed_mouse_move(f64 x, f64 y) {
    if (mouse_initialized_) {
        mouse_dx_ += (x - mouse_x_);
        mouse_dy_ += (y - mouse_y_);
    }
    mouse_x_ = x;
    mouse_y_ = y;
    mouse_initialized_ = true;
    if (events_) {
        events_->publish(MouseMoveEvent{x, y, mouse_dx_, mouse_dy_});
    }
}

void InputManager::feed_scroll(f64 x_off, f64 y_off) {
    scroll_x_ += x_off;
    scroll_y_ += y_off;
    if (events_) {
        events_->publish(MouseScrollEvent{x_off, y_off});
    }
}

void InputManager::feed_text(u32 codepoint) {
    if (events_) {
        events_->publish(TextInputEvent{codepoint});
    }
}

bool InputManager::is_key_down(Key key) const noexcept {
    return keys_[key_index(key)].down;
}

bool InputManager::was_key_pressed(Key key) const noexcept {
    return keys_[key_index(key)].pressed;
}

bool InputManager::was_key_released(Key key) const noexcept {
    return keys_[key_index(key)].released;
}

bool InputManager::is_mouse_down(MouseButton b) const noexcept {
    return mouse_[mouse_index(b)].down;
}

bool InputManager::was_mouse_pressed(MouseButton b) const noexcept {
    return mouse_[mouse_index(b)].pressed;
}

bool InputManager::was_mouse_released(MouseButton b) const noexcept {
    return mouse_[mouse_index(b)].released;
}

void InputManager::register_action(InputAction action) {
    const std::string key = action.name;
    actions_[key] = std::move(action);
}

void InputManager::clear_actions() {
    actions_.clear();
}

bool InputManager::is_down(std::string_view action) const {
    const auto it = actions_.find(std::string(action));
    if (it == actions_.end()) {
        return false;
    }
    const auto& a = it->second;
    for (Key k : a.keys) {
        if (is_key_down(k)) return true;
    }
    for (MouseButton b : a.mouse_buttons) {
        if (is_mouse_down(b)) return true;
    }
    for (GamepadButton b : a.gamepad_buttons) {
        const auto i = static_cast<usize>(b);
        if (i < pad_.size() && pad_[i].down) return true;
    }
    return false;
}

bool InputManager::was_pressed(std::string_view action) const {
    const auto it = actions_.find(std::string(action));
    if (it == actions_.end()) {
        return false;
    }
    const auto& a = it->second;
    for (Key k : a.keys) {
        if (was_key_pressed(k)) return true;
    }
    for (MouseButton b : a.mouse_buttons) {
        if (was_mouse_pressed(b)) return true;
    }
    for (GamepadButton b : a.gamepad_buttons) {
        const auto i = static_cast<usize>(b);
        if (i < pad_.size() && pad_[i].pressed) return true;
    }
    return false;
}

bool InputManager::was_released(std::string_view action) const {
    const auto it = actions_.find(std::string(action));
    if (it == actions_.end()) {
        return false;
    }
    const auto& a = it->second;
    for (Key k : a.keys) {
        if (was_key_released(k)) return true;
    }
    for (MouseButton b : a.mouse_buttons) {
        if (was_mouse_released(b)) return true;
    }
    for (GamepadButton b : a.gamepad_buttons) {
        const auto i = static_cast<usize>(b);
        if (i < pad_.size() && pad_[i].released) return true;
    }
    return false;
}

void InputManager::register_default_rpg_actions() {
    register_action(InputAction{
        "confirm",
        {Key::Z, Key::Space, Key::Enter, Key::C},
        {MouseButton::Left},
        {GamepadButton::A},
    });
    register_action(InputAction{
        "cancel",
        {Key::X, Key::Escape, Key::Num0},
        {MouseButton::Right},
        {GamepadButton::B},
    });
    register_action(InputAction{
        "menu",
        {Key::LeftShift, Key::RightShift, Key::V},
        {},
        {GamepadButton::Y},
    });
    register_action(InputAction{
        "up",
        {Key::Up, Key::W},
        {},
        {GamepadButton::DpadUp},
    });
    register_action(InputAction{
        "down",
        {Key::Down, Key::S},
        {},
        {GamepadButton::DpadDown},
    });
    register_action(InputAction{
        "left",
        {Key::Left, Key::A},
        {},
        {GamepadButton::DpadLeft},
    });
    register_action(InputAction{
        "right",
        {Key::Right, Key::D},
        {},
        {GamepadButton::DpadRight},
    });
    register_action(InputAction{
        "page_up",
        {Key::Q, Key::PageUp},
        {},
        {GamepadButton::LeftBumper},
    });
    register_action(InputAction{
        "page_down",
        {Key::E, Key::PageDown},
        {},
        {GamepadButton::RightBumper},
    });
    core::log_info("Input", "Default RPG actions registered");
}

} // namespace aether::input
