/**
 * @file window_events.hpp
 * @brief Events des Window-Subsystems (für EventBus).
 */
#pragma once

#include <aether/core/types.hpp>

#include <string>
#include <vector>

namespace aether::window {

struct WindowResizeEvent {
    i32 width  = 0;
    i32 height = 0;
};

struct WindowCloseEvent {};

struct WindowFocusEvent {
    bool focused = false;
};

struct WindowFramebufferResizeEvent {
    i32 width  = 0;
    i32 height = 0;
};

struct WindowContentScaleEvent {
    f32 xscale = 1.0f;
    f32 yscale = 1.0f;
};

struct WindowDropEvent {
    std::vector<std::string> paths;
};

} // namespace aether::window
