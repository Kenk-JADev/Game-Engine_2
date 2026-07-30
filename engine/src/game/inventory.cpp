/**
 * @file inventory.cpp
 */
#include <aether/game/inventory.hpp>

#include <algorithm>

namespace aether::game {

void PartyInventory::setup_from_database(const Database& db) {
    party_.clear();
    if (db.actors.empty()) {
        PartyMember m;
        m.actor_id = 1;
        m.name = "Hero";
        party_.push_back(m);
        return;
    }
    // First actor joins party by default
    const auto& a = db.actors.front();
    PartyMember m;
    m.actor_id = a.id;
    m.name = a.name;
    m.max_hp = a.max_hp;
    m.max_mp = a.max_mp;
    m.hp = a.max_hp;
    m.mp = a.max_mp;
    m.attack = a.attack;
    m.defense = a.defense;
    party_.push_back(m);
    gold_ = 100;
}

bool PartyInventory::spend_gold(i32 amount) noexcept {
    if (amount < 0 || gold_ < amount) {
        return false;
    }
    gold_ -= amount;
    return true;
}

void PartyInventory::gain_item(u32 item_id, i32 amount) {
    if (amount == 0) {
        return;
    }
    for (auto& it : items_) {
        if (it.item_id == item_id) {
            it.count += amount;
            if (it.count < 0) {
                it.count = 0;
            }
            compact_items();
            return;
        }
    }
    if (amount > 0) {
        items_.push_back(InventoryItem{item_id, amount});
    }
}

bool PartyInventory::lose_item(u32 item_id, i32 amount) {
    if (amount <= 0) {
        return true;
    }
    for (auto& it : items_) {
        if (it.item_id == item_id) {
            if (it.count < amount) {
                return false;
            }
            it.count -= amount;
            compact_items();
            return true;
        }
    }
    return false;
}

i32 PartyInventory::item_count(u32 item_id) const noexcept {
    for (const auto& it : items_) {
        if (it.item_id == item_id) {
            return it.count;
        }
    }
    return 0;
}

bool PartyInventory::use_item(u32 item_id, usize party_index, const Database& db) {
    if (item_count(item_id) <= 0 || party_index >= party_.size()) {
        return false;
    }
    const ItemData* def = nullptr;
    for (const auto& d : db.items) {
        if (d.id == item_id) {
            def = &d;
            break;
        }
    }
    if (!def) {
        return false;
    }
    auto& m = party_[party_index];
    m.hp = std::min(m.max_hp, m.hp + def->hp_recover);
    m.mp = std::min(m.max_mp, m.mp + def->mp_recover);
    if (def->consumable) {
        lose_item(item_id, 1);
    }
    return true;
}

PartyMember* PartyInventory::member(usize index) {
    return index < party_.size() ? &party_[index] : nullptr;
}

const PartyMember* PartyInventory::member(usize index) const {
    return index < party_.size() ? &party_[index] : nullptr;
}

void PartyInventory::gain_exp(i32 amount) {
    if (amount <= 0 || party_.empty()) {
        return;
    }
    for (auto& m : party_) {
        if (!m.in_party) {
            continue;
        }
        m.exp += amount;
        // simple level curve
        while (m.exp >= m.level * 100) {
            m.exp -= m.level * 100;
            ++m.level;
            m.max_hp += 8;
            m.max_mp += 3;
            m.attack += 2;
            m.defense += 1;
            m.hp = m.max_hp;
            m.mp = m.max_mp;
        }
    }
}

void PartyInventory::compact_items() {
    items_.erase(std::remove_if(items_.begin(), items_.end(),
                                [](const InventoryItem& i) { return i.count <= 0; }),
                 items_.end());
}

nlohmann::json PartyInventory::to_json() const {
    nlohmann::json items = nlohmann::json::array();
    for (const auto& it : items_) {
        items.push_back({{"id", it.item_id}, {"count", it.count}});
    }
    nlohmann::json party = nlohmann::json::array();
    for (const auto& m : party_) {
        party.push_back({{"actor_id", m.actor_id},
                         {"name", m.name},
                         {"hp", m.hp},
                         {"mp", m.mp},
                         {"max_hp", m.max_hp},
                         {"max_mp", m.max_mp},
                         {"attack", m.attack},
                         {"defense", m.defense},
                         {"level", m.level},
                         {"exp", m.exp},
                         {"in_party", m.in_party}});
    }
    return nlohmann::json{{"gold", gold_}, {"items", items}, {"party", party}};
}

void PartyInventory::from_json(const nlohmann::json& j) {
    gold_ = j.value("gold", 0);
    items_.clear();
    if (j.contains("items") && j["items"].is_array()) {
        for (const auto& it : j["items"]) {
            items_.push_back(
                InventoryItem{it.value("id", 0u), it.value("count", 0)});
        }
    }
    party_.clear();
    if (j.contains("party") && j["party"].is_array()) {
        for (const auto& pj : j["party"]) {
            PartyMember m;
            m.actor_id = pj.value("actor_id", 0u);
            m.name = pj.value("name", "Hero");
            m.hp = pj.value("hp", 100);
            m.mp = pj.value("mp", 50);
            m.max_hp = pj.value("max_hp", 100);
            m.max_mp = pj.value("max_mp", 50);
            m.attack = pj.value("attack", 10);
            m.defense = pj.value("defense", 10);
            m.level = pj.value("level", 1);
            m.exp = pj.value("exp", 0);
            m.in_party = pj.value("in_party", true);
            party_.push_back(m);
        }
    }
}

} // namespace aether::game
