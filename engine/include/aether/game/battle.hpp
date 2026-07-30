/**
 * @file battle.hpp
 * @brief Einfacher turn-basierter Kampf (stilisierte RPGs).
 */
#pragma once

#include <aether/core/types.hpp>
#include <aether/game/database.hpp>
#include <aether/game/inventory.hpp>
#include <aether/input/input_manager.hpp>

#include <functional>
#include <string>
#include <vector>

namespace aether::game {

enum class BattleAction {
    Attack,
    Skill,
    Item,
    Defend,
    Flee,
};

enum class BattlePhase {
    Intro,
    PlayerChoose,
    PlayerAct,
    EnemyAct,
    Won,
    Lost,
    Fled,
};

struct BattleFighter {
    std::string name;
    i32 hp = 10;
    i32 max_hp = 10;
    i32 mp = 0;
    i32 max_mp = 0;
    i32 attack = 5;
    i32 defense = 3;
    i32 skill_power = 0;
    i32 skill_mp = 0;
    bool defending = false;
    bool is_enemy = false;
};

struct BattleResult {
    bool won = false;
    bool fled = false;
    i32 exp = 0;
    i32 gold = 0;
};

/**
 * @brief Ein Kampf. update() verarbeitet Input und Züge.
 */
class Battle {
public:
    using LogFn = std::function<void(const std::string&)>;

    void set_log(LogFn fn) { log_ = std::move(fn); }

    /**
     * @brief Startet Kampf: Party[0] vs EnemyData.
     */
    void start(const PartyMember& hero, const EnemyData& enemy,
               const SkillData* skill = nullptr);

    void update(const input::InputManager& input);

    [[nodiscard]] bool finished() const noexcept {
        return phase_ == BattlePhase::Won || phase_ == BattlePhase::Lost ||
               phase_ == BattlePhase::Fled;
    }
    [[nodiscard]] BattleResult result() const noexcept { return result_; }
    [[nodiscard]] BattlePhase phase() const noexcept { return phase_; }
    [[nodiscard]] int menu_index() const noexcept { return menu_index_; }
    [[nodiscard]] const BattleFighter& hero() const noexcept { return hero_; }
    [[nodiscard]] const BattleFighter& enemy() const noexcept { return enemy_; }
    [[nodiscard]] const std::vector<std::string>& log() const noexcept {
        return lines_;
    }
    [[nodiscard]] const char* menu_label(int i) const noexcept;

    /** @brief Statuszeile für HUD. */
    [[nodiscard]] std::string status_line() const;

private:
    void push(std::string s);
    void resolve_player(BattleAction a);
    void resolve_enemy();
    void check_end();
    i32 damage(const BattleFighter& atk, const BattleFighter& def, i32 power) const;

    BattleFighter hero_{};
    BattleFighter enemy_{};
    BattlePhase phase_ = BattlePhase::Intro;
    BattleResult result_{};
    int menu_index_ = 0;
    i32 enemy_exp_ = 0;
    i32 enemy_gold_ = 0;
    std::vector<std::string> lines_;
    LogFn log_;
    static constexpr int kMenuCount = 4; // Attack Skill Defend Flee
};

} // namespace aether::game
