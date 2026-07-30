/**
 * @file scene_stack.cpp
 */
#include <aether/game/scene_stack.hpp>
#include <aether/core/logger.hpp>

namespace aether::game {

// ---- Title ------------------------------------------------------------------

void TitleScene::on_enter(GameContext& ctx) {
    ctx.title_index = 0;
    ctx.status_line = "AetherRPG – Titel";
    core::log_info("Scene", "Title");
}

void TitleScene::update(GameContext& ctx) {
    if (!ctx.input) {
        return;
    }
    auto& in = *ctx.input;
    if (in.was_pressed("up")) {
        ctx.title_index = (ctx.title_index + 2) % 3;
    }
    if (in.was_pressed("down")) {
        ctx.title_index = (ctx.title_index + 1) % 3;
    }
    if (in.was_pressed("confirm")) {
        if (ctx.title_index == 0) {
            ctx.request_new_game = true;
        } else if (ctx.title_index == 1) {
            ctx.request_continue = true;
            ctx.save_mode = false;
        } else {
            ctx.request_quit = true;
        }
    }
}

void TitleScene::draw_overlay(GameContext& ctx) {
    static const char* items[] = {"Neues Spiel", "Fortsetzen", "Beenden"};
    ctx.status_line = std::string("TITEL > ") + items[ctx.title_index];
}

// ---- Menu -------------------------------------------------------------------

void MenuScene::on_enter(GameContext& ctx) {
    ctx.menu_index = 0;
    core::log_info("Scene", "Menu");
}

void MenuScene::update(GameContext& ctx) {
    if (!ctx.input) {
        return;
    }
    auto& in = *ctx.input;
    const int n = static_cast<int>(items_.size());
    if (in.was_pressed("up")) {
        ctx.menu_index = (ctx.menu_index + n - 1) % n;
    }
    if (in.was_pressed("down")) {
        ctx.menu_index = (ctx.menu_index + 1) % n;
    }
    if (in.was_pressed("cancel")) {
        if (callback_) {
            callback_("close");
        }
        return;
    }
    if (in.was_pressed("confirm") && callback_) {
        callback_(items_[static_cast<usize>(ctx.menu_index)]);
    }
}

void MenuScene::draw_overlay(GameContext& ctx) {
    ctx.status_line = "MENÜ > " + items_[static_cast<usize>(ctx.menu_index)];
}

// ---- Dialog -----------------------------------------------------------------

void DialogScene::on_enter(GameContext& ctx) {
    ctx.dialog_open = true;
    ctx.dialog_index = 0;
    core::log_info("Scene", "Dialog");
}

void DialogScene::update(GameContext& ctx) {
    if (!ctx.input) {
        return;
    }
    if (ctx.input->was_pressed("confirm") || ctx.input->was_pressed("cancel")) {
        ++ctx.dialog_index;
        if (ctx.dialog_index >= static_cast<int>(ctx.dialog_lines.size())) {
            ctx.dialog_open = false;
            ctx.dialog_lines.clear();
            ctx.dialog_index = 0;
        }
    }
}

void DialogScene::draw_overlay(GameContext& ctx) {
    if (ctx.dialog_index >= 0 &&
        ctx.dialog_index < static_cast<int>(ctx.dialog_lines.size())) {
        ctx.status_line = "DIALOG: " + ctx.dialog_lines[static_cast<usize>(ctx.dialog_index)];
    } else {
        ctx.status_line = "DIALOG";
    }
}

// ---- Save/Load --------------------------------------------------------------

void SaveLoadScene::on_enter(GameContext& ctx) {
    ctx.saveload_index = 0;
    core::log_info("Scene", ctx.save_mode ? "Save" : "Load");
}

void SaveLoadScene::update(GameContext& ctx) {
    if (!ctx.input) {
        return;
    }
    auto& in = *ctx.input;
    constexpr int kSlots = 5;
    if (in.was_pressed("up")) {
        ctx.saveload_index = (ctx.saveload_index + kSlots - 1) % kSlots;
    }
    if (in.was_pressed("down")) {
        ctx.saveload_index = (ctx.saveload_index + 1) % kSlots;
    }
    if (in.was_pressed("cancel")) {
        if (callback_) {
            callback_(-1, ctx.save_mode);
        }
        return;
    }
    if (in.was_pressed("confirm") && callback_) {
        callback_(ctx.saveload_index + 1, ctx.save_mode);
    }
}

void SaveLoadScene::draw_overlay(GameContext& ctx) {
    ctx.status_line = std::string(ctx.save_mode ? "SPEICHERN" : "LADEN") + " > Slot " +
                      std::to_string(ctx.saveload_index + 1);
}

// ---- Map --------------------------------------------------------------------

void MapScene::update(GameContext& /*ctx*/) {
    // Runtime owns map logic
}

void MapScene::draw_overlay(GameContext& ctx) {
    if (ctx.status_line.empty()) {
        ctx.status_line = "MAP";
    }
}

// ---- Choice -----------------------------------------------------------------

void ChoiceScene::on_enter(GameContext& ctx) {
    ctx.choice_open = true;
    ctx.choice_index = 0;
    ctx.choice_result = -1;
    core::log_info("Scene", "Choice");
}

void ChoiceScene::update(GameContext& ctx) {
    if (!ctx.input || ctx.choice_labels.empty()) {
        return;
    }
    const int n = static_cast<int>(ctx.choice_labels.size());
    if (ctx.input->was_pressed("up")) {
        ctx.choice_index = (ctx.choice_index + n - 1) % n;
    }
    if (ctx.input->was_pressed("down")) {
        ctx.choice_index = (ctx.choice_index + 1) % n;
    }
    if (ctx.input->was_pressed("confirm")) {
        ctx.choice_result = ctx.choice_index;
        ctx.choice_open = false;
    }
    if (ctx.input->was_pressed("cancel")) {
        ctx.choice_result = n - 1;
        ctx.choice_open = false;
    }
}

void ChoiceScene::draw_overlay(GameContext& ctx) {
    std::string s = "AUSWAHL";
    for (int i = 0; i < static_cast<int>(ctx.choice_labels.size()); ++i) {
        s += (i == ctx.choice_index ? " > " : "   ");
        s += ctx.choice_labels[static_cast<usize>(i)];
    }
    ctx.status_line = s;
}

// ---- Battle scene shell -----------------------------------------------------

void BattleScene::on_enter(GameContext& ctx) {
    ctx.battle_done_ack = false;
    core::log_info("Scene", "Battle");
}

void BattleScene::update(GameContext& ctx) {
    if (tick_) {
        tick_(ctx);
    }
}

void BattleScene::draw_overlay(GameContext& ctx) {
    if (ctx.status_line.empty()) {
        ctx.status_line = "KAMPF";
    }
}

// ---- Stack ------------------------------------------------------------------

void GameSceneStack::clear() {
    stack_.clear();
}

void GameSceneStack::push(std::unique_ptr<IGameScene> scene, GameContext& ctx) {
    if (!scene) {
        return;
    }
    stack_.push_back(std::move(scene));
    stack_.back()->on_enter(ctx);
}

void GameSceneStack::pop(GameContext& ctx) {
    if (stack_.empty()) {
        return;
    }
    stack_.back()->on_exit(ctx);
    stack_.pop_back();
}

void GameSceneStack::replace(std::unique_ptr<IGameScene> scene, GameContext& ctx) {
    clear();
    push(std::move(scene), ctx);
}

IGameScene* GameSceneStack::current() noexcept {
    return stack_.empty() ? nullptr : stack_.back().get();
}

const IGameScene* GameSceneStack::current() const noexcept {
    return stack_.empty() ? nullptr : stack_.back().get();
}

void GameSceneStack::update(GameContext& ctx) {
    if (auto* s = current()) {
        s->update(ctx);
    }
}

void GameSceneStack::fixed_update(GameContext& ctx) {
    // Only bottom-most Map gets fixed update if menu is on top? 
    // Call all from bottom for map physics when map is under menu – only map.
    for (auto& s : stack_) {
        if (s->id() == GameSceneId::Map) {
            s->fixed_update(ctx);
        }
    }
    if (auto* s = current()) {
        if (s->id() != GameSceneId::Map) {
            s->fixed_update(ctx);
        }
    }
}

void GameSceneStack::draw_overlay(GameContext& ctx) {
    // draw from bottom to top
    for (auto& s : stack_) {
        s->draw_overlay(ctx);
    }
}

} // namespace aether::game
