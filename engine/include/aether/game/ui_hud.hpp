/**
 * @file ui_hud.hpp
 * @brief Text-/Panel-HUD für Dialog, Menü, Kampf (log + optionale ImGui-Zeichnung).
 *
 * Zeichnet keine OpenGL-Primitives selbst – liefert strukturierte HUD-State
 * für Runtime-Overlay und Editor-Testspiel. ImGui optional.
 */
#pragma once

#include <aether/core/types.hpp>
#include <aether/game/battle.hpp>
#include <aether/game/inventory.hpp>
#include <aether/game/quest.hpp>
#include <aether/game/scene_stack.hpp>

#include <string>
#include <vector>

namespace aether::game {

struct HudPanel {
    std::string title;
    std::vector<std::string> lines;
    int cursor = -1; ///< highlighted line
    bool visible = false;
};

struct HudState {
    HudPanel dialog;
    HudPanel menu;
    HudPanel battle;
    HudPanel quest;
    HudPanel status; ///< HP/gold one-liner
    std::string toast;
};

/**
 * @brief Baut HudState aus GameContext + optional Battle/Quest/Party.
 */
class HudBuilder {
public:
    void set_party(const PartyInventory* inv) { inv_ = inv; }
    void set_quests(const QuestLog* q) { quests_ = q; }
    void set_battle(const Battle* b) { battle_ = b; }

    [[nodiscard]] HudState build(const GameContext& ctx) const;

    /**
     * @brief Zeichnet mit Dear ImGui wenn AETHER_WITH_IMGUI, sonst no-op.
     * @return true wenn gezeichnet
     */
    static bool draw_imgui(const HudState& hud);

private:
    const PartyInventory* inv_ = nullptr;
    const QuestLog* quests_ = nullptr;
    const Battle* battle_ = nullptr;
};

} // namespace aether::game
