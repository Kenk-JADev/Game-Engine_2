/**
 * @file gameplay_test.cpp
 * @brief Player, map loader, weather, animation, interaction.
 */
#include <aether/anim/animation.hpp>
#include <aether/game/map_loader.hpp>
#include <aether/game/player.hpp>
#include <aether/game/weather.hpp>
#include <aether/input/input_module.hpp>
#include <aether/render/render_module.hpp>
#include <aether/scene/scene.hpp>

#include <cstdio>
#include <filesystem>
#include <cmath>

using namespace aether;
using namespace aether::game;
using namespace aether::anim;
using namespace aether::input;
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
    // Animation tween
    Tween tw;
    tw.from = 0;
    tw.to = 10;
    tw.duration = 1.0f;
    tw.update(0.5f);
    CHECK(std::fabs(tw.value() - 5.0f) < 0.01f);

    auto idle = make_idle_bob(0.2f, 2.0f);
    Animator an;
    an.play(idle);
    an.update(0.5f);
    Transform t;
    an.apply(t);
    CHECK(an.playing());

    // Weather
    WeatherSystem weather;
    weather.set(WeatherType::Rain, 8.0f, 0.0f);
    CHECK(weather.state().type == WeatherType::Rain);
    auto c = weather.clear_color_mod(Color::white());
    CHECK(c.b > 0.5f);

    // Default map + player movement
    auto sc = create_default_map("Test");
    CHECK(sc != nullptr);
    CHECK(!sc->objects().empty());
    CHECK(sc->has_nav());

    EntityId pid = kInvalidEntity;
    for (const auto& o : sc->objects()) {
        if (o.type == ObjectType::Character) {
            pid = o.id;
            break;
        }
    }
    CHECK(pid != kInvalidEntity);

    PlayerController player;
    u32 col = sc->find(pid)->collision_id;
    player.bind(sc.get(), pid, col);
    const auto start = player.position();

    InputManager input;
    input.register_default_rpg_actions();
    input.begin_frame();
    input.feed_key(Key::W, Action::Press);
    // move several fixed steps
    for (int i = 0; i < 10; ++i) {
        player.update_movement(input, 1.0 / 60.0, &sc->collision());
    }
    CHECK(player.position().z < start.z); // W = up = -Z

    // Interact target near elder
    player.set_position({3.0f, 0.0f, -1.0f});
    const EntityId target = player.find_interact_target(*sc);
    CHECK(target != kInvalidEntity);

    // Map save/load roundtrip
    const auto path =
        std::filesystem::temp_directory_path() / "aether_map_test.json";
    auto saved = save_map(path, *sc);
    CHECK(saved.is_ok());
    auto loaded = load_map(path, nullptr, nullptr);
    CHECK(loaded.ok);
    CHECK(loaded.scene->objects().size() >= sc->objects().size());
    std::filesystem::remove(path);

    // Follow camera
    FollowCamera cam;
    cam.snap({0, 0, 0});
    cam.update({5, 0, 5}, 0.1);
    Camera c3;
    cam.apply(c3);
    CHECK(c3.position().y > 0.0f);

    if (g_failures == 0) {
        std::puts("OK: gameplay_test passed");
        return 0;
    }
    std::fprintf(stderr, "FAIL: %d\n", g_failures);
    return 1;
}
