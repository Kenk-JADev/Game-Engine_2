/**
 * @file null_window.hpp
 * @brief Headless-Fenster ohne OS-Window / GL-Kontext.
 */
#pragma once

#include <aether/window/window.hpp>

namespace aether::window {

class NullWindow final : public Window {
public:
    NullWindow(WindowDesc desc, core::EventBus* events);
    ~NullWindow() override;

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
    bool should_close_ = false;
};

} // namespace aether::window
