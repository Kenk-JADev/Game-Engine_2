/**
 * @file inventory.hpp
 * @brief Inventar & Party-Zustand (Runtime).
 */
#pragma once

#include <aether/core/types.hpp>
#include <aether/game/database.hpp>

#include <nlohmann/json.hpp>

#include <string>
#include <unordered_map>
#include <vector>

namespace aether::game {

struct InventoryItem {
    u32 item_id = 0;
    i32 count = 0;
};

/**
 * @brief Party-Mitglied zur Laufzeit (abgeleitet von ActorData).
 */
struct PartyMember {
    ActorId actor_id = 0;
    std::string name;
    i32 hp = 100;
    i32 mp = 50;
    i32 max_hp = 100;
    i32 max_mp = 50;
    i32 attack = 10;
    i32 defense = 10;
    i32 level = 1;
    i32 exp = 0;
    bool in_party = true;
};

/**
 * @brief Spieler-Fortschritt: Inventar, Gold, Party, Switches liegen in GameState.
 */
class PartyInventory {
public:
    void setup_from_database(const Database& db);

    // --- Gold ---
    void set_gold(i32 g) noexcept { gold_ = g < 0 ? 0 : g; }
    [[nodiscard]] i32 gold() const noexcept { return gold_; }
    void gain_gold(i32 amount) noexcept { set_gold(gold_ + amount); }
    bool spend_gold(i32 amount) noexcept;

    // --- Items ---
    void gain_item(u32 item_id, i32 amount = 1);
    bool lose_item(u32 item_id, i32 amount = 1);
    [[nodiscard]] i32 item_count(u32 item_id) const noexcept;
    [[nodiscard]] const std::vector<InventoryItem>& items() const noexcept {
        return items_;
    }

    /**
     * @brief Verbraucht Item auf Party-Mitglied (HP/MP aus Database).
     */
    bool use_item(u32 item_id, usize party_index, const Database& db);

    // --- Party ---
    [[nodiscard]] std::vector<PartyMember>& party() noexcept { return party_; }
    [[nodiscard]] const std::vector<PartyMember>& party() const noexcept {
        return party_;
    }
    PartyMember* member(usize index);
    [[nodiscard]] const PartyMember* member(usize index) const;

    void gain_exp(i32 amount);

    // --- Serialization ---
    [[nodiscard]] nlohmann::json to_json() const;
    void from_json(const nlohmann::json& j);

private:
    void compact_items();

    i32 gold_ = 0;
    std::vector<InventoryItem> items_;
    std::vector<PartyMember> party_;
};

} // namespace aether::game
