/**
 * @file input_manager.hpp
 * @brief Action-Mapping und Abfrage von Tastatur/Maus/Gamepad.
 *
 * RPG-Maker-typische Abfragen:
 *   Input.press?(:C)  → is_down(ActionName)
 *   Input.trigger?(:B) → was_pressed(ActionName)
 */
#pragma once

#include <aether/core/event_bus.hpp>
#include <aether/core/types.hpp>
#include <aether/input/input_events.hpp>
#include <aether/input/keys.hpp>
#include <aether/window/window.hpp>

#include <array>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace aether::input {

/**
 * @brief Eine logische Aktion (z. B. "confirm", "cancel", "up").
 */
struct InputAction {
    std::string name;
    std::vector<Key> keys;
    std::vector<MouseButton> mouse_buttons;
    std::vector<GamepadButton> gamepad_buttons;
};

/**
 * @brief Zustand eines digitalen Buttons über Frames.
 */
struct ButtonState {
    bool down = false;
    bool pressed = false;  ///< rising edge dieses Frames
    bool released = false; ///< falling edge dieses Frames
};

/**
 * @brief Zentraler Input-Dienst.
 *
 * Aufrufreihenfolge pro Frame:
 *   1. window.poll_events()   (liefert OS-Events)
 *   2. input.begin_frame()
 *   3. input.feed_* / inject  (oder automatische Anbindung)
 *   4. Spiel-Logik liest is_down / was_pressed
 *   5. input.end_frame()
 */
class InputManager : public aether::NonMovable {
public:
    explicit InputManager(core::EventBus* events = nullptr);
    ~InputManager();

    void set_event_bus(core::EventBus* events) noexcept { events_ = events; }

    /**
     * @brief Bindet an ein Fenster (GLFW-Callbacks wenn verfügbar).
     */
    void attach_window(window::Window* window);

    void detach_window();

    // ---- Frame-Lebenszyklus -------------------------------------------------

    /** @brief Löscht edge-Flags, übernimmt last→current. */
    void begin_frame();

    /** @brief Schließt Frame ab (Scroll-Deltas etc. zurücksetzen). */
    void end_frame();

    // ---- Manuelles Feed (Tests / Null-Backend) ------------------------------

    void feed_key(Key key, Action action, Modifiers mods = {});
    void feed_mouse_button(MouseButton button, Action action, Modifiers mods = {});
    void feed_mouse_move(f64 x, f64 y);
    void feed_scroll(f64 x_off, f64 y_off);
    void feed_text(u32 codepoint);

    // ---- Rohe Abfragen ------------------------------------------------------

    [[nodiscard]] bool is_key_down(Key key) const noexcept;
    [[nodiscard]] bool was_key_pressed(Key key) const noexcept;
    [[nodiscard]] bool was_key_released(Key key) const noexcept;

    [[nodiscard]] bool is_mouse_down(MouseButton b) const noexcept;
    [[nodiscard]] bool was_mouse_pressed(MouseButton b) const noexcept;
    [[nodiscard]] bool was_mouse_released(MouseButton b) const noexcept;

    [[nodiscard]] f64 mouse_x() const noexcept { return mouse_x_; }
    [[nodiscard]] f64 mouse_y() const noexcept { return mouse_y_; }
    [[nodiscard]] f64 mouse_dx() const noexcept { return mouse_dx_; }
    [[nodiscard]] f64 mouse_dy() const noexcept { return mouse_dy_; }
    [[nodiscard]] f64 scroll_x() const noexcept { return scroll_x_; }
    [[nodiscard]] f64 scroll_y() const noexcept { return scroll_y_; }

    // ---- Action-Mapping -----------------------------------------------------

    void register_action(InputAction action);
    void clear_actions();

    /** @brief RPG-Maker-ähnlich: gehalten. */
    [[nodiscard]] bool is_down(std::string_view action) const;

    /** @brief RPG-Maker-ähnlich: gerade gedrückt (Trigger). */
    [[nodiscard]] bool was_pressed(std::string_view action) const;

    /** @brief Gerade losgelassen. */
    [[nodiscard]] bool was_released(std::string_view action) const;

    /**
     * @brief Standard-Mapping im Stil klassischer RPG Maker:
     *  confirm/Z/Space/Enter, cancel/X/Escape, menu/Shift,
     *  up/down/left/right + WASD
     */
    void register_default_rpg_actions();

private:
    void update_button(ButtonState& state, bool down);

    core::EventBus* events_ = nullptr;
    window::Window* window_ = nullptr;

    std::array<ButtonState, static_cast<usize>(Key::Count)> keys_{};
    std::array<ButtonState, static_cast<usize>(MouseButton::Count)> mouse_{};

    f64 mouse_x_ = 0.0;
    f64 mouse_y_ = 0.0;
    f64 mouse_dx_ = 0.0;
    f64 mouse_dy_ = 0.0;
    f64 scroll_x_ = 0.0;
    f64 scroll_y_ = 0.0;
    bool mouse_initialized_ = false;

    std::unordered_map<std::string, InputAction> actions_;
};

} // namespace aether::input
