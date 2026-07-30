/**
 * @file core_test.cpp
 * @brief Unit-Tests für das Core-Modul (ohne externes Framework).
 */
#include <aether/core/core.hpp>

#include <atomic>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>
#include <thread>
#include <vector>

namespace fs = std::filesystem;
using namespace aether;
using namespace aether::core;

static int g_failures = 0;

#define CHECK(cond)                                                            \
    do {                                                                       \
        if (!(cond)) {                                                         \
            std::fprintf(stderr, "CHECK failed: %s (%s:%d)\n", #cond,         \
                         __FILE__, __LINE__);                                  \
            ++g_failures;                                                      \
        }                                                                      \
    } while (0)

static void test_result_type() {
    auto ok = Result<int>::ok(42);
    CHECK(ok.is_ok());
    CHECK(ok.value() == 42);

    auto err = Result<int>::fail("nope");
    CHECK(!err.is_ok());
    CHECK(err.error().what() == "nope");

    auto vok = Result<void>::ok();
    CHECK(vok);
    auto verr = Result<void>::fail("x");
    CHECK(!verr);
}

static void test_time_system() {
    TimeSystem time(1.0 / 60.0);
    time.reset();
    std::this_thread::sleep_for(std::chrono::milliseconds(16));
    const f64 dt = time.update();
    CHECK(dt > 0.0);
    CHECK(time.frame_count() == 1);

    int steps = 0;
    // Akkumulator füllen
    for (int i = 0; i < 5; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        time.update();
    }
    time.drain_fixed_steps([&](f64 fixed) {
        CHECK(fixed > 0.0);
        ++steps;
    });
    CHECK(steps >= 1);
}

static void test_logger_and_config_roundtrip() {
    const fs::path dir = fs::temp_directory_path() / "aether_core_test";
    fs::create_directories(dir);
    const fs::path cfg_path = dir / "engine.json";

    EngineConfig cfg;
    cfg.mode = AppMode::Headless;
    cfg.graphics.title = "Test";
    cfg.graphics.width = 800;
    cfg.graphics.height = 600;
    cfg.log.level = "debug";
    cfg.log.console = false;
    cfg.log.file = true;
    cfg.paths.logs_dir = dir / "logs";
    cfg.worker_threads = 2;

    auto save = save_engine_config(cfg_path, cfg);
    CHECK(save.is_ok());

    auto loaded = load_engine_config(cfg_path);
    CHECK(loaded.is_ok());
    CHECK(loaded.value().graphics.title == "Test");
    CHECK(loaded.value().graphics.width == 800);
    CHECK(loaded.value().mode == AppMode::Headless);
    CHECK(log_level_from_string(loaded.value().log.level) == LogLevel::Debug);

    fs::remove_all(dir);
}

static void test_thread_pool() {
    ThreadPool pool(2);
    std::atomic<int> counter{0};
    std::vector<std::future<int>> futures;
    for (int i = 0; i < 20; ++i) {
        futures.push_back(pool.submit([&counter, i] {
            counter.fetch_add(1, std::memory_order_relaxed);
            return i * i;
        }));
    }
    int sum = 0;
    for (auto& f : futures) {
        sum += f.get();
    }
    CHECK(counter.load() == 20);
    CHECK(sum == 2470); // 0^2+…+19^2 = 2470
    pool.wait_idle();
    pool.shutdown();
}

static void test_event_bus() {
    EventBus bus;
    int hits = 0;
    const auto id = bus.subscribe<FrameBeginEvent>([&](const FrameBeginEvent& e) {
        CHECK(e.delta_seconds >= 0.0);
        ++hits;
    });
    bus.publish(FrameBeginEvent{0.016, 1});
    bus.publish(FrameBeginEvent{0.016, 2});
    CHECK(hits == 2);
    bus.unsubscribe(id);
    bus.publish(FrameBeginEvent{0.016, 3});
    CHECK(hits == 2);
}

static void test_engine_context() {
    EngineConfig cfg;
    cfg.mode = AppMode::Headless;
    cfg.log.console = false;
    cfg.log.file = false;
    cfg.log.level = "warn";
    cfg.worker_threads = 1;

    auto ctx = EngineContext::create(std::move(cfg));
    CHECK(ctx != nullptr);
    CHECK(EngineContext::active() == ctx.get());
    CHECK(active_logger() != nullptr);

    ctx->start();
    CHECK(ctx->state() == ContextState::Running);

    int boots = 0;
    // Boot already fired in start – subscribe after and only count frames
    ctx->events().subscribe<FrameBeginEvent>([&](const FrameBeginEvent&) { ++boots; });

    CHECK(ctx->pump_frame());
    CHECK(ctx->pump_frame());
    CHECK(boots == 2);
    CHECK(ctx->time().frame_count() == 2);

    ctx->shutdown();
    CHECK(ctx->state() == ContextState::Stopped);
    CHECK(!ctx->pump_frame());
}

int main() {
    test_result_type();
    test_time_system();
    test_logger_and_config_roundtrip();
    test_thread_pool();
    test_event_bus();
    test_engine_context();

    if (g_failures == 0) {
        std::puts("OK: core_test passed");
        return 0;
    }
    std::fprintf(stderr, "FAIL: %d check(s) failed\n", g_failures);
    return 1;
}
