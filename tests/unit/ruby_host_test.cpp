/**
 * @file ruby_host_test.cpp
 * @brief Tests für RubyHost: echte Engine-API-Bindings auf Stub-VM.
 *
 * Prüft, dass Ruby-Aufrufe (über vm.eval mit Modul-Methoden-Syntax) echte
 * Auswirkungen auf die angebundenen Engine-Systeme haben.
 */
#include <aether/audio/audio_engine.hpp>
#include <aether/core/core.hpp>
#include <aether/game/database.hpp>
#include <aether/game/event_system.hpp>
#include <aether/game/inventory.hpp>
#include <aether/game/player.hpp>
#include <aether/game/quest.hpp>
#include <aether/game/scene_stack.hpp>
#include <aether/game/weather.hpp>
#include <aether/input/input_manager.hpp>
#include <aether/render/camera.hpp>
#include <aether/ruby/ruby_host.hpp>
#include <aether/ruby/ruby_vm.hpp>
#include <aether/scene/scene.hpp>

#include <cstdio>
#include <memory>
#include <string>

using namespace aether;
using namespace aether::ruby;

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
    core::EngineConfig cfg;
    cfg.mode = core::AppMode::Headless;
    auto ctx = core::EngineContext::create(cfg);
    ctx->start();

    // --- Systeme aufbauen ----------------------------------------------------
    game::GameState state;
    game::PartyInventory inventory;
    game::QuestLog quests;
    quests.register_def({"q1", "Erste Quest", "Beschreibung", 10, 5, 1, 1});
    game::WeatherSystem weather;
    input::InputManager input(&ctx->events());
    input.register_default_rpg_actions();
    auto audio = audio::AudioEngine::create(audio::AudioBackend::Null, cfg.audio);
    render::Camera camera;
    camera.set_perspective(45.0f, 16.0f / 9.0f, 0.1f, 500.0f);
    scene::Scene scene("TestMap");
    game::GameContext gctx;
    gctx.input = &input;
    game::GameSceneStack scenes;

    game::Database db = game::Database::make_default();
    db.items.push_back({1, "Potion", "Heilt HP", 10, true, 20, 0});
    db.enemies.push_back({1, "Slime", 30, 5, 3, 8, 4, "slime.png"});

    // Spieler-Scene-Objekt
    render::Transform pt;
    pt.position = {0.0f, 0.0f, 0.0f};
    auto player_id = scene.place(scene::ObjectType::Character, "Player",
                                 render::Mesh::create_cube(), pt);
    game::PlayerController player;
    player.bind(&scene, player_id, 0);

    // NPC in der Szene
    render::Transform nt;
    nt.position = {3.0f, 0.0f, 4.0f};
    (void)scene.place(scene::ObjectType::Npc, "Bob", render::Mesh::create_cube(), nt);

    // --- VM + Host ------------------------------------------------------------
    auto vm = RubyVM::create(RubyBackend::Stub);
    vm->define_engine_api();
    RubyHostBindings b;
    b.state = &state;
    b.inventory = &inventory;
    b.quests = &quests;
    b.weather = &weather;
    b.player = &player;
    b.camera = &camera;
    b.input = &input;
    b.audio = audio.get();
    b.scenes = &scenes;
    b.gctx = &gctx;
    b.database = &db;
    b.scene = &scene;
    b.screen_width = 1280;
    b.screen_height = 720;
    RubyHost host(b);
    host.install(*vm);

    // --- Audio ----------------------------------------------------------------
    auto r = vm->eval("Audio.bgm_play(\"Theme1\", 70, 100)");
    CHECK(r.ok);
    CHECK(audio->bgm_state().playing);
    CHECK(audio->bgm_state().name == "Theme1");
    CHECK(audio->bgm_state().params.volume == 70);
    vm->eval("Audio.bgm_stop(0)");
    CHECK(!audio->bgm_state().playing);

    // --- Wetter ---------------------------------------------------------------
    vm->eval("Weather.set(\"rain\", 7)");
    weather.update(1.0); // Fade abschließen (Runtime macht das pro Frame)
    CHECK(weather.state().type == game::WeatherType::Rain);
    CHECK(weather.state().power > 6.9f && weather.state().power < 7.1f);
    r = vm->eval("Weather.type");
    CHECK(r.ok && r.value == "\"rain\"");

    // --- Inventar -------------------------------------------------------------
    vm->eval("Inventory.gain(1, 3)"); // Item-Id 1 = Potion
    CHECK(inventory.item_count(1) == 3);
    vm->eval("Inventory.gain(\"Potion\", 2)"); // per Name
    CHECK(inventory.item_count(1) == 5);
    vm->eval("Inventory.lose(1, 2)");
    CHECK(inventory.item_count(1) == 3);
    r = vm->eval("Inventory.count(1)");
    CHECK(r.ok && r.value == "3");
    r = vm->eval("Inventory.has?(1)");
    CHECK(r.ok && r.value == "true");
    vm->eval("Inventory.gold= 250");
    CHECK(inventory.gold() == 250);
    r = vm->eval("Inventory.gold");
    CHECK(r.ok && r.value == "250");

    // --- Quest ----------------------------------------------------------------
    vm->eval("Quest.start(\"q1\")");
    CHECK(quests.active("q1"));
    r = vm->eval("Quest.active?(\"q1\")");
    CHECK(r.ok && r.value == "true");
    vm->eval("Quest.complete(\"q1\")");
    CHECK(quests.completed("q1"));
    r = vm->eval("Quest.list");
    CHECK(r.ok);

    // --- Schalter/Variablen ---------------------------------------------------
    vm->eval("Game.set_switch(7, true)");
    CHECK(state.get_switch(7));
    r = vm->eval("Game.switch(7)");
    CHECK(r.ok && r.value == "true");
    vm->eval("Game.set_variable(3, 42)");
    CHECK(state.get_variable(3) == 42);
    r = vm->eval("Game.variable(3)");
    CHECK(r.ok && r.value == "42");

    // --- Spieler --------------------------------------------------------------
    vm->eval("Player.set_position(1, 2, 3)");
    // Engine erzwingt Y aus der PlayerConfig (stilisierter RPG-Look)
    CHECK(player.position().x == 1.0f && player.position().y == 0.0f &&
          player.position().z == 3.0f);
    vm->eval("Player.move(0, 0, 1)");
    CHECK(player.position().z == 4.0f);
    vm->eval("Player.transfer(2, 5, 0, 5, 6)");
    u32 tmap = 0;
    render::Vec3 tpos{0.0f};
    i32 tdir = 0;
    CHECK(host.take_transfer(tmap, tpos, tdir));
    CHECK(tmap == 2 && tdir == 6);
    CHECK(tpos.x == 5.0f && tpos.z == 5.0f);
    CHECK(!host.take_transfer(tmap, tpos, tdir)); // nur einmal

    // --- NPC ------------------------------------------------------------------
    r = vm->eval("NPC.find(\"Bob\")");
    CHECK(r.ok && !r.value.empty() && r.value != "nil");
    vm->eval("NPC.set_position(\"Bob\", 10, 0, 12)");
    {
        bool found = false;
        for (const auto& o : scene.objects()) {
            if (o.name == "Bob" && o.transform.position.x == 10.0f &&
                o.transform.position.z == 12.0f) {
                found = true;
            }
        }
        CHECK(found);
    }
    vm->eval("NPC.say(\"Bob\", \"Hallo!\" )");
    CHECK(gctx.dialog_open);
    CHECK(!gctx.dialog_lines.empty() && gctx.dialog_lines.front() == "Hallo!");
    gctx.dialog_open = false;

    // --- Gegner ---------------------------------------------------------------
    vm->eval("Enemy.spawn(1, 0, 0, 0)");
    r = vm->eval("Enemy.count");
    CHECK(r.ok && r.value == "1");
    r = vm->eval("Enemy.alive?(\"Enemy_1\")");
    CHECK(r.ok && r.value == "true");
    vm->eval("Enemy.kill(\"Enemy_1\")");
    r = vm->eval("Enemy.count");
    CHECK(r.ok && r.value == "0");

    // --- Kamera ---------------------------------------------------------------
    vm->eval("Camera.move_to(1, 2, 3)");
    CHECK(camera.position().x == 1.0f && camera.position().y == 2.0f &&
          camera.position().z == 3.0f);
    vm->eval("Camera.look_at(0, 1, 0)");
    CHECK(camera.target().y == 1.0f);
    vm->eval("Camera.zoom(60)");
    CHECK(camera.fov_y_degrees() > 59.9f && camera.fov_y_degrees() < 60.1f);

    // --- Szenen / Karte / Fade ------------------------------------------------
    vm->eval("SceneManager.goto(\"menu\")");
    CHECK(host.take_scene_request() == "menu");
    vm->eval("SceneManager.goto(:title)");
    CHECK(host.take_scene_request() == "title");
    vm->eval("SceneManager.goto(\"exit\")");
    CHECK(gctx.request_quit);
    CHECK(host.take_scene_request().empty());
    vm->eval("Map.load(3)");
    u32 mload = 0;
    CHECK(host.take_map_load(mload) && mload == 3);
    vm->eval("Graphics.fade_out(30)");
    CHECK(host.take_fade_request() == 1);
    vm->eval("Map.tint(0.2, 0.3, 0.4, 0.5)");
    f32 tr = 0, tg = 0, tb = 0, ta = 0;
    host.map_tint(tr, tg, tb, ta);
    CHECK(tr > 0.19f && tg > 0.29f && tb > 0.39f && ta > 0.49f);
    vm->eval("Camera.shake(3, 0.5)");
    f32 sp = 0, sd = 0;
    host.camera_shake(sp, sd);
    CHECK(sp > 2.9f && sd > 0.49f);

    // --- Graphics (Setter-Syntax im Stub) --------------------------------------
    r = vm->eval("Graphics.frame_rate = 30");
    CHECK(r.ok);
    r = vm->call_host("Graphics", "frame_rate");
    CHECK(r.ok && r.value == "30");
    r = vm->eval("Graphics.width");
    CHECK(r.ok && r.value == "1280");

    // --- Input ------------------------------------------------------------------
    vm->eval("Input.trigger?(:C)"); // kein Trigger → false, keine Auswirkung
    input.feed_key(input::Key::Space, input::Action::Press);
    input.begin_frame();
    r = vm->eval("Input.press?(:C)");
    CHECK(r.ok && r.value == "true");
    r = vm->eval("Input.press?(:B)");
    CHECK(r.ok && r.value == "false");

    // --- Dialog ----------------------------------------------------------------
    vm->eval("Dialogue.start(\"Willkommen!\")");
    CHECK(gctx.dialog_open);
    r = vm->eval("Dialogue.text");
    CHECK(r.ok && r.value == "\"Willkommen!\"");
    vm->eval("Dialogue.choices(\"Ja\", \"Nein\")");
    CHECK(gctx.choice_open);
    CHECK(gctx.choice_labels.size() == 2 && gctx.choice_labels[0] == "Ja");
    gctx.choice_result = 1;
    r = vm->eval("Dialogue.choice");
    CHECK(r.ok && r.value == "1");
    vm->eval("Dialogue.close()");
    CHECK(!gctx.dialog_open && !gctx.choice_open);

    ctx->shutdown();
    ctx->state();

    if (g_failures == 0) {
        std::puts("OK: ruby_host_test passed");
        return 0;
    }
    std::fprintf(stderr, "FAIL: %d\n", g_failures);
    return 1;
}
