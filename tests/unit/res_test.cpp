/**
 * @file res_test.cpp
 * @brief Tests für ResourceManager (VFS, Cache, JSON, async).
 */
#include <aether/res/resource_module.hpp>
#include <aether/core/core.hpp>

#include <cstdio>
#include <filesystem>
#include <fstream>

using namespace aether::core;
using namespace aether::res;
namespace fs = std::filesystem;

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
    const fs::path root = fs::temp_directory_path() / "aether_res_test";
    fs::remove_all(root);
    fs::create_directories(root / "data");
    fs::create_directories(root / "graphics" / "textures");

    {
        std::ofstream(root / "data" / "actors.json") << R"({"actors":[{"id":1,"name":"Hero"}]})";
        std::ofstream(root / "data" / "note.txt") << "hello aether";
        std::ofstream(root / "graphics" / "textures" / "hero.png", std::ios::binary)
            << "fake-png-bytes";
    }

    EngineConfig cfg;
    cfg.mode = AppMode::Headless;
    cfg.log.console = false;
    cfg.log.file = false;
    cfg.worker_threads = 2;
    auto ctx = EngineContext::create(std::move(cfg));
    ctx->start();

    ResourceManager res(&ctx->thread_pool());
    res.mount("data", root / "data");
    res.mount("graphics", root / "graphics");

    CHECK(res.exists("data/note.txt"));
    CHECK(res.exists("data/actors.json"));
    CHECK(res.exists("graphics/textures/hero.png"));
    CHECK(!res.exists("data/missing.json"));

    auto text = res.load_text("data/note.txt");
    CHECK(text.is_ok());
    CHECK(text.value()->text == "hello aether");
    CHECK(res.cached_count() == 1);

    // Cache hit
    auto text2 = res.load_text("data/note.txt");
    CHECK(text2.is_ok());
    CHECK(text2.value().get() == text.value().get());
    auto info = res.info_by_key("data/note.txt");
    CHECK(info.has_value());
    CHECK(info->ref_count == 2);

    auto json = res.load_json("data/actors.json");
    CHECK(json.is_ok());
    CHECK((*json.value())["actors"][0]["name"] == "Hero");

    auto bytes = res.load_bytes("graphics/textures/hero.png");
    CHECK(bytes.is_ok());
    CHECK(!bytes.value()->data.empty());

    auto tex = res.load_texture("graphics/textures/hero.png");
    CHECK(tex.is_ok());
    CHECK(tex.value()->width == 2);
    CHECK(tex.value()->channels == 4);

    // Mesh stub (file must exist – reuse png path as dummy mesh key via mount)
    // create dummy model file
    {
        std::ofstream(root / "graphics" / "hero.obj") << "v 0 0 0\n";
    }
    auto mesh = res.load_mesh("graphics/hero.obj");
    CHECK(mesh.is_ok());
    CHECK(mesh.value()->lod_count() == 1);

    auto fut = res.load_text_async("data/note.txt");
    auto async_text = fut.get();
    CHECK(async_text.is_ok());
    // async load bumped ref_count (3 total: text, text2, async)

    // Release all text refs & unload
    const auto handle = info->handle;
    res.release(handle);
    res.release(handle);
    res.release(handle);
    // drop shared_ptrs so only cache owns data
    text = aether::Result<std::shared_ptr<TextData>>::fail("drop");
    text2 = aether::Result<std::shared_ptr<TextData>>::fail("drop");
    async_text = aether::Result<std::shared_ptr<TextData>>::fail("drop");

    const auto removed = res.unload_unused();
    CHECK(removed >= 1); // note.txt should be gone (ref 0)

    auto fb = res.fallback_texture();
    CHECK(fb->width == 1);
    CHECK(fb->pixels.size() == 4);

    res.clear();
    CHECK(res.cached_count() == 0);

    ctx->shutdown();
    fs::remove_all(root);

    if (g_failures == 0) {
        std::puts("OK: res_test passed");
        return 0;
    }
    std::fprintf(stderr, "FAIL: %d check(s) failed\n", g_failures);
    return 1;
}
