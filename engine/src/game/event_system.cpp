/**
 * @file event_system.cpp
 */
#include <aether/game/event_system.hpp>
#include <aether/core/logger.hpp>

#include <unordered_map>

namespace aether::game {
namespace {

const std::unordered_map<std::string, EventCommandType> kCmdMap = {
    {"nop", EventCommandType::Nop},
    {"message", EventCommandType::Message},
    {"choice", EventCommandType::Choice},
    {"set_switch", EventCommandType::SetSwitch},
    {"set_variable", EventCommandType::SetVariable},
    {"conditional", EventCommandType::ConditionalBranch},
    {"transfer", EventCommandType::TransferPlayer},
    {"camera", EventCommandType::ControlCamera},
    {"weather", EventCommandType::SetWeather},
    {"animation", EventCommandType::PlayAnimation},
    {"quest", EventCommandType::StartQuest},
    {"shop", EventCommandType::Shop},
    {"battle", EventCommandType::Battle},
    {"script", EventCommandType::Script},
    {"wait", EventCommandType::Wait},
    {"comment", EventCommandType::Comment},
    {"end", EventCommandType::End},
};

} // namespace

const char* to_string(EventCommandType t) noexcept {
    switch (t) {
    case EventCommandType::Nop: return "nop";
    case EventCommandType::Message: return "message";
    case EventCommandType::Choice: return "choice";
    case EventCommandType::SetSwitch: return "set_switch";
    case EventCommandType::SetVariable: return "set_variable";
    case EventCommandType::ConditionalBranch: return "conditional";
    case EventCommandType::TransferPlayer: return "transfer";
    case EventCommandType::ControlCamera: return "camera";
    case EventCommandType::SetWeather: return "weather";
    case EventCommandType::PlayAnimation: return "animation";
    case EventCommandType::StartQuest: return "quest";
    case EventCommandType::Shop: return "shop";
    case EventCommandType::Battle: return "battle";
    case EventCommandType::Script: return "script";
    case EventCommandType::Wait: return "wait";
    case EventCommandType::Comment: return "comment";
    case EventCommandType::End: return "end";
    }
    return "nop";
}

EventCommandType event_command_type_from_string(std::string_view s) noexcept {
    const auto it = kCmdMap.find(std::string(s));
    return it == kCmdMap.end() ? EventCommandType::Nop : it->second;
}

const char* to_string(EventTrigger t) noexcept {
    switch (t) {
    case EventTrigger::ActionButton: return "action";
    case EventTrigger::PlayerTouch: return "player_touch";
    case EventTrigger::EventTouch: return "event_touch";
    case EventTrigger::Autorun: return "autorun";
    case EventTrigger::Parallel: return "parallel";
    }
    return "action";
}

EventTrigger event_trigger_from_string(std::string_view s) noexcept {
    if (s == "player_touch") return EventTrigger::PlayerTouch;
    if (s == "event_touch") return EventTrigger::EventTouch;
    if (s == "autorun") return EventTrigger::Autorun;
    if (s == "parallel") return EventTrigger::Parallel;
    return EventTrigger::ActionButton;
}

void GameState::set_switch(u32 id, bool value) {
    if (id < switches_.size()) switches_[id] = value;
}

bool GameState::get_switch(u32 id) const noexcept {
    return id < switches_.size() ? switches_[id] : false;
}

void GameState::set_variable(u32 id, i32 value) {
    if (id < variables_.size()) variables_[id] = value;
}

i32 GameState::get_variable(u32 id) const noexcept {
    return id < variables_.size() ? variables_[id] : 0;
}

void GameState::clear() {
    std::fill(switches_.begin(), switches_.end(), false);
    std::fill(variables_.begin(), variables_.end(), 0);
}

EventInterpreter::EventInterpreter(GameState* state) : state_(state) {}

void EventInterpreter::start(const std::vector<EventCommand>& commands) {
    commands_ = commands;
    index_ = 0;
    running_ = !commands_.empty();
    messages_.clear();
    wait_frames_ = 0;
    last_type_ = EventCommandType::Nop;
}

void EventInterpreter::stop() {
    running_ = false;
    commands_.clear();
    index_ = 0;
}

bool EventInterpreter::update() {
    if (!running_) return false;

    if (wait_frames_ > 0) {
        --wait_frames_;
        return true;
    }

    while (index_ < commands_.size()) {
        const EventCommand& cmd = commands_[index_++];
        last_type_ = cmd.type;
        if (!execute(cmd)) {
            // execute returns false → yield until next frame (wait)
            return true;
        }
        if (cmd.type == EventCommandType::End) {
            running_ = false;
            return false;
        }
    }
    running_ = false;
    return false;
}

bool EventInterpreter::execute(const EventCommand& cmd) {
    switch (cmd.type) {
    case EventCommandType::Message: {
        std::string text = cmd.params.value("text", "");
        messages_.push_back(std::move(text));
        return true;
    }
    case EventCommandType::SetSwitch: {
        const u32 id = cmd.params.value("id", 0u);
        const bool v = cmd.params.value("value", true);
        if (state_) state_->set_switch(id, v);
        return true;
    }
    case EventCommandType::SetVariable: {
        const u32 id = cmd.params.value("id", 0u);
        const i32 v = cmd.params.value("value", 0);
        if (state_) state_->set_variable(id, v);
        return true;
    }
    case EventCommandType::ConditionalBranch: {
        // params: switch_id / variable_id + op + value
        bool ok = true;
        if (cmd.params.contains("switch_id") && state_) {
            ok = state_->get_switch(cmd.params["switch_id"].get<u32>()) ==
                 cmd.params.value("value", true);
        } else if (cmd.params.contains("variable_id") && state_) {
            const i32 lhs = state_->get_variable(cmd.params["variable_id"].get<u32>());
            const i32 rhs = cmd.params.value("value", 0);
            const std::string op = cmd.params.value("op", "==");
            if (op == "==") ok = lhs == rhs;
            else if (op == "!=") ok = lhs != rhs;
            else if (op == ">") ok = lhs > rhs;
            else if (op == "<") ok = lhs < rhs;
            else if (op == ">=") ok = lhs >= rhs;
            else if (op == "<=") ok = lhs <= rhs;
        }
        if (ok) {
            // inline children immediately
            for (const auto& child : cmd.children) {
                last_type_ = child.type;
                if (!execute(child)) return false;
            }
        }
        return true;
    }
    case EventCommandType::TransferPlayer: {
        if (transfer_handler_) {
            transfer_handler_(cmd.params.value("map_id", 1u),
                              cmd.params.value("x", 0.0f),
                              cmd.params.value("y", 0.0f),
                              cmd.params.value("z", 0.0f),
                              cmd.params.value("direction", 2));
        }
        return true;
    }
    case EventCommandType::Script: {
        if (script_handler_) {
            script_handler_(cmd.params.value("code", ""));
        }
        return true;
    }
    case EventCommandType::Wait: {
        wait_frames_ = cmd.params.value("frames", 1);
        return false; // yield
    }
    case EventCommandType::Comment:
    case EventCommandType::Nop:
    case EventCommandType::Choice:
    case EventCommandType::ControlCamera:
    case EventCommandType::SetWeather:
    case EventCommandType::PlayAnimation:
    case EventCommandType::StartQuest:
    case EventCommandType::Shop:
    case EventCommandType::Battle:
    case EventCommandType::End:
        return true;
    }
    return true;
}

void to_json(nlohmann::json& j, const EventCommand& c) {
    j = nlohmann::json{{"type", to_string(c.type)}, {"params", c.params}};
    if (!c.children.empty()) {
        j["children"] = c.children;
    }
}

void from_json(const nlohmann::json& j, EventCommand& c) {
    c.type = event_command_type_from_string(j.value("type", "nop"));
    c.params = j.value("params", nlohmann::json::object());
    c.children.clear();
    if (j.contains("children") && j["children"].is_array()) {
        for (const auto& ch : j["children"]) {
            EventCommand child;
            from_json(ch, child);
            c.children.push_back(std::move(child));
        }
    }
}

void to_json(nlohmann::json& j, const MapEvent& e) {
    nlohmann::json pages = nlohmann::json::array();
    for (const auto& p : e.pages) {
        nlohmann::json cmds = nlohmann::json::array();
        for (const auto& c : p.commands) {
            nlohmann::json jc;
            to_json(jc, c);
            cmds.push_back(std::move(jc));
        }
        pages.push_back({
            {"name", p.name},
            {"trigger", to_string(p.trigger)},
            {"conditions", p.conditions},
            {"commands", cmds},
        });
    }
    j = nlohmann::json{
        {"id", e.id},
        {"name", e.name},
        {"x", e.x},
        {"y", e.y},
        {"z", e.z},
        {"pages", pages},
    };
}

void from_json(const nlohmann::json& j, MapEvent& e) {
    e.id = j.value("id", 0u);
    e.name = j.value("name", "");
    e.x = j.value("x", 0.0f);
    e.y = j.value("y", 0.0f);
    e.z = j.value("z", 0.0f);
    e.pages.clear();
    if (j.contains("pages") && j["pages"].is_array()) {
        for (const auto& pj : j["pages"]) {
            EventPage p;
            p.name = pj.value("name", "");
            p.trigger = event_trigger_from_string(pj.value("trigger", "action"));
            p.conditions = pj.value("conditions", nlohmann::json::object());
            if (pj.contains("commands")) {
                for (const auto& cj : pj["commands"]) {
                    EventCommand c;
                    from_json(cj, c);
                    p.commands.push_back(std::move(c));
                }
            }
            e.pages.push_back(std::move(p));
        }
    }
}

} // namespace aether::game
