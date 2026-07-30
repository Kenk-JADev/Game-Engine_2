/**
 * @file event_system.hpp
 * @brief Visuelles Eventsystem – Datenmodell (ohne Code-Pflicht für Autoren).
 *
 * Events werden als JSON gespeichert und im Editor als Befehlsliste bearbeitet.
 * Unterstützte Befehlstypen (Phase 1):
 *   message, choice, set_switch, set_variable, conditional,
 *   transfer, camera, weather, animation, quest, shop, battle, script
 */
#pragma once

#include <aether/core/types.hpp>

#include <nlohmann/json.hpp>

#include <functional>
#include <string>
#include <string_view>
#include <vector>

namespace aether::game {

/** @brief Typ eines Event-Befehls. */
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
    Shop,
    Battle,
    Script,
    Wait,
    Comment,
    End,
};

[[nodiscard]] const char* to_string(EventCommandType t) noexcept;
[[nodiscard]] EventCommandType event_command_type_from_string(std::string_view s) noexcept;

/**
 * @brief Ein Befehl im Event (Parameter als JSON-Objekt).
 */
struct EventCommand {
    EventCommandType type = EventCommandType::Nop;
    nlohmann::json params = nlohmann::json::object();
    std::vector<EventCommand> children;
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

/**
 * @brief Laufzeit: Switches & Variables (RPG-Maker-Stil).
 */
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
 * @brief Führt Event-Befehle schrittweise aus (Interpreter).
 */
class EventInterpreter {
public:
    explicit EventInterpreter(GameState* state);

    void start(const std::vector<EventCommand>& commands);
    void stop();
    [[nodiscard]] bool is_running() const noexcept { return running_; }

    bool update();

    [[nodiscard]] EventCommandType last_type() const noexcept { return last_type_; }
    [[nodiscard]] const std::vector<std::string>& messages() const noexcept {
        return messages_;
    }

    using ScriptHandler = std::function<void(const std::string& code)>;
    using TransferHandler =
        std::function<void(u32 map_id, f32 x, f32 y, f32 z, i32 dir)>;

    void set_script_handler(ScriptHandler h) { script_handler_ = std::move(h); }
    void set_transfer_handler(TransferHandler h) { transfer_handler_ = std::move(h); }

private:
    bool execute(const EventCommand& cmd);

    GameState* state_ = nullptr;
    std::vector<EventCommand> commands_;
    usize index_ = 0;
    bool running_ = false;
    EventCommandType last_type_ = EventCommandType::Nop;
    std::vector<std::string> messages_;
    ScriptHandler script_handler_;
    TransferHandler transfer_handler_;
    i32 wait_frames_ = 0;
};

void to_json(nlohmann::json& j, const EventCommand& c);
void from_json(const nlohmann::json& j, EventCommand& c);
void to_json(nlohmann::json& j, const MapEvent& e);
void from_json(const nlohmann::json& j, MapEvent& e);

} // namespace aether::game
