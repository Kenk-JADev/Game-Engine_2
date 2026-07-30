/**
 * @file quest.hpp
 * @brief Einfaches Quest-System (Start/Fortschritt/Abschluss).
 */
#pragma once

#include <aether/core/types.hpp>

#include <nlohmann/json.hpp>

#include <string>
#include <unordered_map>
#include <vector>

namespace aether::game {

enum class QuestStatus {
    Inactive,
    Active,
    Completed,
    Failed,
};

struct QuestDef {
    std::string id;
    std::string title;
    std::string description;
    i32 exp_reward = 0;
    i32 gold_reward = 0;
    u32 item_reward_id = 0;
    i32 item_reward_count = 0;
};

struct QuestProgress {
    std::string id;
    QuestStatus status = QuestStatus::Inactive;
    i32 counter = 0;
    i32 target = 1;
};

class QuestLog {
public:
    void register_def(QuestDef def);
    [[nodiscard]] const QuestDef* find_def(std::string_view id) const;

    bool start(std::string_view id);
    bool complete(std::string_view id);
    bool fail(std::string_view id);
    bool advance(std::string_view id, i32 amount = 1);

    [[nodiscard]] QuestStatus status(std::string_view id) const;
    [[nodiscard]] bool active(std::string_view id) const;
    [[nodiscard]] bool completed(std::string_view id) const;

    [[nodiscard]] std::vector<QuestProgress> active_list() const;
    [[nodiscard]] std::vector<QuestProgress> all() const;

    [[nodiscard]] nlohmann::json to_json() const;
    void from_json(const nlohmann::json& j);

    /** Built-in demo quests. */
    void load_defaults();

private:
    std::unordered_map<std::string, QuestDef> defs_;
    std::unordered_map<std::string, QuestProgress> progress_;
};

} // namespace aether::game
