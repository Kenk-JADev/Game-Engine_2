/**
 * @file game_test.cpp
 * @brief Event-Interpreter, Database, Plugin-Manifest.
 */
#include <aether/game/event_system.hpp>
#include <aether/game/database.hpp>
#include <aether/plugin/plugin_loader.hpp>

#include <cstdio>
#include <filesystem>
#include <fstream>

using namespace aether;
using namespace aether::game;
using namespace aether::plugin;
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

static void test_events() {
    GameState state;
    EventInterpreter interp(&state);

    std::vector<EventCommand> cmds;
    cmds.push_back({EventCommandType::Message, {{"text", "Hello"}}, {}});
    cmds.push_back({EventCommandType::SetSwitch, {{"id", 1}, {"value", true}}, {}});
    cmds.push_back({EventCommandType::SetVariable, {{"id", 2}, {"value", 42}}, {}});

    EventCommand cond;
    cond.type = EventCommandType::ConditionalBranch;
    cond.params = {{"switch_id", 1}, {"value", true}};
    cond.children.push_back({EventCommandType::Message, {{"text", "Yes"}}, {}});
    cmds.push_back(cond);

    bool transferred = false;
    interp.set_transfer_handler([&](u32, f32, f32, f32, i32) { transferred = true; });
    cmds.push_back({EventCommandType::TransferPlayer,
                    {{"map_id", 2}, {"x", 1.0}, {"y", 0.0}, {"z", 3.0}},
                    {}});

    bool scripted = false;
    interp.set_script_handler([&](const std::string&) { scripted = true; });
    cmds.push_back({EventCommandType::Script, {{"code", "puts 1"}}, {}});

    interp.start(cmds);
    while (interp.update()) {
    }

    CHECK(interp.messages().size() >= 2);
    CHECK(interp.messages()[0] == "Hello");
    CHECK(state.get_switch(1) == true);
    CHECK(state.get_variable(2) == 42);
    CHECK(transferred);
    CHECK(scripted);

    // JSON roundtrip
    nlohmann::json j;
    to_json(j, cmds[0]);
    EventCommand back;
    from_json(j, back);
    CHECK(back.type == EventCommandType::Message);
}

static void test_database() {
    const fs::path dir = fs::temp_directory_path() / "aether_db_test";
    fs::remove_all(dir);
    auto db = Database::make_default();
    db.actors[0].name = "Aria";
    auto s = db.save_to_directory(dir);
    CHECK(s.is_ok());

    auto loaded = Database::load_from_directory(dir);
    CHECK(loaded.is_ok());
    CHECK(loaded.value().actors[0].name == "Aria");
    CHECK(!loaded.value().items.empty());
    fs::remove_all(dir);
}

static void test_plugin_manifest() {
    const fs::path dir = fs::temp_directory_path() / "aether_plug_test" / "demo";
    fs::create_directories(dir);
    {
        std::ofstream(dir / "plugin.json") << R"({
          "name": "DemoPlug",
          "version": "2.0.0",
          "entry": "main.rb",
          "api_version": 1
        })";
        std::ofstream(dir / "main.rb") << "true\n";
    }
    auto man = PluginLoader::load_manifest(dir);
    CHECK(man.is_ok());
    CHECK(man.value().name == "DemoPlug");
    CHECK(man.value().version == "2.0.0");

    PluginLoader loader;
    CHECK(loader.scan(dir.parent_path()) == 1);
    fs::remove_all(dir.parent_path());
}

int main() {
    test_events();
    test_database();
    test_plugin_manifest();
    if (g_failures == 0) {
        std::puts("OK: game_test passed");
        return 0;
    }
    std::fprintf(stderr, "FAIL: %d check(s) failed\n", g_failures);
    return 1;
}
