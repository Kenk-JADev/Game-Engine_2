/**
 * @file quest.cpp
 */
#include <aether/game/quest.hpp>
#include <aether/core/logger.hpp>

namespace aether::game {

void QuestLog::register_def(QuestDef def) {
    const std::string id = def.id;
    defs_[id] = std::move(def);
}

const QuestDef* QuestLog::find_def(std::string_view id) const {
    const auto it = defs_.find(std::string(id));
    return it == defs_.end() ? nullptr : &it->second;
}

bool QuestLog::start(std::string_view id) {
    const auto* def = find_def(id);
    if (!def) {
        core::log_warn("Quest", std::string("Unknown quest ") + std::string(id));
        return false;
    }
    auto& p = progress_[std::string(id)];
    if (p.status == QuestStatus::Active || p.status == QuestStatus::Completed) {
        return false;
    }
    p.id = def->id;
    p.status = QuestStatus::Active;
    p.counter = 0;
    p.target = 1;
    core::log_info("Quest", "Started: " + def->title);
    return true;
}

bool QuestLog::complete(std::string_view id) {
    auto it = progress_.find(std::string(id));
    if (it == progress_.end() || it->second.status != QuestStatus::Active) {
        return false;
    }
    it->second.status = QuestStatus::Completed;
    it->second.counter = it->second.target;
    core::log_info("Quest", "Completed: " + std::string(id));
    return true;
}

bool QuestLog::fail(std::string_view id) {
    auto it = progress_.find(std::string(id));
    if (it == progress_.end() || it->second.status != QuestStatus::Active) {
        return false;
    }
    it->second.status = QuestStatus::Failed;
    return true;
}

bool QuestLog::advance(std::string_view id, i32 amount) {
    auto it = progress_.find(std::string(id));
    if (it == progress_.end() || it->second.status != QuestStatus::Active) {
        return false;
    }
    it->second.counter += amount;
    if (it->second.counter >= it->second.target) {
        return complete(id);
    }
    return true;
}

QuestStatus QuestLog::status(std::string_view id) const {
    const auto it = progress_.find(std::string(id));
    return it == progress_.end() ? QuestStatus::Inactive : it->second.status;
}

bool QuestLog::active(std::string_view id) const {
    return status(id) == QuestStatus::Active;
}

bool QuestLog::completed(std::string_view id) const {
    return status(id) == QuestStatus::Completed;
}

std::vector<QuestProgress> QuestLog::active_list() const {
    std::vector<QuestProgress> out;
    for (const auto& [k, p] : progress_) {
        (void)k;
        if (p.status == QuestStatus::Active) {
            out.push_back(p);
        }
    }
    return out;
}

std::vector<QuestProgress> QuestLog::all() const {
    std::vector<QuestProgress> out;
    out.reserve(progress_.size());
    for (const auto& [k, p] : progress_) {
        (void)k;
        out.push_back(p);
    }
    return out;
}

nlohmann::json QuestLog::to_json() const {
    nlohmann::json arr = nlohmann::json::array();
    for (const auto& [k, p] : progress_) {
        (void)k;
        arr.push_back({{"id", p.id},
                       {"status", static_cast<int>(p.status)},
                       {"counter", p.counter},
                       {"target", p.target}});
    }
    return nlohmann::json{{"progress", arr}};
}

void QuestLog::from_json(const nlohmann::json& j) {
    progress_.clear();
    if (!j.contains("progress")) {
        return;
    }
    for (const auto& e : j["progress"]) {
        QuestProgress p;
        p.id = e.value("id", "");
        p.status = static_cast<QuestStatus>(e.value("status", 0));
        p.counter = e.value("counter", 0);
        p.target = e.value("target", 1);
        if (!p.id.empty()) {
            progress_[p.id] = p;
        }
    }
}

void QuestLog::load_defaults() {
    register_def(QuestDef{"main_001", "Der Älteste",
                          "Sprich mit dem Ältesten im Dorf.", 20, 50, 1, 1});
    register_def(QuestDef{"hunt_001", "Schleim-Jagd",
                          "Besiege einen Schleim im Kampf.", 30, 20, 0, 0});
}

} // namespace aether::game
