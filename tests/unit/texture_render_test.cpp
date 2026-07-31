/**
 * @file texture_render_test.cpp
 * @brief Textur-Pipeline: stb-Load → SceneObject → Renderable → Renderer.
 *
 * Prüft die komplette Kette ohne GPU:
 *   PNG → ResourceManager::load_texture → TextureData
 *   SceneObject.texture_path → JSON-Roundtrip → attach_textures
 *   collect_renderables → NullRenderer::draw → stats.textured_draws
 */
#include <aether/core/core.hpp>
#include <aether/game/map_loader.hpp>
#include <aether/render/renderer.hpp>
#include <aether/res/resource_manager.hpp>
#include <aether/scene/scene.hpp>

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>

using namespace aether;
using namespace aether::render;
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

// 4x4 RGBA-Checker-PNG (84 Bytes)
static const unsigned char kTinyPng[] = {
    0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00, 0x00, 0x0d, 0x49, 0x48,
    0x44, 0x52, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x04, 0x08, 0x06, 0x00, 0x00,
    0x00, 0xa9, 0xf1, 0x9e, 0x7e, 0x00, 0x00, 0x00, 0x1b, 0x49, 0x44, 0x41, 0x54, 0x78,
    0x9c, 0x63, 0x30, 0x3a, 0x61, 0xf4, 0x1f, 0x84, 0x4f, 0x18, 0x41, 0x30, 0x03, 0x86,
    0x00, 0x8c, 0x01, 0x93, 0xc0, 0x10, 0x00, 0x00, 0x92, 0x08, 0x22, 0xb1, 0x86, 0xd2,
    0x62, 0x24, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4e, 0x44, 0xae, 0x42, 0x60, 0x82,
};

int main() {
    const fs::path root = fs::temp_directory_path() / "aether_tex_test";
    fs::remove_all(root);
    fs::create_directories(root / "textures");

    // --- 1. PNG → TextureData ------------------------------------------------
    const fs::path png = root / "textures" / "tiny.png";
    {
        std::ofstream o(png, std::ios::binary);
        o.write(reinterpret_cast<const char*>(kTinyPng), sizeof(kTinyPng));
    }

    ResourceManager res;
    res.mount("g", root);
    auto tex = res.load_texture("g/textures/tiny.png");
    CHECK(tex.is_ok());
    if (tex) {
        const auto& t = *tex.value();
        CHECK(t.width == 4 && t.height == 4);
        CHECK(t.channels >= 3);
        CHECK(t.pixels.size() == static_cast<usize>(t.width) * t.height * 4);
        // Pixel (0,0) ist Grün (50,200,50), Pixel (2,0) ist Rot (200,50,50)
        CHECK(t.pixels[0] < t.pixels[8]);
        CHECK(t.pixels[1] > t.pixels[9]);
    }

    // --- 2. Scene-JSON-Roundtrip mit texture_path -----------------------------
    {
        scene::Scene sc("TexMap");
        render::Transform t;
        t.scale = {10.0f, 1.0f, 10.0f};
        auto id = sc.place(scene::ObjectType::Prop, "Boden",
                           render::Mesh::create_plane(10.0f), t);
        if (auto* o = sc.find(id)) {
            o->texture_path = "g/textures/tiny.png";
        }
        const fs::path json = root / "texmap.json";
        CHECK(game::save_map(json, sc).is_ok());

        auto sc2 = scene::Scene::create_from_json(
            nlohmann::json::parse(std::ifstream(json)));
        bool found = false;
        for (const auto& o : sc2->objects()) {
            if (o.name == "Boden" && o.texture_path == "g/textures/tiny.png") {
                found = true;
            }
        }
        CHECK(found);
    }

    // --- 3. attach_textures via map_loader ------------------------------------
    {
        auto loaded = game::load_map(root / "texmap.json", &res, nullptr);
        CHECK(loaded.ok && loaded.scene != nullptr);
        bool has_tex = false;
        for (const auto& o : loaded.scene->objects()) {
            if (o.texture && o.texture->width == 4) {
                has_tex = true;
            }
        }
        CHECK(has_tex);
    }

    // --- 4. Renderable-Kette → NullRenderer-Statistik --------------------------
    {
        auto loaded = game::load_map(root / "texmap.json", &res, nullptr);
        CHECK(loaded.ok && loaded.scene != nullptr);

        core::EngineConfig cfg;
        cfg.mode = core::AppMode::Headless;
        auto ctx = core::EngineContext::create(cfg);
        ctx->start();
        auto renderer = Renderer::create(RendererDesc{});
        CHECK(renderer != nullptr);
        CHECK(renderer->backend() == RendererBackend::Null);

        // Lazy-Upload zählt nur einmal
        renderer->upload_texture(*loaded.scene->objects()[0].texture);
        renderer->upload_texture(*loaded.scene->objects()[0].texture);
        CHECK(renderer->stats().textures_uploaded == 1);

        // Karte mit Kamera zeichnen (Boden mit Textur sichtbar)
        render::Camera cam;
        cam.set_perspective(45.0f, 1.6f, 0.1f, 500.0f);
        cam.look_at({0.0f, 8.0f, 12.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f});

        std::vector<Renderable> items;
        loaded.scene->collect_renderables(items);
        CHECK(items.size() == 1);
        CHECK(items[0].texture != nullptr);
        CHECK(items[0].texture->width == 4);

        renderer->begin_frame();
        renderer->draw(cam, items);
        renderer->end_frame();
        CHECK(renderer->stats().drawn >= 1);
        CHECK(renderer->stats().textured_draws >= 1);

        ctx->shutdown();
        ctx.reset();
    }

    fs::remove_all(root);

    if (g_failures == 0) {
        std::puts("OK: texture_render_test passed");
        return 0;
    }
    std::fprintf(stderr, "FAIL: %d\n", g_failures);
    return 1;
}
