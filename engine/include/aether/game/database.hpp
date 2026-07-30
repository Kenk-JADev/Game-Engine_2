/**
 * @file database.hpp
 * @brief Spiel-Datenbank (Helden, Gegner, Items, Skills, …) als JSON.
 *
 * Der Editor zeigt Tabs – keine Engine-Components.
 */
#pragma once

#include <aether/core/types.hpp>

#include <nlohmann/json.hpp>

#include <filesystem>
#include <string>
#include <vector>

namespace aether::game {

struct ActorData {
    ActorId id = 0;
    std::string name = "Actor";
    std::string class_name = "Adventurer";
    i32 max_hp = 100;
    i32 max_mp = 50;
    i32 attack = 10;
    i32 defense = 10;
    i32 speed = 10;
    std::string character_graphic;
};

struct EnemyData {
    u32 id = 0;
    std::string name = "Enemy";
    i32 max_hp = 50;
    i32 attack = 8;
    i32 defense = 5;
    i32 exp = 10;
    i32 gold = 5;
    std::string battler_graphic;
};

struct ItemData {
    u32 id = 0;
    std::string name = "Item";
    std::string description;
    i32 price = 10;
    bool consumable = true;
    i32 hp_recover = 0;
    i32 mp_recover = 0;
};

struct SkillData {
    u32 id = 0;
    std::string name = "Skill";
    std::string description;
    i32 mp_cost = 5;
    i32 power = 10;
    std::string scope = "one_enemy";
};

struct SystemData {
    std::string game_title = "AetherRPG";
    std::vector<std::string> start_bgm;
    u32 start_map_id = 1;
    f32 start_x = 0, start_y = 0, start_z = 0;
};

/**
 * @brief Gesamte Datenbank eines Projekts.
 */
struct Database {
    std::vector<ActorData> actors;
    std::vector<EnemyData> enemies;
    std::vector<ItemData> items;
    std::vector<SkillData> skills;
    SystemData system;

    /**
     * @brief Lädt JSON-Datenbankdateien aus einem Projekt-data-Ordner.
     */
    [[nodiscard]] static Result<Database> load_from_directory(
        const std::filesystem::path& data_dir);

    /**
     * @brief Speichert die Datenbank als JSON-Dateien im data-Ordner.
     */
    [[nodiscard]] Result<void> save_to_directory(
        const std::filesystem::path& data_dir) const;

    /** @brief Legt leere Standard-Einträge an (neues Projekt). */
    static Database make_default();
};

} // namespace aether::game
