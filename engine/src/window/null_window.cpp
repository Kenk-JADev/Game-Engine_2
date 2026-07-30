/**
 * @file null_window.cpp
 */
#include "null_window.hpp"

#include <aether/core/logger.hpp>

namespace aether::window {

NullWindow::NullWindow(WindowDesc desc, core::EventBus* events)
    : Window(WindowBackend::Null, std::move(desc), events) {
    title_ = desc_.title;
    width_ = desc_.width > 0 ? desc_.width : 1280;
    height_ = desc_.height > 0 ? desc_.height : 720;
    fb_width_ = width_;
    fb_height_ = height_;
    vsync_ = desc_.vsync;
    fullscreen_ = desc_.fullscreen;
    open_ = true;
    core::log_info("Window", "NullWindow created " + std::to_string(width_) + "x" +
                                 std::to_string(height_) + " (headless)");
}

NullWindow::~NullWindow() {
    backend_destroy();
}

void NullWindow::backend_poll_events() {
    // keine OS-Events
}

void NullWindow::backend_swap_buffers() {
    // kein Swap
}

void NullWindow::backend_make_context_current() {
    // kein GL-Kontext
}

void NullWindow::backend_set_vsync(bool enabled) {
    vsync_ = enabled;
}

void NullWindow::backend_set_title(const std::string& title) {
    title_ = title;
}

void NullWindow::backend_set_size(i32 w, i32 h) {
    if (w <= 0 || h <= 0) {
        return;
    }
    notify_resize(w, h);
    notify_framebuffer_resize(w, h);
}

void NullWindow::backend_set_fullscreen(bool fs) {
    fullscreen_ = fs;
}

bool NullWindow::backend_should_close() const {
    return should_close_;
}

void NullWindow::backend_set_should_close(bool v) {
    should_close_ = v;
    if (v) {
        notify_close();
    }
}

void* NullWindow::backend_native_handle() const {
    return nullptr;
}

void NullWindow::backend_destroy() {
    if (!open_) {
        return;
    }
    open_ = false;
    core::log_debug("Window", "NullWindow destroyed");
}

} // namespace aether::window
