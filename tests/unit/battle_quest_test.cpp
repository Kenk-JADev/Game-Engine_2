/**
 * @file battle_quest_test.cpp
 */
#include <aether/game/battle.hpp>
#include <aether/game/event_system.hpp>
#include <aether/game/inventory.hpp>
#include <aether/game/quest.hpp>
#include <aether/game/ui_hud.hpp>
#include <aether/input/input_module.hpp>

#include <cstdio>

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
    // Quest
    QuestLog ql;
    ql.load_defaults();
    CHECK(ql.start("main_001"));
    CHECK(ql.active("main_001"));
    CHECK(ql.complete("main_001"));
    CHECK(ql.completed("main_001"));

    // Battle attack until win
    Database db = Database::make_default();
    PartyInventory inv;
    inv.setup_from_database(db);
    Battle battle;
    battle.start(inv.party().front(), db.enemies.front(),
                 db.skills.empty() ? nullptr : &db.skills.front());
    CHECK(!battle.finished());

    InputManager input;
    input.register_default_rpg_actions();
    auto confirm_once = [&](Battle& b) {
        input.begin_frame();
        input.feed_key(Key::Z, Action::Press);
        b.update(input);
        input.feed_key(Key::Z, Action::Release);
        input.end_frame();
    };
    int guard = 0;
    while (!battle.finished() && guard++ < 200) {
        confirm_once(battle);
    }
    CHECK(battle.finished());

    // Force win path: weak enemy
    EnemyData slime = db.enemies.front();
    slime.max_hp = 1;
    slime.attack = 0;
    Battle b2;
    PartyMember hero = inv.party().front();
    hero.attack = 50;
    b2.start(hero, slime, nullptr);
    confirm_once(b2);
    CHECK(b2.finished());
    CHECK(b2.result().won);

    // Event choice + battle request
    GameState st;
    EventInterpreter interp(&st);
    EventCommand choice;
    choice.type = EventCommandType::Choice;
    nlohmann::json branch_a = nlohmann::json::array();
    branch_a.push_back({{"type", "message"}, {"params", {{"text", "A"}}}});
    nlohmann::json branch_b = nlohmann::json::array();
    branch_b.push_back({{"type", "battle"}, {"params", {{"enemy_id", 1}}}});
    choice.params = nlohmann::json{{"options", nlohmann::json::array({"A", "B"})},
                                   {"branches", nlohmann::json::array({branch_a, branch_b})}};
    interp.start({choice});
    CHECK(interp.update()); // yields on choice
    CHECK(interp.is_waiting_for_choice());
    interp.resume_choice(1);
    interp.update();
    CHECK(interp.pending_request().kind == EventRequest::Kind::Battle);

    // HUD build
    HudBuilder hud;
    hud.set_party(&inv);
    hud.set_quests(&ql);
    GameContext ctx;
    ctx.status_line = "MAP";
    auto hs = hud.build(ctx);
    CHECK(hs.status.visible);
    CHECK(!hs.status.lines.empty());

    if (g_failures == 0) {
        std::puts("OK: battle_quest_test passed");
        return 0;
    }
    std::fprintf(stderr, "FAIL: %d\n", g_failures);
    return 1;
}
