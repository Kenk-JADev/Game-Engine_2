/**
 * @file pick_test.cpp
 */
#include <aether/game/map_loader.hpp>
#include <aether/render/camera.hpp>
#include <aether/scene/scene.hpp>

#include <cmath>
#include <cstdio>

using namespace aether;
using namespace aether::render;
using namespace aether::scene;
using namespace aether::game;

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
    Camera cam;
    cam.set_perspective(45.f, 16.f / 9.f, 0.1f, 500.f);
    cam.look_at({10, 10, 10}, {0, 0, 0}, {0, 1, 0});

    Vec3 o, d;
    cam.screen_to_ray(640, 360, 1280, 720, o, d);
    CHECK(std::fabs(glm::length(d) - 1.0f) < 0.01f);

    Vec3 hit;
    CHECK(Camera::ray_plane_y(o, d, 0.0f, hit));

    AABB box{{-1, 0, -1}, {1, 2, 1}};
    f32 t = 0;
    // ray from -z toward origin
    CHECK(Camera::ray_aabb({0, 1, -5}, {0, 0, 1}, box, t));
    CHECK(t > 0.0f);

    auto sc = create_default_map("t");
    CHECK(sc != nullptr);
    // pick from above toward ground player area
    const EntityId id = sc->pick_ray({0, 20, 0}, {0, -1, 0});
    // should hit something (player or ground)
    CHECK(id != kInvalidEntity);

    if (g_failures == 0) {
        std::puts("OK: pick_test passed");
        return 0;
    }
    std::fprintf(stderr, "FAIL: %d\n", g_failures);
    return 1;
}
