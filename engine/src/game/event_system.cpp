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
    {"quest_complete", EventCommandType::CompleteQuest},
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
    case EventCommandType::CompleteQuest: return "quest_complete";
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
    waiting_choice_ = false;
    pending_ = {};
    last_type_ = EventCommandType::Nop;
}

void EventInterpreter::stop() {
    running_ = false;
    waiting_choice_ = false;
    commands_.clear();
    index_ = 0;
    pending_ = {};
}

void EventInterpreter::insert_commands(const std::vector<EventCommand>& cmds) {
    commands_.insert(commands_.begin() + static_cast<std::ptrdiff_t>(index_), cmds.begin(),
                     cmds.end());
}

void EventInterpreter::resume_choice(int index) {
    if (!waiting_choice_) {
        return;
    }
    waiting_choice_ = false;
    pending_.choice_result = index;
    // children[i] is branch for choice i; else_children unused
    if (index >= 0 && index < static_cast<int>(pending_choice_cmd_.children.size())) {
        // Each "child" may be a single command or we store branch as nested list in params
        // Convention: children[i] is first command of branch; branches stored as
        // params["branches"] = [[cmds],[cmds]] OR children grouped.
        // Simpler: params choices labels; children[i] executed as single chain via
        // children nested in each choice command using params branches.
    }
    // Prefer params.branches as array of command arrays
    if (pending_choice_cmd_.params.contains("branches") &&
        pending_choice_cmd_.params["branches"].is_array()) {
        const auto& branches = pending_choice_cmd_.params["branches"];
        if (index >= 0 && index < static_cast<int>(branches.size())) {
            std::vector<EventCommand> branch;
            for (const auto& jc : branches[static_cast<usize>(index)]) {
                EventCommand c;
                from_json(jc, c);
                branch.push_back(std::move(c));
            }
            insert_commands(branch);
        }
    } else if (index >= 0 &&
               index < static_cast<int>(pending_choice_cmd_.children.size())) {
        // treat each child as a one-command branch
        insert_commands({pending_choice_cmd_.children[static_cast<usize>(index)]});
    }
    if (state_ && pending_choice_cmd_.params.contains("variable_id")) {
        state_->set_variable(pending_choice_cmd_.params["variable_id"].get<u32>(), index);
    }
    pending_ = {};
}

bool EventInterpreter::update() {
    if (!running_ || waiting_choice_) {
        return running_;
    }
    if (wait_frames_ > 0) {
        --wait_frames_;
        return true;
    }
    while (index_ < commands_.size()) {
        const EventCommand cmd = commands_[index_++];
        last_type_ = cmd.type;
        if (!execute(cmd)) {
            return true; // yielded
        }
        if (cmd.type == EventCommandType::End) {
            running_ = false;
            return false;
        }
        if (waiting_choice_) {
            return true;
        }
        if (pending_.kind != EventRequest::Kind::None) {
            return true; // runtime should handle request
        }
    }
    running_ = false;
    return false;
}

bool EventInterpreter::execute(const EventCommand& cmd) {
    switch (cmd.type) {
    case EventCommandType::Message:
        messages_.push_back(cmd.params.value("text", ""));
        return true;

    case EventCommandType::SetSwitch:
        if (state_) {
            state_->set_switch(cmd.params.value("id", 0u), cmd.params.value("value", true));
        }
        return true;

    case EventCommandType::SetVariable:
        if (state_) {
            state_->set_variable(cmd.params.value("id", 0u), cmd.params.value("value", 0));
        }
        return true;

    case EventCommandType::ConditionalBranch: {
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
        } else if (cmd.params.contains("quest_completed")) {
            // handled by runtime via variable convention – skip
        }
        const auto& branch = ok ? cmd.children : cmd.else_children;
        for (const auto& child : branch) {
            last_type_ = child.type;
            if (!execute(child)) {
                return false;
            }
            if (waiting_choice_ || pending_.kind != EventRequest::Kind::None) {
                return true;
            }
        }
        return true;
    }

    case EventCommandType::Choice: {
        pending_choice_cmd_ = cmd;
        pending_.kind = EventRequest::Kind::Choice;
        pending_.params = cmd.params;
        pending_.choice_labels.clear();
        if (cmd.params.contains("options") && cmd.params["options"].is_array()) {
            for (const auto& o : cmd.params["options"]) {
                pending_.choice_labels.push_back(o.get<std::string>());
            }
        } else {
            pending_.choice_labels = {"Ja", "Nein"};
        }
        waiting_choice_ = true;
        return false; // yield
    }

    case EventCommandType::TransferPlayer:
        if (transfer_handler_) {
            transfer_handler_(cmd.params.value("map_id", 1u), cmd.params.value("x", 0.0f),
                              cmd.params.value("y", 0.0f), cmd.params.value("z", 0.0f),
                              cmd.params.value("direction", 2));
        }
        return true;

    case EventCommandType::Script:
        if (script_handler_) {
            script_handler_(cmd.params.value("code", ""));
        }
        return true;

    case EventCommandType::Wait:
        wait_frames_ = cmd.params.value("frames", 1);
        return false;

    case EventCommandType::SetWeather:
        pending_.kind = EventRequest::Kind::Weather;
        pending_.params = cmd.params;
        return true;

    case EventCommandType::StartQuest:
        pending_.kind = EventRequest::Kind::QuestStart;
        pending_.params = cmd.params;
        return true;

    case EventCommandType::CompleteQuest:
        pending_.kind = EventRequest::Kind::QuestComplete;
        pending_.params = cmd.params;
        return true;

    case EventCommandType::Shop:
        pending_.kind = EventRequest::Kind::Shop;
        pending_.params = cmd.params;
        return true;

    case EventCommandType::Battle:
        pending_.kind = EventRequest::Kind::Battle;
        pending_.params = cmd.params;
        return true;

    case EventCommandType::ControlCamera:
        pending_.kind = EventRequest::Kind::Camera;
        pending_.params = cmd.params;
        return true;

    case EventCommandType::PlayAnimation:
    case EventCommandType::Comment:
    case EventCommandType::Nop:
    case EventCommandType::End:
        return true;
    }
    return true;
}

void to_json(nlohmann::json& j, const EventCommand& c) {
    j = nlohmann::json{{"type", to_string(c.type)}, {"params", c.params}};
    if (!c.children.empty()) {
        nlohmann::json arr = nlohmann::json::array();
        for (const auto& ch : c.children) {
            nlohmann::json jc;
            to_json(jc, ch);
            arr.push_back(std::move(jc));
        }
        j["children"] = arr;
    }
    if (!c.else_children.empty()) {
        nlohmann::json arr = nlohmann::json::array();
        for (const auto& ch : c.else_children) {
            nlohmann::json jc;
            to_json(jc, ch);
            arr.push_back(std::move(jc));
        }
        j["else"] = arr;
    }
}

void from_json(const nlohmann::json& j, EventCommand& c) {
    c.type = event_command_type_from_string(j.value("type", "nop"));
    c.params = j.value("params", nlohmann::json::object());
    c.children.clear();
    c.else_children.clear();
    if (j.contains("children") && j["children"].is_array()) {
        for (const auto& ch : j["children"]) {
            EventCommand child;
            from_json(ch, child);
            c.children.push_back(std::move(child));
        }
    }
    if (j.contains("else") && j["else"].is_array()) {
        for (const auto& ch : j["else"]) {
            EventCommand child;
            from_json(ch, child);
            c.else_children.push_back(std::move(child));
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
        pages.push_back({{"name", p.name},
                         {"trigger", to_string(p.trigger)},
                         {"conditions", p.conditions},
                         {"commands", cmds}});
    }
    j = nlohmann::json{{"id", e.id}, {"name", e.name}, {"x", e.x},
                       {"y", e.y},   {"z", e.z},       {"pages", pages}};
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
