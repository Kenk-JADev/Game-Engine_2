/**
 * @file save_system.cpp
 */
#include <aether/game/save_system.hpp>
#include <aether/core/logger.hpp>

#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace aether::game {
namespace fs = std::filesystem;

namespace {

std::string now_string() {
    using clock = std::chrono::system_clock;
    const auto t = clock::to_time_t(clock::now());
    std::tm tm_buf{};
#if defined(_WIN32)
    localtime_s(&tm_buf, &t);
#else
    localtime_r(&t, &tm_buf);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm_buf, "%Y-%m-%d %H:%M");
    return oss.str();
}

} // namespace

SaveSystem::SaveSystem(fs::path save_dir) : save_dir_(std::move(save_dir)) {}

fs::path SaveSystem::slot_path(int slot) const {
    std::ostringstream name;
    name << "save" << std::setw(2) << std::setfill('0') << slot << ".json";
    return save_dir_ / name.str();
}

bool SaveSystem::slot_exists(int slot) const {
    std::error_code ec;
    return fs::exists(slot_path(slot), ec);
}

std::vector<SaveHeader> SaveSystem::list_slots(int max_slots) const {
    std::vector<SaveHeader> out;
    for (int i = 1; i <= max_slots; ++i) {
        if (!slot_exists(i)) {
            continue;
        }
        auto loaded = const_cast<SaveSystem*>(this)->load(i);
        if (loaded) {
            out.push_back(loaded.value().header);
        }
    }
    return out;
}

Result<void> SaveSystem::save(int slot, const SaveData& data) {
    try {
        std::error_code ec;
        fs::create_directories(save_dir_, ec);
        const auto path = slot_path(slot);
        nlohmann::json j{
            {"header",
             {{"slot", slot},
              {"game_title", data.header.game_title},
              {"timestamp", data.header.timestamp.empty() ? now_string()
                                                          : data.header.timestamp},
              {"map_name", data.header.map_name},
              {"playtime_seconds", data.header.playtime_seconds},
              {"gold", data.inventory.gold()},
              {"party_level", data.inventory.party().empty()
                                  ? 1
                                  : data.inventory.party().front().level}}},
            {"map_id", data.map_id},
            {"player",
             {{"x", data.player_pos.x},
              {"y", data.player_pos.y},
              {"z", data.player_pos.z},
              {"facing", data.player_facing}}},
            {"inventory", data.inventory.to_json()},
            {"switches", data.switches},
            {"variables", data.variables},
            {"weather",
             {{"type", WeatherSystem::to_string(data.weather)},
              {"power", data.weather_power}}},
        };
        std::ofstream out(path);
        if (!out) {
            return Result<void>::fail("Cannot write " + path.string());
        }
        out << j.dump(2) << '\n';
        core::log_info("Save", "Saved slot " + std::to_string(slot));
        return Result<void>::ok();
    } catch (const std::exception& ex) {
        return Result<void>::fail(ex.what());
    }
}

Result<SaveData> SaveSystem::load(int slot) {
    const auto path = slot_path(slot);
    std::ifstream in(path);
    if (!in) {
        return Result<SaveData>::fail("No save in slot " + std::to_string(slot));
    }
    try {
        nlohmann::json j;
        in >> j;
        SaveData d;
        d.header.slot = slot;
        if (j.contains("header")) {
            const auto& h = j["header"];
            d.header.game_title = h.value("game_title", "");
            d.header.timestamp = h.value("timestamp", "");
            d.header.map_name = h.value("map_name", "");
            d.header.playtime_seconds = h.value("playtime_seconds", 0);
            d.header.gold = h.value("gold", 0);
            d.header.party_level = h.value("party_level", 1);
        }
        d.map_id = j.value("map_id", 1u);
        if (j.contains("player")) {
            const auto& p = j["player"];
            d.player_pos = {p.value("x", 0.0f), p.value("y", 0.0f),
                            p.value("z", 0.0f)};
            d.player_facing = p.value("facing", 2);
        }
        if (j.contains("inventory")) {
            d.inventory.from_json(j["inventory"]);
        }
        if (j.contains("switches") && j["switches"].is_array()) {
            d.switches = j["switches"].get<std::vector<u8>>();
        }
        if (j.contains("variables") && j["variables"].is_array()) {
            d.variables = j["variables"].get<std::vector<i32>>();
        }
        if (j.contains("weather")) {
            d.weather = WeatherSystem::from_string(j["weather"].value("type", "none"));
            d.weather_power = j["weather"].value("power", 0.0f);
        }
        return Result<SaveData>::ok(std::move(d));
    } catch (const std::exception& ex) {
        return Result<SaveData>::fail(ex.what());
    }
}

SaveData SaveSystem::capture(const std::string& title, u32 map_id,
                             const std::string& map_name,
                             const render::Vec3& player_pos, i32 facing,
                             const PartyInventory& inv, const GameState& state,
                             const WeatherSystem& weather, i32 playtime_sec) {
    SaveData d;
    d.header.game_title = title;
    d.header.timestamp = now_string();
    d.header.map_name = map_name;
    d.header.playtime_seconds = playtime_sec;
    d.header.gold = inv.gold();
    d.header.party_level = inv.party().empty() ? 1 : inv.party().front().level;
    d.map_id = map_id;
    d.player_pos = player_pos;
    d.player_facing = facing;
    d.inventory = inv;
    // snapshot first 512 switches/vars
    d.switches.resize(512);
    d.variables.resize(512);
    for (u32 i = 0; i < 512; ++i) {
        d.switches[i] = state.get_switch(i) ? 1 : 0;
        d.variables[i] = state.get_variable(i);
    }
    d.weather = weather.state().type;
    d.weather_power = weather.state().power;
    return d;
}

void SaveSystem::apply_state(const SaveData& data, GameState& state,
                             PartyInventory& inv, WeatherSystem& weather) {
    inv = data.inventory;
    for (u32 i = 0; i < data.switches.size(); ++i) {
        state.set_switch(i, data.switches[i] != 0);
    }
    for (u32 i = 0; i < data.variables.size(); ++i) {
        state.set_variable(i, data.variables[i]);
    }
    weather.set(data.weather, data.weather_power, 0.0f);
}

} // namespace aether::game
