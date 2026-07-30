/**
 * @file save_system.hpp
 * @brief Speichern / Laden von Spielständen (JSON).
 */
#pragma once

#include <aether/core/types.hpp>
#include <aether/game/event_system.hpp>
#include <aether/game/inventory.hpp>
#include <aether/game/weather.hpp>
#include <aether/render/math.hpp>

#include <filesystem>
#include <string>
#include <vector>

namespace aether::game {

struct SaveHeader {
    int slot = 0;
    std::string game_title;
    std::string timestamp;
    std::string map_name;
    i32 playtime_seconds = 0;
    i32 gold = 0;
    i32 party_level = 1;
};

struct SaveData {
    SaveHeader header;
    u32 map_id = 1;
    render::Vec3 player_pos{0.0f};
    i32 player_facing = 2;
    // Note: PartyInventory is value-copied into saves
    PartyInventory inventory{};
    // Switches / variables as dense arrays (subset)
    std::vector<u8> switches;   ///< 0/1
    std::vector<i32> variables;
    WeatherType weather = WeatherType::None;
    f32 weather_power = 0.0f;
};

/**
 * @brief Savegame I/O unter saves/saveXX.json.
 */
class SaveSystem {
public:
    explicit SaveSystem(std::filesystem::path save_dir = "saves");

    [[nodiscard]] std::filesystem::path slot_path(int slot) const;
    [[nodiscard]] bool slot_exists(int slot) const;
    [[nodiscard]] std::vector<SaveHeader> list_slots(int max_slots = 16) const;

    [[nodiscard]] Result<void> save(int slot, const SaveData& data);
    [[nodiscard]] Result<SaveData> load(int slot);

    /**
     * @brief Baut SaveData aus laufendem Zustand.
     */
    static SaveData capture(const std::string& title, u32 map_id,
                            const std::string& map_name,
                            const render::Vec3& player_pos, i32 facing,
                            const PartyInventory& inv, const GameState& state,
                            const WeatherSystem& weather, i32 playtime_sec);

    /**
     * @brief Wendet SaveData auf GameState / Inventory / Weather an.
     */
    static void apply_state(const SaveData& data, GameState& state,
                            PartyInventory& inv, WeatherSystem& weather);

private:
    std::filesystem::path save_dir_;
};

} // namespace aether::game
