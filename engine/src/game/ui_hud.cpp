/**
 * @file ui_hud.cpp
 */
#include <aether/game/ui_hud.hpp>

#if defined(AETHER_WITH_IMGUI)
#  include <imgui.h>
#endif

namespace aether::game {

HudState HudBuilder::build(const GameContext& ctx) const {
    HudState h;

    // Status bar
    h.status.visible = true;
    if (inv_ && !inv_->party().empty()) {
        const auto& m = inv_->party().front();
        h.status.lines = {m.name + "  Lv" + std::to_string(m.level) + "  HP " +
                          std::to_string(m.hp) + "/" + std::to_string(m.max_hp) +
                          "  MP " + std::to_string(m.mp) + "/" + std::to_string(m.max_mp) +
                          "  Gold " + std::to_string(inv_->gold())};
    } else {
        h.status.lines = {ctx.status_line.empty() ? "AetherRPG" : ctx.status_line};
    }

    // Dialog
    if (ctx.dialog_open && !ctx.dialog_lines.empty()) {
        h.dialog.visible = true;
        h.dialog.title = "Dialog";
        if (ctx.dialog_index >= 0 &&
            ctx.dialog_index < static_cast<int>(ctx.dialog_lines.size())) {
            h.dialog.lines = {ctx.dialog_lines[static_cast<usize>(ctx.dialog_index)]};
        }
        h.dialog.lines.push_back("[Z/Enter] Weiter");
    }

    // Battle
    if (battle_ && !battle_->finished()) {
        h.battle.visible = true;
        h.battle.title = "Kampf";
        h.battle.lines.push_back(battle_->status_line());
        for (const auto& l : battle_->log()) {
            h.battle.lines.push_back(l);
        }
        if (battle_->phase() == BattlePhase::PlayerChoose) {
            for (int i = 0; i < 4; ++i) {
                std::string row = (i == battle_->menu_index() ? "> " : "  ");
                row += battle_->menu_label(i);
                h.battle.lines.push_back(row);
            }
            h.battle.cursor = static_cast<int>(h.battle.lines.size()) - 4 + battle_->menu_index();
        }
    } else if (battle_ && battle_->finished()) {
        h.battle.visible = true;
        h.battle.title = "Kampf-Ende";
        h.battle.lines.push_back(battle_->status_line());
        h.battle.lines.push_back("[Z] Weiter");
    }

    // Quests
    if (quests_) {
        auto active = quests_->active_list();
        if (!active.empty()) {
            h.quest.visible = true;
            h.quest.title = "Quests";
            for (const auto& q : active) {
                const auto* def = quests_->find_def(q.id);
                std::string t = def ? def->title : q.id;
                h.quest.lines.push_back("• " + t + " (" + std::to_string(q.counter) +
                                        "/" + std::to_string(q.target) + ")");
            }
        }
    }

    // Generic menu/title from status when no dialog/battle
    if (!h.dialog.visible && !h.battle.visible && !ctx.status_line.empty()) {
        if (ctx.status_line.rfind("MENÜ", 0) == 0 || ctx.status_line.rfind("TITEL", 0) == 0 ||
            ctx.status_line.rfind("SPEICHERN", 0) == 0 ||
            ctx.status_line.rfind("LADEN", 0) == 0) {
            h.menu.visible = true;
            h.menu.title = "Menü";
            h.menu.lines = {ctx.status_line};
        }
    }

    h.toast = ctx.status_line;
    return h;
}

bool HudBuilder::draw_imgui(const HudState& hud) {
#if defined(AETHER_WITH_IMGUI)
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    const ImVec2 work = vp->WorkSize;
    const ImVec2 pos = vp->WorkPos;

    // Status top
    if (hud.status.visible && !hud.status.lines.empty()) {
        ImGui::SetNextWindowPos(ImVec2(pos.x + 8, pos.y + 8), ImGuiCond_Always);
        ImGui::SetNextWindowBgAlpha(0.55f);
        ImGui::Begin("##hud_status", nullptr,
                     ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
                         ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove);
        for (const auto& l : hud.status.lines) {
            ImGui::TextUnformatted(l.c_str());
        }
        ImGui::End();
    }

    // Quest top-right
    if (hud.quest.visible) {
        ImGui::SetNextWindowPos(ImVec2(pos.x + work.x - 280, pos.y + 8), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(270, 0), ImGuiCond_Always);
        ImGui::SetNextWindowBgAlpha(0.6f);
        ImGui::Begin("Quests", nullptr,
                     ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
                         ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove);
        ImGui::TextUnformatted(hud.quest.title.c_str());
        ImGui::Separator();
        for (const auto& l : hud.quest.lines) {
            ImGui::TextWrapped("%s", l.c_str());
        }
        ImGui::End();
    }

    // Dialog bottom
    if (hud.dialog.visible) {
        ImGui::SetNextWindowPos(ImVec2(pos.x + work.x * 0.1f, pos.y + work.y - 140),
                                ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(work.x * 0.8f, 120), ImGuiCond_Always);
        ImGui::SetNextWindowBgAlpha(0.85f);
        ImGui::Begin("##dialog", nullptr,
                     ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoNav |
                         ImGuiWindowFlags_NoMove);
        for (const auto& l : hud.dialog.lines) {
            ImGui::TextWrapped("%s", l.c_str());
        }
        ImGui::End();
    }

    // Battle center
    if (hud.battle.visible) {
        ImGui::SetNextWindowPos(ImVec2(pos.x + work.x * 0.2f, pos.y + work.y * 0.25f),
                                ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(work.x * 0.6f, work.y * 0.45f), ImGuiCond_Always);
        ImGui::SetNextWindowBgAlpha(0.8f);
        ImGui::Begin("Kampf", nullptr,
                     ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoNav |
                         ImGuiWindowFlags_NoMove);
        for (int i = 0; i < static_cast<int>(hud.battle.lines.size()); ++i) {
            if (i == hud.battle.cursor) {
                ImGui::TextColored(ImVec4(1, 1, 0.3f, 1), "%s",
                                   hud.battle.lines[static_cast<usize>(i)].c_str());
            } else {
                ImGui::TextUnformatted(hud.battle.lines[static_cast<usize>(i)].c_str());
            }
        }
        ImGui::End();
    }

    // Menu toast
    if (hud.menu.visible) {
        ImGui::SetNextWindowPos(ImVec2(pos.x + work.x * 0.35f, pos.y + work.y * 0.35f),
                                ImGuiCond_Always);
        ImGui::SetNextWindowBgAlpha(0.8f);
        ImGui::Begin("##menu", nullptr,
                     ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
                         ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove);
        for (const auto& l : hud.menu.lines) {
            ImGui::TextUnformatted(l.c_str());
        }
        ImGui::End();
    }
    return true;
#else
    (void)hud;
    return false;
#endif
}

} // namespace aether::game
