/**
 * @file window.hpp
 * @brief Plattformfenster und OpenGL-Kontext-Hülle.
 *
 * Backend:
 *  - GLFW  (Desktop, wenn AETHER_WITH_GLFW)
 *  - Null  (Headless / Tests / fehlende GUI-Deps)
 *
 * Der Anwender (Editor-Autor) sieht diese Klasse nicht – nur die Engine.
 */
#pragma once

#include <aether/core/config.hpp>
#include <aether/core/event_bus.hpp>
#include <aether/core/types.hpp>
#include <aether/window/window_events.hpp>

#include <memory>
#include <optional>
#include <string>

namespace aether::window {

/** @brief Welches Backend aktiv ist. */
enum class WindowBackend {
    Null, ///< Kein OS-Fenster (Headless)
    Glfw, ///< GLFW + OpenGL-Kontext
};

/**
 * @brief Erstellungsparameter für ein Fenster.
 */
struct WindowDesc {
    std::string title        = "AetherRPG";
    i32  width               = 1280;
    i32  height              = 720;
    bool fullscreen          = false;
    bool vsync               = true;
    bool resizable           = true;
    bool visible             = true;
    /** OpenGL 3.3 Core – Ziel Intel HD 4600 */
    i32  gl_major            = 3;
    i32  gl_minor            = 3;
    bool gl_forward_compat   = true;
    bool gl_debug_context    = false;
};

/**
 * @brief Abstraktes Anwendungsfenster.
 *
 * RAII: Destruktor schließt das OS-Fenster.
 * OpenGL-Kontext ist nach create() aktuell (make_context_current).
 */
class Window : public aether::NonMovable {
public:
    /**
     * @brief Erzeugt ein Fenster.
     * @param desc   Parameter
     * @param events Optionaler EventBus für Resize/Close/…
     * @param force_backend  Wenn gesetzt, erzwingt Backend (sonst Auto)
     */
    [[nodiscard]] static std::unique_ptr<Window> create(
        const WindowDesc& desc,
        core::EventBus* events = nullptr,
        std::optional<WindowBackend> force_backend = std::nullopt);

    /**
     * @brief Baut WindowDesc aus GraphicsConfig.
     */
    [[nodiscard]] static WindowDesc desc_from_graphics(const core::GraphicsConfig& g);

    virtual ~Window();

    [[nodiscard]] WindowBackend backend() const noexcept { return backend_; }
    [[nodiscard]] bool is_open() const noexcept { return open_; }
    [[nodiscard]] bool should_close() const;
    void set_should_close(bool value);

    void poll_events();
    void swap_buffers();

    void make_context_current();
    void set_vsync(bool enabled);
    [[nodiscard]] bool vsync() const noexcept { return vsync_; }

    void set_title(std::string title);
    [[nodiscard]] const std::string& title() const noexcept { return title_; }

    void set_size(i32 width, i32 height);
    [[nodiscard]] i32 width() const noexcept { return width_; }
    [[nodiscard]] i32 height() const noexcept { return height_; }
    [[nodiscard]] i32 framebuffer_width() const noexcept { return fb_width_; }
    [[nodiscard]] i32 framebuffer_height() const noexcept { return fb_height_; }

    void set_fullscreen(bool fullscreen);
    [[nodiscard]] bool fullscreen() const noexcept { return fullscreen_; }

    /** @brief Natives Handle (GLFWwindow* oder nullptr). */
    [[nodiscard]] void* native_handle() const noexcept;

    void set_event_bus(core::EventBus* events) noexcept { events_ = events; }

protected:
    Window(WindowBackend backend, WindowDesc desc, core::EventBus* events);

    virtual void backend_poll_events() = 0;
    virtual void backend_swap_buffers() = 0;
    virtual void backend_make_context_current() = 0;
    virtual void backend_set_vsync(bool enabled) = 0;
    virtual void backend_set_title(const std::string& title) = 0;
    virtual void backend_set_size(i32 w, i32 h) = 0;
    virtual void backend_set_fullscreen(bool fs) = 0;
    virtual bool backend_should_close() const = 0;
    virtual void backend_set_should_close(bool v) = 0;
    virtual void* backend_native_handle() const = 0;
    virtual void backend_destroy() = 0;

    void notify_resize(i32 w, i32 h);
    void notify_framebuffer_resize(i32 w, i32 h);
    void notify_close();
    void notify_focus(bool focused);

    WindowBackend backend_;
    WindowDesc desc_;
    core::EventBus* events_ = nullptr;

    std::string title_;
    i32 width_ = 0;
    i32 height_ = 0;
    i32 fb_width_ = 0;
    i32 fb_height_ = 0;
    bool vsync_ = true;
    bool fullscreen_ = false;
    bool open_ = false;
};

/**
 * @brief Einmalige Plattform-Init/Terminate (GLFW).
 *
 * Wird intern von Window::create verwaltet.
 */
class WindowSystem : public aether::NonMovable {
public:
    static Result<void> initialize(WindowBackend backend);
    static void terminate();
    [[nodiscard]] static bool initialized() noexcept;
    [[nodiscard]] static WindowBackend active_backend() noexcept;
};

} // namespace aether::window
