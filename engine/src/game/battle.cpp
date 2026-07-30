/**
 * @file battle.cpp
 */
#include <aether/game/battle.hpp>

#include <algorithm>
#include <cstdlib>

namespace aether::game {

void Battle::push(std::string s) {
    lines_.push_back(s);
    if (lines_.size() > 8) {
        lines_.erase(lines_.begin());
    }
    if (log_) {
        log_(s);
    }
}

void Battle::start(const PartyMember& hero, const EnemyData& enemy,
                   const SkillData* skill) {
    hero_ = {};
    hero_.name = hero.name;
    hero_.hp = hero.hp;
    hero_.max_hp = hero.max_hp;
    hero_.mp = hero.mp;
    hero_.max_mp = hero.max_mp;
    hero_.attack = hero.attack;
    hero_.defense = hero.defense;
    if (skill) {
        hero_.skill_power = skill->power;
        hero_.skill_mp = skill->mp_cost;
    } else {
        hero_.skill_power = hero.attack + 5;
        hero_.skill_mp = 5;
    }

    enemy_ = {};
    enemy_.name = enemy.name;
    enemy_.hp = enemy.max_hp;
    enemy_.max_hp = enemy.max_hp;
    enemy_.attack = enemy.attack;
    enemy_.defense = enemy.defense;
    enemy_.is_enemy = true;
    enemy_exp_ = enemy.exp;
    enemy_gold_ = enemy.gold;

    phase_ = BattlePhase::PlayerChoose;
    menu_index_ = 0;
    result_ = {};
    lines_.clear();
    push("Kampf gegen " + enemy_.name + "!");
    push("Was tust du?");
}

const char* Battle::menu_label(int i) const noexcept {
    switch (i) {
    case 0: return "Angriff";
    case 1: return "Fertigkeit";
    case 2: return "Verteidigen";
    case 3: return "Flucht";
    default: return "?";
    }
}

std::string Battle::status_line() const {
    std::string s = "KAMPF | " + hero_.name + " HP " + std::to_string(hero_.hp) + "/" +
                    std::to_string(hero_.max_hp) + " MP " + std::to_string(hero_.mp) +
                    "  vs  " + enemy_.name + " HP " + std::to_string(enemy_.hp) + "/" +
                    std::to_string(enemy_.max_hp);
    if (phase_ == BattlePhase::PlayerChoose) {
        s += "  > ";
        s += menu_label(menu_index_);
    } else if (phase_ == BattlePhase::Won) {
        s += "  | SIEG";
    } else if (phase_ == BattlePhase::Lost) {
        s += "  | NIEDERLAGE";
    } else if (phase_ == BattlePhase::Fled) {
        s += "  | FLUCHT";
    }
    return s;
}

i32 Battle::damage(const BattleFighter& atk, const BattleFighter& def,
                   i32 power) const {
    i32 defv = def.defense;
    if (def.defending) {
        defv *= 2;
    }
    i32 dmg = power + atk.attack - defv / 2;
    if (dmg < 1) {
        dmg = 1;
    }
    // small variance
    dmg += (static_cast<i32>(std::rand()) % 3);
    return dmg;
}

void Battle::resolve_player(BattleAction a) {
    hero_.defending = false;
    switch (a) {
    case BattleAction::Attack: {
        const i32 d = damage(hero_, enemy_, 0);
        enemy_.hp = std::max(0, enemy_.hp - d);
        push(hero_.name + " greift an! " + std::to_string(d) + " Schaden.");
        break;
    }
    case BattleAction::Skill: {
        if (hero_.mp < hero_.skill_mp) {
            push("Nicht genug MP!");
            phase_ = BattlePhase::PlayerChoose;
            return;
        }
        hero_.mp -= hero_.skill_mp;
        const i32 d = damage(hero_, enemy_, hero_.skill_power);
        enemy_.hp = std::max(0, enemy_.hp - d);
        push(hero_.name + " wirkt Fertigkeit! " + std::to_string(d) + " Schaden.");
        break;
    }
    case BattleAction::Defend:
        hero_.defending = true;
        push(hero_.name + " verteidigt sich.");
        break;
    case BattleAction::Flee:
        if ((std::rand() % 100) < 55) {
            phase_ = BattlePhase::Fled;
            result_.fled = true;
            push("Flucht erfolgreich!");
            return;
        }
        push("Flucht gescheitert!");
        break;
    case BattleAction::Item:
        push("Kein Item gewählt.");
        break;
    }
    check_end();
    if (!finished()) {
        phase_ = BattlePhase::EnemyAct;
        resolve_enemy();
    }
}

void Battle::resolve_enemy() {
    if (finished()) {
        return;
    }
    enemy_.defending = false;
    const i32 d = damage(enemy_, hero_, 0);
    hero_.hp = std::max(0, hero_.hp - d);
    push(enemy_.name + " greift an! " + std::to_string(d) + " Schaden.");
    hero_.defending = false;
    check_end();
    if (!finished()) {
        phase_ = BattlePhase::PlayerChoose;
        push("Was tust du?");
    }
}

void Battle::check_end() {
    if (enemy_.hp <= 0) {
        phase_ = BattlePhase::Won;
        result_.won = true;
        result_.exp = enemy_exp_;
        result_.gold = enemy_gold_;
        push("Sieg! +" + std::to_string(result_.exp) + " EXP, +" +
             std::to_string(result_.gold) + " Gold.");
    } else if (hero_.hp <= 0) {
        phase_ = BattlePhase::Lost;
        result_.won = false;
        push(hero_.name + " wurde besiegt…");
    }
}

void Battle::update(const input::InputManager& input) {
    if (finished()) {
        if (input.was_pressed("confirm") || input.was_pressed("cancel")) {
            // caller pops scene
        }
        return;
    }
    if (phase_ != BattlePhase::PlayerChoose) {
        return;
    }
    if (input.was_pressed("up")) {
        menu_index_ = (menu_index_ + kMenuCount - 1) % kMenuCount;
    }
    if (input.was_pressed("down")) {
        menu_index_ = (menu_index_ + 1) % kMenuCount;
    }
    if (input.was_pressed("confirm")) {
        BattleAction a = BattleAction::Attack;
        if (menu_index_ == 1) a = BattleAction::Skill;
        if (menu_index_ == 2) a = BattleAction::Defend;
        if (menu_index_ == 3) a = BattleAction::Flee;
        resolve_player(a);
    }
}

} // namespace aether::game
