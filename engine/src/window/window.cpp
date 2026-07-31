/**
 * @file window.cpp
 * @brief Window-Basis und Factory.
 */
#include <aether/window/window.hpp>

#include "null_window.hpp"

#if defined(AETHER_WITH_GLFW)
#  include "glfw_window.hpp"
#  include <GLFW/glfw3.h>
#endif

#include <aether/core/logger.hpp>

#include <utility>

namespace aether::window {

#if defined(AETHER_WITH_GLFW)
// Externe Funktion aus glfw_window.cpp (NICHT im anonymous namespace:
// dort haette sie internal linkage und waere vom Linker nicht auffindbar).
void glfw_install_error_callback();
#endif

namespace {

bool g_initialized = false;
WindowBackend g_backend = WindowBackend::Null;
int g_init_refcount = 0;

WindowBackend choose_backend(std::optional<WindowBackend> force,
                             core::AppMode mode) {
    if (force.has_value()) {
        return *force;
    }
    if (mode == core::AppMode::Headless) {
        return WindowBackend::Null;
    }
#if defined(AETHER_WITH_GLFW)
    return WindowBackend::Glfw;
#else
    return WindowBackend::Null;
#endif
}

} // namespace

// -----------------------------------------------------------------------------
// WindowSystem
// -----------------------------------------------------------------------------

Result<void> WindowSystem::initialize(WindowBackend backend) {
    if (g_initialized) {
        if (backend != g_backend) {
            return Result<void>::fail("WindowSystem already initialized with another backend");
        }
        ++g_init_refcount;
        return Result<void>::ok();
    }

    if (backend == WindowBackend::Glfw) {
#if defined(AETHER_WITH_GLFW)
        glfw_install_error_callback();
        if (!glfwInit()) {
            return Result<void>::fail("glfwInit() failed");
        }
        core::log_info("Window", "GLFW initialized");
#else
        return Result<void>::fail("AETHER_WITH_GLFW not enabled at build time");
#endif
    } else {
        core::log_info("Window", "Null window system initialized");
    }

    g_backend = backend;
    g_initialized = true;
    g_init_refcount = 1;
    return Result<void>::ok();
}

void WindowSystem::terminate() {
    if (!g_initialized) {
        return;
    }
    --g_init_refcount;
    if (g_init_refcount > 0) {
        return;
    }
#if defined(AETHER_WITH_GLFW)
    if (g_backend == WindowBackend::Glfw) {
        glfwTerminate();
        core::log_info("Window", "GLFW terminated");
    }
#endif
    g_initialized = false;
    g_backend = WindowBackend::Null;
    g_init_refcount = 0;
}

bool WindowSystem::initialized() noexcept {
    return g_initialized;
}

WindowBackend WindowSystem::active_backend() noexcept {
    return g_backend;
}

// -----------------------------------------------------------------------------
// Window
// -----------------------------------------------------------------------------

Window::Window(WindowBackend backend, WindowDesc desc, core::EventBus* events)
    : backend_(backend)
    , desc_(std::move(desc))
    , events_(events) {}

Window::~Window() = default;

WindowDesc Window::desc_from_graphics(const core::GraphicsConfig& g) {
    WindowDesc d;
    d.title = g.title;
    d.width = g.width;
    d.height = g.height;
    d.fullscreen = g.fullscreen;
    d.vsync = g.vsync;
    d.resizable = g.resizable;
    return d;
}

std::unique_ptr<Window> Window::create(const WindowDesc& desc,
                                       core::EventBus* events,
                                       std::optional<WindowBackend> force_backend) {
    // Mode-Hinweis: wenn Headless erzwungen über force, sonst Default Glfw/Null
    const WindowBackend backend = choose_backend(force_backend, core::AppMode::Runtime);

    auto init = WindowSystem::initialize(backend);
    if (!init) {
        core::log_error("Window", init.error().what());
        // Fallback auf Null, falls GLFW scheitert
        if (backend != WindowBackend::Null) {
            core::log_warn("Window", "Falling back to NullWindow");
            auto null_init = WindowSystem::initialize(WindowBackend::Null);
            if (!null_init) {
                return nullptr;
            }
            return std::make_unique<NullWindow>(desc, events);
        }
        return nullptr;
    }

#if defined(AETHER_WITH_GLFW)
    if (backend == WindowBackend::Glfw) {
        auto gw = GlfwWindow::try_create(desc, events);
        if (gw) {
            return gw;
        }
        core::log_warn("Window", "GLFW window failed – falling back to NullWindow");
        WindowSystem::terminate();
        auto null_init = WindowSystem::initialize(WindowBackend::Null);
        if (!null_init) {
            return nullptr;
        }
        return std::make_unique<NullWindow>(desc, events);
    }
#endif

    return std::make_unique<NullWindow>(desc, events);
}

bool Window::should_close() const {
    return !open_ || backend_should_close();
}

void Window::set_should_close(bool value) {
    backend_set_should_close(value);
}

void Window::poll_events() {
    if (open_) {
        backend_poll_events();
    }
}

void Window::swap_buffers() {
    if (open_) {
        backend_swap_buffers();
    }
}

void Window::make_context_current() {
    if (open_) {
        backend_make_context_current();
    }
}

void Window::set_vsync(bool enabled) {
    backend_set_vsync(enabled);
}

void Window::set_title(std::string title) {
    backend_set_title(title);
}

void Window::set_size(i32 width, i32 height) {
    backend_set_size(width, height);
}

void Window::set_fullscreen(bool fullscreen) {
    backend_set_fullscreen(fullscreen);
}

void* Window::native_handle() const noexcept {
    return backend_native_handle();
}

void Window::notify_resize(i32 w, i32 h) {
    width_ = w;
    height_ = h;
    if (events_) {
        events_->publish(WindowResizeEvent{w, h});
    }
}

void Window::notify_framebuffer_resize(i32 w, i32 h) {
    fb_width_ = w;
    fb_height_ = h;
    if (events_) {
        events_->publish(WindowFramebufferResizeEvent{w, h});
    }
}

void Window::notify_close() {
    if (events_) {
        events_->publish(WindowCloseEvent{});
    }
}

void Window::notify_focus(bool focused) {
    if (events_) {
        events_->publish(WindowFocusEvent{focused});
    }
}

} // namespace aether::window
