/**
 * @file database.hpp
 * @brief Spiel-Datenbank (Helden, Klassen, Gegner, Items, Skills, …) als JSON.
 */
#pragma once

#include <aether/core/types.hpp>

#include <nlohmann/json.hpp>

#include <filesystem>
#include <string>
#include <vector>

namespace aether::game {

struct ClassData {
    u32 id = 0;
    std::string name = "Adventurer";
    i32 base_hp = 100;
    i32 base_mp = 50;
    i32 base_attack = 10;
    i32 base_defense = 10;
    i32 base_speed = 10;
    std::vector<u32> skill_ids;
};

struct ActorData {
    ActorId id = 0;
    std::string name = "Actor";
    u32 class_id = 1;
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

struct AnimationData {
    u32 id = 0;
    std::string name = "Animation";
    std::string graphic;
    i32 frames = 12;
    f32 speed = 1.0f;
};

struct SystemData {
    std::string game_title = "AetherRPG";
    std::vector<std::string> start_bgm;
    u32 start_map_id = 1;
    f32 start_x = 0, start_y = 0, start_z = 0;
    std::string title_bgm = "Theme1";
    std::string battle_bgm;
    std::string victory_me = "Victory";
};

struct Database {
    std::vector<ClassData> classes;
    std::vector<ActorData> actors;
    std::vector<EnemyData> enemies;
    std::vector<ItemData> items;
    std::vector<SkillData> skills;
    std::vector<AnimationData> animations;
    SystemData system;

    [[nodiscard]] static Result<Database> load_from_directory(
        const std::filesystem::path& data_dir);

    [[nodiscard]] Result<void> save_to_directory(
        const std::filesystem::path& data_dir) const;

    static Database make_default();

    [[nodiscard]] const ClassData* find_class(u32 id) const;
    [[nodiscard]] const ActorData* find_actor(u32 id) const;
    [[nodiscard]] const EnemyData* find_enemy(u32 id) const;
    [[nodiscard]] const ItemData* find_item(u32 id) const;
    [[nodiscard]] const SkillData* find_skill(u32 id) const;
};

} // namespace aether::game
