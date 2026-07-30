/**
 * @file phys_nav_test.cpp
 */
#include <aether/phys/collision.hpp>
#include <aether/nav/pathfinding.hpp>
#include <aether/scene/scene.hpp>
#include <aether/render/mesh.hpp>

#include <cmath>
#include <cstdio>

using namespace aether;
using namespace aether::phys;
using namespace aether::nav;
using namespace aether::scene;
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

int main() {
    // AABB helpers
    AABB a{{-0.5f, 0, -0.5f}, {0.5f, 1, 0.5f}};
    AABB b{{0.25f, 0, -0.5f}, {1.25f, 1, 0.5f}};
    CHECK(CollisionWorld::aabb_overlap(a, b));
    Vec3 n;
    const f32 pen = CollisionWorld::aabb_penetration_xz(a, b, n);
    CHECK(pen > 0.0f);

    // Collision world move
    CollisionWorld world;
    CollisionBody wall;
    wall.local_bounds = AABB{{-1, 0, -1}, {1, 2, 1}};
    wall.transform.position = {3, 0, 0}; // center at x=3 → occupies [2,4]
    wall.type = BodyType::Static;
    wall.block_movement = true;
    world.add(wall);

    CollisionBody player = make_body_from_mesh_bounds(
        0, ObjectKind::Player, AABB{{-0.3f, 0, -0.3f}, {0.3f, 1.6f, 0.3f}}, {});
    player.transform.position = {0, 0, 0};
    const u32 pid = world.add(player);

    // Move far into wall – should not end inside solid
    const Vec3 before = world.find(pid)->transform.position;
    const Vec3 final_pos = world.move_and_collide(pid, Vec3{5.0f, 0, 0});
    CHECK(final_pos.x > before.x); // moved some
    // Player half-extent 0.3, wall starts at 2 → max center ~ 1.7
    CHECK(final_pos.x < 2.0f);

    // Trigger overlap
    CollisionBody trig;
    trig.local_bounds = AABB{{-0.5f, 0, -0.5f}, {0.5f, 1, 0.5f}};
    trig.transform.position = final_pos;
    trig.type = BodyType::Trigger;
    trig.block_movement = false;
    world.add(trig);
    auto hits = world.query_overlaps();
    CHECK(!hits.empty());

    // Scene place + nav
    Scene scene("t");
    auto cube = Mesh::create_cube(2.0f);
    Transform t;
    t.position = {0, 0, 0};
    scene.place(ObjectType::Prop, "block", cube, t);
    Transform t2;
    t2.position = {6, 0, 0};
    scene.place(ObjectType::Prop, "block2", cube, t2);
    scene.bake_navigation(8.0f, 1.0f);
    CHECK(scene.has_nav());
    CHECK(scene.nav_grid().width > 0);

    auto path = find_path(scene.nav_grid(), Vec3{-4, 0, 0}, Vec3{10, 0, 0});
    CHECK(!path.empty());

    NavAgent agent;
    agent.set_path(path);
    Vec3 pos = {-4, 0, 0};
    for (int i = 0; i < 400 && agent.has_path(); ++i) {
        pos += agent.update(pos, 8.0f, 0.05);
    }
    CHECK(!agent.has_path() || glm::length(Vec3{pos.x - path.back().x, 0, pos.z - path.back().z}) < 3.0f);

    if (g_failures == 0) {
        std::puts("OK: phys_nav_test passed");
        return 0;
    }
    std::fprintf(stderr, "FAIL: %d\n", g_failures);
    return 1;
}
