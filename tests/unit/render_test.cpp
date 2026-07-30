/**
 * @file render_test.cpp
 * @brief Tests für Camera, Frustum-Culling, LOD, NullRenderer.
 */
#include <aether/render/render_module.hpp>
#include <aether/core/core.hpp>

#include <cmath>
#include <cstdio>
#include <vector>

using namespace aether;
using namespace aether::core;
using namespace aether::render;

static int g_failures = 0;

#define CHECK(cond)                                                            \
    do {                                                                       \
        if (!(cond)) {                                                         \
            std::fprintf(stderr, "CHECK failed: %s (%s:%d)\n", #cond,         \
                         __FILE__, __LINE__);                                  \
            ++g_failures;                                                      \
        }                                                                      \
    } while (0)

static void test_aabb_transform() {
    AABB box{{-1, -1, -1}, {1, 1, 1}};
    Transform t;
    t.position = {10, 0, 0};
    t.scale = {2, 2, 2};
    const AABB w = box.transformed(t.matrix());
    CHECK(std::fabs(w.center().x - 10.0f) < 0.01f);
    CHECK(std::fabs(w.extents().x - 2.0f) < 0.01f);
}

static void test_mesh_lod_select() {
    auto mesh = Mesh::create_cube(1.0f);
    MeshLod far_lod = mesh->lod(0);
    far_lod.max_distance = 20.0f;
    // Vereinfachen: weniger Indizes symbolisch – gleiche Daten ok
    mesh->set_lod(0, far_lod);

    MeshLod low = mesh->lod(0);
    low.max_distance = 1.0e9f;
    // Halbiere Indizes künstlich nicht nötig
    mesh->set_lod(1, low);

    // Nach set_lod(0) mit max 20 und lod1 max inf
    // select: dist 5 -> lod0, dist 50 -> lod1
    CHECK(mesh->select_lod(5.0f) == 0);
    CHECK(mesh->select_lod(50.0f) == 1);
}

static void test_frustum_culling_and_stats() {
    RendererDesc desc;
    desc.backend = RendererBackend::Null;
    desc.enable_frustum_culling = true;
    desc.enable_lod = true;

    auto renderer = Renderer::create(desc, nullptr);
    CHECK(renderer != nullptr);

    Camera cam;
    cam.set_perspective(60.0f, 16.0f / 9.0f, 0.1f, 100.0f);
    cam.look_at({0, 2, 10}, {0, 0, 0}, {0, 1, 0});

    auto cube = Mesh::create_cube(1.0f);
    // LOD0 nah, LOD1 fern
    {
        MeshLod l0 = cube->lod(0);
        l0.max_distance = 15.0f;
        cube->set_lod(0, std::move(l0));
        MeshLod l1 = cube->lod(0); // copy geometry
        // rebuild second lod from cube factory data
        auto tmp = Mesh::create_cube(1.0f);
        MeshLod low = tmp->lod(0);
        low.max_distance = 1.0e9f;
        cube->set_lod(1, std::move(low));
    }
    renderer->upload_mesh(*cube);

    std::vector<Renderable> items;

    Renderable near_obj;
    near_obj.mesh = cube;
    near_obj.transform.position = {0, 0, 0}; // vor Kamera
    items.push_back(near_obj);

    Renderable far_obj;
    far_obj.mesh = cube;
    far_obj.transform.position = {0, 0, -80}; // weit, aber im Frustum-Bereich je nach look
    // Kamera bei z=10 schaut auf 0 – Objekt bei z=-80 ist vor der Kamera Richtung -z? 
    // look_at eye(0,2,10) -> center(0,0,0) schaut Richtung -Z. z=-80 ist weiter in Blickrichtung.
    items.push_back(far_obj);

    Renderable behind;
    behind.mesh = cube;
    behind.transform.position = {0, 0, 50}; // hinter Kamera
    items.push_back(behind);

    Renderable side_far;
    side_far.mesh = cube;
    side_far.transform.position = {500, 0, 0}; // weit seitlich
    items.push_back(side_far);

    renderer->begin_frame();
    renderer->draw(cam, items);
    renderer->end_frame();

    const auto& st = renderer->stats();
    CHECK(st.submitted == 4);
    // mindestens das seitliche und idealerweise hintere gecullt
    CHECK(st.culled >= 1);
    CHECK(st.drawn >= 1);
    CHECK(st.drawn + st.culled == st.submitted);
    CHECK(st.triangles > 0);

    // LOD: nahes Objekt lod0, fernes lod1
    CHECK(st.lod_histogram[0] + st.lod_histogram[1] == st.drawn);
}

static void test_shaders_builtin() {
    auto src = ShaderProgram::builtin_unlit_color();
    CHECK(!src.vertex.empty());
    CHECK(!src.fragment.empty());
    CHECK(src.name == "unlit_color");

    RendererDesc desc;
    auto renderer = Renderer::create(desc);
    auto* p = renderer->shaders().find("lit_basic");
    CHECK(p != nullptr);
    CHECK(p->ready());
}

int main() {
    test_aabb_transform();
    test_mesh_lod_select();
    test_frustum_culling_and_stats();
    test_shaders_builtin();

    if (g_failures == 0) {
        std::puts("OK: render_test passed");
        return 0;
    }
    std::fprintf(stderr, "FAIL: %d check(s) failed\n", g_failures);
    return 1;
}
