/**
 * @file glfw_window.cpp
 */
#include "glfw_window.hpp"

#if defined(AETHER_WITH_GLFW)

#include <aether/core/logger.hpp>

#include <GLFW/glfw3.h>

#include <string>

namespace aether::window {
namespace {

void glfw_error_callback(int code, const char* description) {
    core::log_error("GLFW", std::string("[") + std::to_string(code) + "] " +
                                (description ? description : ""));
}

GlfwWindow* from_native(GLFWwindow* w) {
    return static_cast<GlfwWindow*>(glfwGetWindowUserPointer(w));
}

} // namespace

std::unique_ptr<GlfwWindow> GlfwWindow::try_create(WindowDesc desc, core::EventBus* events) {
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, desc.gl_major);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, desc.gl_minor);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#if defined(__APPLE__)
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, desc.gl_forward_compat ? GLFW_TRUE : GLFW_FALSE);
#else
    // Auf Desktop-GL optional; HD 4600 unterstützt Core 3.3 ohne Forward-Compat-Pflicht
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, desc.gl_forward_compat ? GLFW_TRUE : GLFW_FALSE);
#endif
    glfwWindowHint(GLFW_RESIZABLE, desc.resizable ? GLFW_TRUE : GLFW_FALSE);
    glfwWindowHint(GLFW_VISIBLE, desc.visible ? GLFW_TRUE : GLFW_FALSE);
    glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);
#if defined(AETHER_DEBUG)
    if (desc.gl_debug_context) {
        glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
    }
#endif

    GLFWmonitor* monitor = nullptr;
    int w = desc.width > 0 ? desc.width : 1280;
    int h = desc.height > 0 ? desc.height : 720;

    if (desc.fullscreen) {
        monitor = glfwGetPrimaryMonitor();
        if (monitor) {
            const GLFWvidmode* mode = glfwGetVideoMode(monitor);
            if (mode) {
                w = mode->width;
                h = mode->height;
                glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);
            }
        }
    }

    GLFWwindow* handle = glfwCreateWindow(w, h, desc.title.c_str(), monitor, nullptr);
    if (!handle) {
        core::log_error("Window", "glfwCreateWindow failed");
        return nullptr;
    }

    auto window = std::unique_ptr<GlfwWindow>(new GlfwWindow(std::move(desc), events, handle));
    return window;
}

GlfwWindow::GlfwWindow(WindowDesc desc, core::EventBus* events, GLFWwindow* handle)
    : Window(WindowBackend::Glfw, std::move(desc), events)
    , handle_(handle) {
    title_ = desc_.title;
    vsync_ = desc_.vsync;
    fullscreen_ = desc_.fullscreen;
    open_ = true;

    glfwSetWindowUserPointer(handle_, this);
    glfwSetFramebufferSizeCallback(handle_, &GlfwWindow::on_framebuffer_size);
    glfwSetWindowSizeCallback(handle_, &GlfwWindow::on_window_size);
    glfwSetWindowCloseCallback(handle_, &GlfwWindow::on_window_close);
    glfwSetWindowFocusCallback(handle_, &GlfwWindow::on_window_focus);
    glfwSetWindowContentScaleCallback(handle_, &GlfwWindow::on_window_content_scale);

    glfwMakeContextCurrent(handle_);
    glfwSwapInterval(vsync_ ? 1 : 0);

    glfwGetWindowSize(handle_, &width_, &height_);
    glfwGetFramebufferSize(handle_, &fb_width_, &fb_height_);
    windowed_w_ = width_;
    windowed_h_ = height_;
    glfwGetWindowPos(handle_, &windowed_x_, &windowed_y_);

    core::log_info("Window", "GLFW window created " + std::to_string(width_) + "x" +
                                 std::to_string(height_) +
                                 " (fb " + std::to_string(fb_width_) + "x" +
                                 std::to_string(fb_height_) + ")");
}

GlfwWindow::~GlfwWindow() {
    backend_destroy();
}

void GlfwWindow::backend_poll_events() {
    glfwPollEvents();
}

void GlfwWindow::backend_swap_buffers() {
    if (handle_) {
        glfwSwapBuffers(handle_);
    }
}

void GlfwWindow::backend_make_context_current() {
    if (handle_) {
        glfwMakeContextCurrent(handle_);
    }
}

void GlfwWindow::backend_set_vsync(bool enabled) {
    vsync_ = enabled;
    if (handle_) {
        glfwMakeContextCurrent(handle_);
        glfwSwapInterval(enabled ? 1 : 0);
    }
}

void GlfwWindow::backend_set_title(const std::string& title) {
    title_ = title;
    if (handle_) {
        glfwSetWindowTitle(handle_, title_.c_str());
    }
}

void GlfwWindow::backend_set_size(i32 w, i32 h) {
    if (!handle_ || w <= 0 || h <= 0) {
        return;
    }
    glfwSetWindowSize(handle_, w, h);
}

void GlfwWindow::backend_set_fullscreen(bool fs) {
    if (!handle_ || fs == fullscreen_) {
        return;
    }
    if (fs) {
        glfwGetWindowPos(handle_, &windowed_x_, &windowed_y_);
        glfwGetWindowSize(handle_, &windowed_w_, &windowed_h_);
        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = monitor ? glfwGetVideoMode(monitor) : nullptr;
        if (monitor && mode) {
            glfwSetWindowMonitor(handle_, monitor, 0, 0, mode->width, mode->height,
                                 mode->refreshRate);
            fullscreen_ = true;
        }
    } else {
        glfwSetWindowMonitor(handle_, nullptr, windowed_x_, windowed_y_, windowed_w_,
                             windowed_h_, 0);
        fullscreen_ = false;
    }
    backend_set_vsync(vsync_);
}

bool GlfwWindow::backend_should_close() const {
    return handle_ ? (glfwWindowShouldClose(handle_) == GLFW_TRUE) : true;
}

void GlfwWindow::backend_set_should_close(bool v) {
    if (handle_) {
        glfwSetWindowShouldClose(handle_, v ? GLFW_TRUE : GLFW_FALSE);
    }
}

void* GlfwWindow::backend_native_handle() const {
    return handle_;
}

void GlfwWindow::backend_destroy() {
    if (!handle_) {
        open_ = false;
        return;
    }
    glfwDestroyWindow(handle_);
    handle_ = nullptr;
    open_ = false;
    core::log_debug("Window", "GLFW window destroyed");
}

void GlfwWindow::on_framebuffer_size(GLFWwindow* w, int width, int height) {
    if (auto* self = from_native(w)) {
        self->notify_framebuffer_resize(width, height);
    }
}

void GlfwWindow::on_window_size(GLFWwindow* w, int width, int height) {
    if (auto* self = from_native(w)) {
        self->notify_resize(width, height);
    }
}

void GlfwWindow::on_window_close(GLFWwindow* w) {
    if (auto* self = from_native(w)) {
        self->notify_close();
    }
}

void GlfwWindow::on_window_focus(GLFWwindow* w, int focused) {
    if (auto* self = from_native(w)) {
        self->notify_focus(focused == GLFW_TRUE);
    }
}

void GlfwWindow::on_window_content_scale(GLFWwindow* w, float xscale, float yscale) {
    if (auto* self = from_native(w)) {
        if (self->events_) {
            self->events_->publish(WindowContentScaleEvent{xscale, yscale});
        }
    }
}

// Wird von WindowSystem genutzt
void glfw_install_error_callback() {
    glfwSetErrorCallback(glfw_error_callback);
}

} // namespace aether::window

#endif // AETHER_WITH_GLFW
