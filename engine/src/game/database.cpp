/**
 * @file database.cpp
 */
#include <aether/game/database.hpp>
#include <aether/core/logger.hpp>

#include <fstream>

namespace aether::game {
namespace fs = std::filesystem;

namespace {

template <typename T>
bool read_array_file(const fs::path& path, std::vector<T>& out, void (*from_j)(const nlohmann::json&, T&)) {
    std::ifstream in(path);
    if (!in) return false;
    try {
        nlohmann::json j;
        in >> j;
        out.clear();
        if (!j.is_array()) return false;
        for (const auto& el : j) {
            T item{};
            from_j(el, item);
            out.push_back(std::move(item));
        }
        return true;
    } catch (...) {
        return false;
    }
}

void actor_from(const nlohmann::json& j, ActorData& a) {
    a.id = j.value("id", 0u);
    a.name = j.value("name", "Actor");
    a.class_name = j.value("class_name", "Adventurer");
    a.max_hp = j.value("max_hp", 100);
    a.max_mp = j.value("max_mp", 50);
    a.attack = j.value("attack", 10);
    a.defense = j.value("defense", 10);
    a.speed = j.value("speed", 10);
    a.character_graphic = j.value("character_graphic", "");
}

nlohmann::json actor_to(const ActorData& a) {
    return {{"id", a.id},
            {"name", a.name},
            {"class_name", a.class_name},
            {"max_hp", a.max_hp},
            {"max_mp", a.max_mp},
            {"attack", a.attack},
            {"defense", a.defense},
            {"speed", a.speed},
            {"character_graphic", a.character_graphic}};
}

void enemy_from(const nlohmann::json& j, EnemyData& e) {
    e.id = j.value("id", 0u);
    e.name = j.value("name", "Enemy");
    e.max_hp = j.value("max_hp", 50);
    e.attack = j.value("attack", 8);
    e.defense = j.value("defense", 5);
    e.exp = j.value("exp", 10);
    e.gold = j.value("gold", 5);
    e.battler_graphic = j.value("battler_graphic", "");
}

nlohmann::json enemy_to(const EnemyData& e) {
    return {{"id", e.id},
            {"name", e.name},
            {"max_hp", e.max_hp},
            {"attack", e.attack},
            {"defense", e.defense},
            {"exp", e.exp},
            {"gold", e.gold},
            {"battler_graphic", e.battler_graphic}};
}

void item_from(const nlohmann::json& j, ItemData& i) {
    i.id = j.value("id", 0u);
    i.name = j.value("name", "Item");
    i.description = j.value("description", "");
    i.price = j.value("price", 10);
    i.consumable = j.value("consumable", true);
    i.hp_recover = j.value("hp_recover", 0);
    i.mp_recover = j.value("mp_recover", 0);
}

nlohmann::json item_to(const ItemData& i) {
    return {{"id", i.id},
            {"name", i.name},
            {"description", i.description},
            {"price", i.price},
            {"consumable", i.consumable},
            {"hp_recover", i.hp_recover},
            {"mp_recover", i.mp_recover}};
}

void skill_from(const nlohmann::json& j, SkillData& s) {
    s.id = j.value("id", 0u);
    s.name = j.value("name", "Skill");
    s.description = j.value("description", "");
    s.mp_cost = j.value("mp_cost", 5);
    s.power = j.value("power", 10);
    s.scope = j.value("scope", "one_enemy");
}

nlohmann::json skill_to(const SkillData& s) {
    return {{"id", s.id},
            {"name", s.name},
            {"description", s.description},
            {"mp_cost", s.mp_cost},
            {"power", s.power},
            {"scope", s.scope}};
}

} // namespace

Database Database::make_default() {
    Database db;
    ActorData hero;
    hero.id = 1;
    hero.name = "Hero";
    db.actors.push_back(hero);

    EnemyData slime;
    slime.id = 1;
    slime.name = "Slime";
    db.enemies.push_back(slime);

    ItemData potion;
    potion.id = 1;
    potion.name = "Potion";
    potion.hp_recover = 50;
    db.items.push_back(potion);

    SkillData fire;
    fire.id = 1;
    fire.name = "Fire";
    fire.mp_cost = 5;
    fire.power = 20;
    db.skills.push_back(fire);

    db.system.game_title = "New Project";
    return db;
}

Result<Database> Database::load_from_directory(const fs::path& data_dir) {
    Database db = make_default();
    std::error_code ec;
    if (!fs::exists(data_dir, ec)) {
        return Result<Database>::ok(std::move(db));
    }

    read_array_file(data_dir / "actors.json", db.actors, actor_from);
    read_array_file(data_dir / "enemies.json", db.enemies, enemy_from);
    read_array_file(data_dir / "items.json", db.items, item_from);
    read_array_file(data_dir / "skills.json", db.skills, skill_from);

    const fs::path sys = data_dir / "system.json";
    if (fs::exists(sys, ec)) {
        try {
            std::ifstream in(sys);
            nlohmann::json j;
            in >> j;
            db.system.game_title = j.value("game_title", db.system.game_title);
            db.system.start_map_id = j.value("start_map_id", 1u);
            db.system.start_x = j.value("start_x", 0.0f);
            db.system.start_y = j.value("start_y", 0.0f);
            db.system.start_z = j.value("start_z", 0.0f);
        } catch (...) {
            // keep defaults
        }
    }

    core::log_info("Database", "Loaded from " + data_dir.string());
    return Result<Database>::ok(std::move(db));
}

Result<void> Database::save_to_directory(const fs::path& data_dir) const {
    std::error_code ec;
    fs::create_directories(data_dir, ec);

    auto write_arr = [&](const char* name, auto&& to_fn, const auto& vec) -> bool {
        nlohmann::json arr = nlohmann::json::array();
        for (const auto& el : vec) arr.push_back(to_fn(el));
        std::ofstream out(data_dir / name);
        if (!out) return false;
        out << arr.dump(2) << '\n';
        return true;
    };

    if (!write_arr("actors.json", actor_to, actors))
        return Result<void>::fail("Cannot write actors.json");
    if (!write_arr("enemies.json", enemy_to, enemies))
        return Result<void>::fail("Cannot write enemies.json");
    if (!write_arr("items.json", item_to, items))
        return Result<void>::fail("Cannot write items.json");
    if (!write_arr("skills.json", skill_to, skills))
        return Result<void>::fail("Cannot write skills.json");

    {
        nlohmann::json j{{"game_title", system.game_title},
                         {"start_map_id", system.start_map_id},
                         {"start_x", system.start_x},
                         {"start_y", system.start_y},
                         {"start_z", system.start_z}};
        std::ofstream out(data_dir / "system.json");
        if (!out) return Result<void>::fail("Cannot write system.json");
        out << j.dump(2) << '\n';
    }

    return Result<void>::ok();
}

} // namespace aether::game
