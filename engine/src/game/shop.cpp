/**
 * @file shop.cpp
 */
#include <aether/game/shop.hpp>

namespace aether::game {
namespace {

const ItemData* find_item(const Database& db, u32 id) {
    for (const auto& it : db.items) {
        if (it.id == id) {
            return &it;
        }
    }
    return nullptr;
}

} // namespace

i32 Shop::price_of(const ShopOffer& o, const Database& db) const {
    if (o.price_override >= 0) {
        return o.price_override;
    }
    if (const auto* d = find_item(db, o.item_id)) {
        return d->price;
    }
    return 0;
}

i32 Shop::sell_price(u32 item_id, const Database& db) const {
    if (const auto* d = find_item(db, item_id)) {
        return d->price / 2;
    }
    return 0;
}

bool Shop::buy(usize offer_index, PartyInventory& inv, const Database& db) {
    if (offer_index >= offers_.size()) {
        return false;
    }
    const auto& o = offers_[offer_index];
    const i32 price = price_of(o, db);
    if (!inv.spend_gold(price)) {
        return false;
    }
    inv.gain_item(o.item_id, 1);
    return true;
}

bool Shop::sell(u32 item_id, i32 count, PartyInventory& inv, const Database& db) {
    if (count <= 0 || inv.item_count(item_id) < count) {
        return false;
    }
    if (!inv.lose_item(item_id, count)) {
        return false;
    }
    inv.gain_gold(sell_price(item_id, db) * count);
    return true;
}

} // namespace aether::game
