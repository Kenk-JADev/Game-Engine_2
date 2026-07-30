/**
 * @file window_test.cpp
 * @brief Tests für NullWindow / Window-API (ohne Display).
 */
#include <aether/window/window_module.hpp>
#include <aether/core/core.hpp>

#include <cstdio>
#include <string>

using namespace aether;
using namespace aether::core;
using namespace aether::window;

static int g_failures = 0;

#define CHECK(cond)                                                            \
    do {                                                                       \
        if (!(cond)) {                                                         \
            std::fprintf(stderr, "CHECK failed: %s (%s:%d)\n", #cond,         \
                         __FILE__, __LINE__);                                  \
            ++g_failures;                                                      \
        }                                                                      \
    } while (0)

int main() {
    EngineConfig cfg;
    cfg.mode = AppMode::Headless;
    cfg.log.console = false;
    cfg.log.file = false;
    auto ctx = EngineContext::create(std::move(cfg));
    ctx->start();

    int resize_count = 0;
    int close_count = 0;
    ctx->events().subscribe<WindowResizeEvent>([&](const WindowResizeEvent& e) {
        CHECK(e.width > 0);
        CHECK(e.height > 0);
        ++resize_count;
    });
    ctx->events().subscribe<WindowCloseEvent>([&](const WindowCloseEvent&) {
        ++close_count;
    });

    WindowDesc desc = Window::desc_from_graphics(ctx->config().graphics);
    desc.title = "WindowTest";
    desc.width = 640;
    desc.height = 360;

    auto window = Window::create(desc, &ctx->events(), WindowBackend::Null);
    CHECK(window != nullptr);
    CHECK(window->backend() == WindowBackend::Null);
    CHECK(window->is_open());
    CHECK(window->width() == 640);
    CHECK(window->height() == 360);
    CHECK(window->title() == "WindowTest");
    CHECK(!window->should_close());

    window->set_title("Renamed");
    CHECK(window->title() == "Renamed");

    window->set_size(800, 600);
    CHECK(window->width() == 800);
    CHECK(window->height() == 600);
    CHECK(resize_count >= 1);

    window->set_vsync(false);
    CHECK(window->vsync() == false);
    window->set_vsync(true);

    window->poll_events();
    window->swap_buffers();
    window->make_context_current();
    CHECK(window->native_handle() == nullptr);

    window->set_should_close(true);
    CHECK(window->should_close());
    CHECK(close_count >= 1);

    window.reset();
    WindowSystem::terminate();

    ctx->shutdown();

    if (g_failures == 0) {
        std::puts("OK: window_test passed");
        return 0;
    }
    std::fprintf(stderr, "FAIL: %d check(s) failed\n", g_failures);
    return 1;
}
