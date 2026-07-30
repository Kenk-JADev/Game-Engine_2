/**
 * @file save_inv_test.cpp
 */
#include <aether/game/inventory.hpp>
#include <aether/game/save_system.hpp>
#include <aether/game/scene_stack.hpp>
#include <aether/game/shop.hpp>
#include <aether/input/input_module.hpp>

#include <cstdio>
#include <filesystem>

using namespace aether;
using namespace aether::game;
using namespace aether::input;

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
    Database db = Database::make_default();
    PartyInventory inv;
    inv.setup_from_database(db);
    CHECK(!inv.party().empty());
    CHECK(inv.gold() == 100);

    inv.gain_item(1, 5);
    CHECK(inv.item_count(1) == 5);
    CHECK(inv.use_item(1, 0, db));
    CHECK(inv.item_count(1) == 4);
    CHECK(inv.party()[0].hp == inv.party()[0].max_hp); // already full + potion

    inv.gain_exp(250);
    CHECK(inv.party()[0].level >= 2);

    // Shop
    Shop shop;
    shop.set_offers({{1, 10}});
    CHECK(shop.buy(0, inv, db));
    CHECK(inv.gold() == 90);
    CHECK(inv.item_count(1) == 5);
    CHECK(shop.sell(1, 1, inv, db));
    CHECK(inv.item_count(1) == 4);

    // Save/Load
    const auto dir = std::filesystem::temp_directory_path() / "aether_save_test";
    std::filesystem::remove_all(dir);
    SaveSystem saves(dir);
    GameState st;
    st.set_switch(3, true);
    st.set_variable(2, 42);
    WeatherSystem weather;
    weather.set(WeatherType::Rain, 4.0f, 0.0f);

    auto data = SaveSystem::capture("Test", 1, "Map", {1, 0, 2}, 8, inv, st, weather, 12);
    auto sr = saves.save(1, data);
    CHECK(sr.is_ok());
    CHECK(saves.slot_exists(1));

    auto loaded = saves.load(1);
    CHECK(loaded.is_ok());
    CHECK(loaded.value().map_id == 1);
    CHECK(std::fabs(loaded.value().player_pos.x - 1.0f) < 0.01f);
    CHECK(loaded.value().inventory.gold() == inv.gold());

    GameState st2;
    PartyInventory inv2;
    WeatherSystem w2;
    SaveSystem::apply_state(loaded.value(), st2, inv2, w2);
    CHECK(st2.get_switch(3));
    CHECK(st2.get_variable(2) == 42);
    CHECK(w2.state().type == WeatherType::Rain);

    // Scene stack
    GameContext gctx;
    InputManager input_mgr;
    input_mgr.register_default_rpg_actions();
    gctx.input = &input_mgr;
    GameSceneStack stack;
    stack.push(std::make_unique<TitleScene>(), gctx);
    CHECK(stack.current()->id() == GameSceneId::Title);
    input_mgr.begin_frame();
    input_mgr.feed_key(Key::Enter, Action::Press);
    stack.update(gctx);
    CHECK(gctx.request_new_game);

    std::filesystem::remove_all(dir);

    if (g_failures == 0) {
        std::puts("OK: save_inv_test passed");
        return 0;
    }
    std::fprintf(stderr, "FAIL: %d\n", g_failures);
    return 1;
}
