/**
 * @file event_system.hpp
 * @brief Visuelles Eventsystem – Datenmodell + Interpreter.
 */
#pragma once

#include <aether/core/types.hpp>

#include <nlohmann/json.hpp>

#include <functional>
#include <string>
#include <string_view>
#include <vector>

namespace aether::game {

enum class EventCommandType {
    Nop,
    Message,
    Choice,
    SetSwitch,
    SetVariable,
    ConditionalBranch,
    TransferPlayer,
    ControlCamera,
    SetWeather,
    PlayAnimation,
    StartQuest,
    CompleteQuest,
    Shop,
    Battle,
    Script,
    Wait,
    Comment,
    End,
};

[[nodiscard]] const char* to_string(EventCommandType t) noexcept;
[[nodiscard]] EventCommandType event_command_type_from_string(std::string_view s) noexcept;

struct EventCommand {
    EventCommandType type = EventCommandType::Nop;
    nlohmann::json params = nlohmann::json::object();
    std::vector<EventCommand> children; ///< choice branches / conditional true
    std::vector<EventCommand> else_children; ///< conditional false / choice alt
};

enum class EventTrigger {
    ActionButton,
    PlayerTouch,
    EventTouch,
    Autorun,
    Parallel,
};

[[nodiscard]] const char* to_string(EventTrigger t) noexcept;
[[nodiscard]] EventTrigger event_trigger_from_string(std::string_view s) noexcept;

struct EventPage {
    std::string name;
    EventTrigger trigger = EventTrigger::ActionButton;
    nlohmann::json conditions = nlohmann::json::object();
    std::vector<EventCommand> commands;
};

struct MapEvent {
    EventId id = kInvalidEntity;
    std::string name;
    f32 x = 0, y = 0, z = 0;
    std::vector<EventPage> pages;
};

class GameState {
public:
    static constexpr usize kMaxSwitches = 4096;
    static constexpr usize kMaxVariables = 4096;

    void set_switch(u32 id, bool value);
    [[nodiscard]] bool get_switch(u32 id) const noexcept;
    void set_variable(u32 id, i32 value);
    [[nodiscard]] i32 get_variable(u32 id) const noexcept;
    void clear();

private:
    std::vector<bool> switches_ = std::vector<bool>(kMaxSwitches, false);
    std::vector<i32> variables_ = std::vector<i32>(kMaxVariables, 0);
};

/**
 * @brief Pending high-level requests produced by event commands.
 * Runtime consumes these to open battle/shop/dialog choice UI.
 */
struct EventRequest {
    enum class Kind { None, Choice, Weather, QuestStart, QuestComplete, Shop, Battle, Camera } kind =
        Kind::None;
    nlohmann::json params = nlohmann::json::object();
    std::vector<std::string> choice_labels;
    int choice_result = -1; ///< set by runtime before resume
};

class EventInterpreter {
public:
    explicit EventInterpreter(GameState* state);

    void start(const std::vector<EventCommand>& commands);
    void stop();
    [[nodiscard]] bool is_running() const noexcept { return running_; }
    [[nodiscard]] bool is_waiting_for_choice() const noexcept {
        return waiting_choice_;
    }

    /**
     * @brief Continue after UI choice (index into choice_labels).
     */
    void resume_choice(int index);

    bool update();

    [[nodiscard]] EventCommandType last_type() const noexcept { return last_type_; }
    [[nodiscard]] const std::vector<std::string>& messages() const noexcept {
        return messages_;
    }
    void clear_messages() { messages_.clear(); }

    [[nodiscard]] const EventRequest& pending_request() const noexcept {
        return pending_;
    }
    void clear_pending() { pending_ = {}; }

    using ScriptHandler = std::function<void(const std::string& code)>;
    using TransferHandler =
        std::function<void(u32 map_id, f32 x, f32 y, f32 z, i32 dir)>;

    void set_script_handler(ScriptHandler h) { script_handler_ = std::move(h); }
    void set_transfer_handler(TransferHandler h) { transfer_handler_ = std::move(h); }

private:
    bool execute(const EventCommand& cmd);
    void insert_commands(const std::vector<EventCommand>& cmds);

    GameState* state_ = nullptr;
    std::vector<EventCommand> commands_;
    usize index_ = 0;
    bool running_ = false;
    bool waiting_choice_ = false;
    EventCommand pending_choice_cmd_{};
    EventCommandType last_type_ = EventCommandType::Nop;
    std::vector<std::string> messages_;
    ScriptHandler script_handler_;
    TransferHandler transfer_handler_;
    i32 wait_frames_ = 0;
    EventRequest pending_{};
};

void to_json(nlohmann::json& j, const EventCommand& c);
void from_json(const nlohmann::json& j, EventCommand& c);
void to_json(nlohmann::json& j, const MapEvent& e);
void from_json(const nlohmann::json& j, MapEvent& e);

} // namespace aether::game
