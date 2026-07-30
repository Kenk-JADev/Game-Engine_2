/**
 * @file shop.hpp
 * @brief Einfacher Händler (kaufen/verkaufen über Item-IDs).
 */
#pragma once

#include <aether/core/types.hpp>
#include <aether/game/database.hpp>
#include <aether/game/inventory.hpp>

#include <string>
#include <vector>

namespace aether::game {

struct ShopOffer {
    u32 item_id = 0;
    i32 price_override = -1; ///< <0 = Database-Preis
};

class Shop {
public:
    void set_name(std::string n) { name_ = std::move(n); }
    [[nodiscard]] const std::string& name() const noexcept { return name_; }

    void set_offers(std::vector<ShopOffer> offers) { offers_ = std::move(offers); }
    [[nodiscard]] const std::vector<ShopOffer>& offers() const noexcept {
        return offers_;
    }

    [[nodiscard]] i32 price_of(const ShopOffer& o, const Database& db) const;
    [[nodiscard]] i32 sell_price(u32 item_id, const Database& db) const;

    bool buy(usize offer_index, PartyInventory& inv, const Database& db);
    bool sell(u32 item_id, i32 count, PartyInventory& inv, const Database& db);

private:
    std::string name_ = "Shop";
    std::vector<ShopOffer> offers_;
};

} // namespace aether::game
