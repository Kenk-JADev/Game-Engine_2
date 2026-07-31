/**
 * @file billboard_test.cpp
 * @brief Billboard-Sprites: Quad-Mesh, Yaw-Modell, Scene-JSON, Render-Pfad.
 */
#include <aether/game/map_loader.hpp>
#include <aether/render/math.hpp>
#include <aether/render/renderer.hpp>
#include <aether/scene/scene.hpp>

#include <cmath>
#include <cstdio>
#include <filesystem>
#include <memory>

using namespace aether;
using namespace aether::render;
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
    // --- Quad-Mesh: senkrecht, Fußpunkt y=0, UV (0,0)=oben-links --------------
    {
        auto quad = Mesh::create_quad(0.7f, 1.1f);
        CHECK(quad != nullptr);
        const auto& lod = quad->lod(0);
        CHECK(lod.vertices.size() == 4);
        CHECK(lod.indices.size() == 6);
        CHECK(lod.vertices[0].position.x == -0.35f);
        CHECK(lod.vertices[0].position.y == 0.0f);
        CHECK(lod.vertices[2].position.y == 1.1f);
        // UV: oben-links = (0,0)
        CHECK(lod.vertices[3].uv.x == 0.0f && lod.vertices[3].uv.y == 0.0f);
        // Normalen zeigen +Z
        CHECK(lod.vertices[0].normal.z > 0.99f);
    }

    // --- billboard_model: Yaw zur Kamera ---------------------------------------
    {
        Transform t;
        t.position = {0.0f, 0.0f, 0.0f};
        t.scale = {1.0f, 1.0f, 1.0f};

        // Kamera bei +Z → keine Drehung (Quad zeigt bereits +Z)
        auto m = billboard_model(t, {0.0f, 5.0f, 10.0f});
        const Vec3 fwd = glm::normalize(Vec3(m * Vec4(0.0f, 0.0f, 1.0f, 0.0f)));
        CHECK(std::fabs(fwd.z - 1.0f) < 1.0e-3f);

        // Kamera bei +X → Billboard zeigt Richtung +X (nur Yaw)
        auto m2 = billboard_model(t, {10.0f, 5.0f, 0.0f});
        const Vec3 fwd2 = glm::normalize(Vec3(m2 * Vec4(0.0f, 0.0f, 1.0f, 0.0f)));
        CHECK(std::fabs(fwd2.x - 1.0f) < 1.0e-3f);
        CHECK(std::fabs(fwd2.y) < 1.0e-3f); // bleibt aufrecht

        // Kamera bei 45° (XZ)
        auto m3 = billboard_model(t, {1.0f, 5.0f, 1.0f});
        const Vec3 fwd3 = glm::normalize(Vec3(m3 * Vec4(0.0f, 0.0f, 1.0f, 0.0f)));
        CHECK(std::fabs(fwd3.x - fwd3.z) < 1.0e-3f);
        CHECK(fwd3.x > 0.7f);
    }

    // --- Scene-JSON-Roundtrip mit billboard ------------------------------------
    {
        scene::Scene sc("BMap");
        render::Transform t;
        auto id = sc.place(scene::ObjectType::Npc, "Wächter",
                           Mesh::create_quad(0.7f, 1.1f), t);
        if (auto* o = sc.find(id)) {
            o->billboard = true;
            o->texture_path = "graphics/textures/elder.png";
        }
        const fs::path jp = fs::temp_directory_path() / "aether_bb_test_map.json";
        CHECK(game::save_map(jp, sc).is_ok());
        auto sc2 = scene::Scene::create_from_json(
            nlohmann::json::parse(std::ifstream(jp)));
        bool found = false;
        for (const auto& o : sc2->objects()) {
            if (o.name == "Wächter" && o.billboard &&
                o.texture_path == "graphics/textures/elder.png") {
                found = true;
            }
        }
        CHECK(found);
        fs::remove(jp);
    }

    // --- Render-Pfad: Billboard-Objekt wird gezeichnet --------------------------
    {
        scene::Scene sc("BB");
        render::Transform t;
        auto id = sc.place(scene::ObjectType::Npc, "Sprüch", Mesh::create_quad(1, 1), t);
        if (auto* o = sc.find(id)) {
            o->billboard = true;
        }
        std::vector<Renderable> items;
        sc.collect_renderables(items);
        CHECK(items.size() == 1);
        CHECK(items[0].billboard);

        render::Camera cam;
        cam.set_perspective(45.0f, 1.6f, 0.1f, 500.0f);
        cam.look_at({0.0f, 6.0f, 10.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f});
        auto renderer = Renderer::create(RendererDesc{});
        renderer->begin_frame();
        renderer->draw(cam, items);
        renderer->end_frame();
        CHECK(renderer->stats().drawn == 1);
    }

    if (g_failures == 0) {
        std::puts("OK: billboard_test passed");
        return 0;
    }
    std::fprintf(stderr, "FAIL: %d\n", g_failures);
    return 1;
}
