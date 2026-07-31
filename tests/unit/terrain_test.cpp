/**
 * @file terrain_test.cpp
 * @brief Höhenfeld-Terrain: Mesh-Generierung, Interpolation, JSON, Player-Y.
 */
#include <aether/game/map_loader.hpp>
#include <aether/game/player.hpp>
#include <aether/input/input_manager.hpp>
#include <aether/render/renderer.hpp>
#include <aether/scene/scene.hpp>

#include <cmath>
#include <cstdio>
#include <filesystem>
#include <memory>

using namespace aether;
using namespace aether::render;
using namespace aether::game;
using namespace aether::scene;
using namespace aether::input;
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

static bool near(f32 a, f32 b, f32 eps = 1.0e-3f) { return std::fabs(a - b) < eps; }

int main() {
    // --- TerrainData: Reset + Interpolation -----------------------------------
    TerrainData td;
    td.reset(2, 2, 4.0f);
    CHECK(td.valid());
    CHECK(td.grid_w() == 3 && td.grid_h() == 3);
    CHECK(td.heights.size() == 9);
    CHECK(near(td.height_at(0.0f, 0.0f), 0.0f));

    // Hügel in der Mitte: h(1,1) = 2 → height_at(0,0) ≈ 0.5 (bilinear)
    td.heights[1 * 3 + 1] = 2.0f;
    CHECK(near(td.height_at(0.0f, 0.0f), 0.5f, 5.0e-2f));
    // außerhalb des Grids (Grid reicht von -4..+4) → 0
    CHECK(near(td.height_at(-6.0f, 0.0f), 0.0f, 1.0e-3f));
    CHECK(near(td.height_at(0.0f, 6.0f), 0.0f, 1.0e-3f));

    // --- Mesh-Generierung ------------------------------------------------------
    {
        // Nur Ecke (0,0) anheben → beide angrenzenden Dreiecke steigen dorthin
        std::vector<f32> h2(9, 0.0f);
        h2[0] = 2.0f;
        auto mesh = Mesh::create_terrain(2, 2, 4.0f, h2);
        CHECK(mesh != nullptr);
        const auto& lod = mesh->lod(0);
        CHECK(lod.vertices.size() == 16);
        CHECK(lod.indices.size() == 24);
        CHECK(mesh->bounds().max.y > 1.9f); // höchster Punkt ≈ 2
        CHECK(lod.vertices[0].position.y > 1.9f); // Ecke (0,0) hoch
        // Flache Normale am höchsten Punkt zeigt nach oben
        CHECK(lod.vertices[0].normal.y > 0.85f);
    }

    // --- Scene-JSON-Roundtrip + load_map --------------------------------------
    {
        Scene sc("Hügelkarte");
        sc.terrain() = td;
        sc.terrain().texture_path = "graphics/textures/checker.png";
        const fs::path jp = fs::temp_directory_path() / "aether_terrain_map.json";
        CHECK(save_map(jp, sc).is_ok());

        auto loaded = load_map(jp, nullptr, nullptr);
        CHECK(loaded.ok && loaded.scene != nullptr);
        CHECK(loaded.scene->terrain().valid());
        CHECK(loaded.scene->terrain().width == 2);
        CHECK(loaded.scene->terrain().heights.size() == 9);
        CHECK(loaded.scene->terrain().texture_path == "graphics/textures/checker.png");
        CHECK(loaded.scene->terrain_mesh() != nullptr);
        CHECK(loaded.scene->terrain_mesh()->lod(0).vertices.size() == 16);
        // collect_renderables hängt das Terrain an
        std::vector<Renderable> items;
        loaded.scene->collect_renderables(items);
        CHECK(items.size() == loaded.scene->objects().size() + 1);
        if (!items.empty()) {
            CHECK(items.back().mesh == loaded.scene->terrain_mesh());
        }
        fs::remove(jp);
    }

    // --- Player-Y folgt dem Terrain --------------------------------------------
    {
        auto sc = std::make_unique<Scene>("T");
        sc->terrain() = td;
        render::Transform pt;
        pt.position = {0.0f, 0.0f, 0.0f};
        auto pid = sc->place(ObjectType::Character, "Player", Mesh::create_quad(), pt);
        PlayerController player;
        player.bind(sc.get(), pid, 0);
        player.set_position({0.0f, 0.0f, 0.0f});

        InputManager input;
        input.register_default_rpg_actions();
        input.begin_frame();
        input.feed_key(Key::W, Action::Press); // -Z
        player.update_movement(input, 1.0 / 60.0, &sc->collision());
        // Y = Terrain-Höhe an der neuen Position (+ cfg.height 0)
        CHECK(near(player.position().y, sc->terrain().height_at(player.position().x,
                                                                player.position().z)));
        CHECK(player.position().y > 0.0f); // Spieler steht auf dem Hügel
    }

    if (g_failures == 0) {
        std::puts("OK: terrain_test passed");
        return 0;
    }
    std::fprintf(stderr, "FAIL: %d\n", g_failures);
    return 1;
}
