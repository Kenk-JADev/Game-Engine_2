/**
 * @file input_test.cpp
 */
#include <aether/input/input_module.hpp>
#include <aether/core/core.hpp>

#include <cstdio>

using namespace aether::core;
using namespace aether::input;

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

    int key_events = 0;
    ctx->events().subscribe<KeyEvent>([&](const KeyEvent&) { ++key_events; });

    InputManager input(&ctx->events());
    input.register_default_rpg_actions();

    input.begin_frame();
    CHECK(!input.is_down("confirm"));
    CHECK(!input.was_pressed("confirm"));

    input.feed_key(Key::Z, Action::Press);
    CHECK(input.is_key_down(Key::Z));
    CHECK(input.was_key_pressed(Key::Z));
    CHECK(input.is_down("confirm"));
    CHECK(input.was_pressed("confirm"));
    CHECK(key_events == 1);

    input.end_frame();
    input.begin_frame();
    // held, but not newly pressed
    CHECK(input.is_down("confirm"));
    CHECK(!input.was_pressed("confirm"));

    input.feed_key(Key::Z, Action::Release);
    CHECK(!input.is_down("confirm"));
    CHECK(input.was_released("confirm"));

    input.feed_mouse_move(10, 20);
    input.feed_mouse_move(15, 28);
    CHECK(input.mouse_x() == 15);
    CHECK(input.mouse_y() == 28);
    CHECK(input.mouse_dx() == 5);
    CHECK(input.mouse_dy() == 8);

    input.feed_scroll(0, -1.0);
    CHECK(input.scroll_y() == -1.0);

    // cancel via Escape
    input.begin_frame();
    input.feed_key(Key::Escape, Action::Press);
    CHECK(input.was_pressed("cancel"));

    ctx->shutdown();

    if (g_failures == 0) {
        std::puts("OK: input_test passed");
        return 0;
    }
    std::fprintf(stderr, "FAIL: %d check(s) failed\n", g_failures);
    return 1;
}
