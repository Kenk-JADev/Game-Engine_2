/**
 * @file ruby_host.hpp
 * @brief Ruby-Host: bindet die Engine-API-Module an echte Systeme.
 *
 * Registriert Host-Funktionen für die Module
 *   Graphics · Audio · Input · SceneManager · Player · NPC · Enemy ·
 *   Camera · Weather · Inventory · Quest · Dialogue · Map · Game
 *
 * Die Funktionen arbeiten auf nicht-besitzenden Zeigern (RubyHostBindings);
 * fehlende Systeme (nullptr) führen zu sicheren No-Ops bzw. Defaults.
 * Anfragen an die Runtime (Transfer, Szenenwechsel, Kartenwechsel, Fades)
 * werden gesammelt und per take_*() pro Frame abgefragt.
 */
#pragma once

#include <aether/core/types.hpp>
#include <aether/render/math.hpp>
#include <aether/ruby/ruby_vm.hpp>

namespace aether::audio {
class AudioEngine;
}
namespace aether::game {
class Database;
class GameState;
class GameSceneStack;
class PartyInventory;
class PlayerController;
class QuestLog;
class WeatherSystem;
struct GameContext;
} // namespace aether::game
namespace aether::input {
class InputManager;
}
namespace aether::render {
class Camera;
}
namespace aether::scene {
class Scene;
}

namespace aether::ruby {

/** @brief Nicht-besitzende Zeiger auf Engine-Systeme (Runtime/Editor befüllt). */
struct RubyHostBindings {
    game::GameState* state = nullptr;          ///< Schalter/Variablen
    game::PartyInventory* inventory = nullptr; ///< Gold/Items/Party
    game::QuestLog* quests = nullptr;
    game::WeatherSystem* weather = nullptr;
    game::PlayerController* player = nullptr;
    render::Camera* camera = nullptr;
    input::InputManager* input = nullptr;
    audio::AudioEngine* audio = nullptr;
    game::GameSceneStack* scenes = nullptr;
    game::GameContext* gctx = nullptr;
    game::Database* database = nullptr;
    scene::Scene* scene = nullptr; ///< aktive Karte (NPC/Enemy)
    i32 screen_width = 1280;
    i32 screen_height = 720;
    i32 frame_rate = 60;
    u32 map_id = 1; ///< von der Runtime nach jedem Kartenwechsel aktualisiert
    std::string map_name;
    i32 map_width = 0;
    i32 map_height = 0;
};

/**
 * @brief Registriert die Engine-Ruby-API mit echtem Verhalten.
 *
 * Lebensdauer: Der RubyHost muss länger leben als die VM-Aufrufe, da die
 * Host-Funktionen `this` einfangen.
 */
class RubyHost {
public:
    explicit RubyHost(RubyHostBindings bindings) : b_(std::move(bindings)) {}

    /** @brief Registriert alle Module.method-Funktionen auf der VM. */
    void install(RubyVM& vm);

    // --- Pro-Frame abfragbare Anfragen (Runtime konsumiert sie) -------------

    /** @brief Nimmt einen anstehenden Spieler-Transfer (falls vorhanden). */
    [[nodiscard]] bool take_transfer(u32& map_id, render::Vec3& pos, i32& dir);
    /** @brief Nimmt eine anstehende Kartenladung (falls vorhanden). */
    [[nodiscard]] bool take_map_load(u32& map_id);
    /** @brief Nimmt eine Szenen-Anfrage ("title", "menu", … oder leer). */
    [[nodiscard]] std::string take_scene_request();
    /** @brief 0 = kein Fade, 1 = fade_out, 2 = fade_in. */
    [[nodiscard]] int take_fade_request();

    [[nodiscard]] const RubyHostBindings& bindings() const noexcept { return b_; }

    /** @brief Letzte Dialog-Auswahl (Choice). */
    void set_choice_result(int index) noexcept { choice_result_ = index; }
    [[nodiscard]] int choice_result() const noexcept { return choice_result_; }

    /** @brief Aktualisiert die Karten-Info (Runtime nach jedem Kartenwechsel). */
    void update_map(u32 id, std::string name) {
        b_.map_id = id;
        b_.map_name = std::move(name);
    }

    /** @brief Setzt die aktive Szene (Runtime nach jedem Kartenwechsel). */
    void set_scene(scene::Scene* s) noexcept { b_.scene = s; }

    /** @brief Kamera-Shake-Parameter (zuletzt via Camera.shake gesetzt). */
    void camera_shake(f32& power, f32& duration) const noexcept {
        power = shake_power_;
        duration = shake_duration_;
    }

    /** @brief Map-Tint (zuletzt via Map.tint gesetzt). */
    void map_tint(f32& r, f32& g, f32& b, f32& a) const noexcept {
        r = tint_.x;
        g = tint_.y;
        b = tint_.z;
        a = tint_.w;
    }

private:
    // Konvertierung → Ruby-Literal
    [[nodiscard]] static std::string lit(i64 v);
    [[nodiscard]] static std::string lit(f64 v);
    [[nodiscard]] static std::string lit(bool v);
    [[nodiscard]] static std::string lit_str(std::string_view v);

    // Argument-Parsing
    [[nodiscard]] static i64 arg_i64(const std::vector<std::string>& args, usize i, i64 dflt);
    [[nodiscard]] static f64 arg_f64(const std::vector<std::string>& args, usize i, f64 dflt);

    // Item-Auflösung: Zahl = Item-Id, sonst Name aus der Datenbank
    [[nodiscard]] u32 resolve_item_id(const std::string& key) const;

    RubyHostBindings b_;
    bool transfer_pending_ = false;
    u32 transfer_map_ = 1;
    render::Vec3 transfer_pos_{0.0f};
    i32 transfer_dir_ = 0;
    bool map_load_pending_ = false;
    u32 map_load_id_ = 1;
    std::string scene_request_;
    int fade_request_ = 0;
    int choice_result_ = -1;
    f32 shake_power_ = 0.0f;
    f32 shake_duration_ = 0.0f;
    render::Vec4 tint_{0.0f};
};

} // namespace aether::ruby
