/**
 * @file glfw_window.hpp
 * @brief GLFW-basiertes Desktop-Fenster (nur wenn AETHER_WITH_GLFW).
 */
#pragma once

#include <aether/window/window.hpp>

#if defined(AETHER_WITH_GLFW)

struct GLFWwindow;

namespace aether::window {

class GlfwWindow final : public Window {
public:
    static std::unique_ptr<GlfwWindow> try_create(WindowDesc desc, core::EventBus* events);

    ~GlfwWindow() override;

protected:
    void backend_poll_events() override;
    void backend_swap_buffers() override;
    void backend_make_context_current() override;
    void backend_set_vsync(bool enabled) override;
    void backend_set_title(const std::string& title) override;
    void backend_set_size(i32 w, i32 h) override;
    void backend_set_fullscreen(bool fs) override;
    bool backend_should_close() const override;
    void backend_set_should_close(bool v) override;
    void* backend_native_handle() const override;
    void backend_destroy() override;

private:
    GlfwWindow(WindowDesc desc, core::EventBus* events, GLFWwindow* handle);

    static void on_framebuffer_size(GLFWwindow* w, int width, int height);
    static void on_window_size(GLFWwindow* w, int width, int height);
    static void on_window_close(GLFWwindow* w);
    static void on_window_focus(GLFWwindow* w, int focused);
    static void on_window_content_scale(GLFWwindow* w, float xscale, float yscale);

    GLFWwindow* handle_ = nullptr;
    i32 windowed_x_ = 100;
    i32 windowed_y_ = 100;
    i32 windowed_w_ = 1280;
    i32 windowed_h_ = 720;
};

} // namespace aether::window

#endif // AETHER_WITH_GLFW
