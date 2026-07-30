/**
 * @file event_runner_test.cpp
 */
#include <aether/game/database.hpp>
#include <aether/game/event_runner.hpp>
#include <aether/game/map_loader.hpp>
#include <aether/scene/scene.hpp>

#include <cstdio>

using namespace aether;
using namespace aether::game;
using namespace aether::scene;

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
    auto sc = create_default_map("t");
    CHECK(sc != nullptr);

    // Add autorun event object
    render::Transform t;
    t.position = {10, 0, 10};
    auto mesh = render::Mesh::create_cube(0.5f);
    auto id = sc->place(ObjectType::Event, "Auto", mesh, t);
    auto* o = sc->find(id);
    CHECK(o != nullptr);
    MapEvent ev;
    ev.name = "Auto";
    EventPage page;
    page.trigger = EventTrigger::Autorun;
    page.commands.push_back(
        {EventCommandType::Message, {{"text", "autorun"}}, {}});
    ev.pages.push_back(page);
    o->map_event = ev;

    GameState st;
    EventInterpreter interp(&st);
    MapEventRunner runner;
    runner.reset_map();

    const bool started =
        runner.update(*sc, {0, 0, 0}, 1.0f, interp, st);
    CHECK(started);
    CHECK(interp.is_running());
    interp.update();
    CHECK(!interp.messages().empty());

    // second update should not re-fire autorun
    EventInterpreter interp2(&st);
    const bool again = runner.update(*sc, {0, 0, 0}, 1.0f, interp2, st);
    CHECK(!again);

    // Touch event
    MapEvent tev;
    tev.name = "Touch";
    EventPage tp;
    tp.trigger = EventTrigger::PlayerTouch;
    tp.commands.push_back(
        {EventCommandType::Message, {{"text", "touch"}}, {}});
    tev.pages.push_back(tp);
    o->map_event = tev;
    o->transform.position = {0, 0, 0};
    runner.reset_map();
    EventInterpreter interp3(&st);
    CHECK(runner.update(*sc, {0.2f, 0, 0.2f}, 1.0f, interp3, st));

    ScreenFade fade;
    fade.fade_out(0.1f);
    fade.update(0.2);
    CHECK(fade.alpha() >= 0.99f || fade.just_black());
    fade.fade_in(0.1f);
    fade.update(0.2);
    CHECK(fade.alpha() <= 0.01f);

    // Database classes
    auto db = Database::make_default();
    CHECK(!db.classes.empty());
    CHECK(db.find_class(1) != nullptr);
    CHECK(db.find_enemy(2) != nullptr);

    if (g_failures == 0) {
        std::puts("OK: event_runner_test passed");
        return 0;
    }
    std::fprintf(stderr, "FAIL: %d\n", g_failures);
    return 1;
}
