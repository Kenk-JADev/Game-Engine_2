/**
 * @file config.hpp
 * @brief Engine- und Anwendungs-Konfiguration (JSON-fähig).
 */
#pragma once

#include <aether/core/logger.hpp>
#include <aether/core/types.hpp>

#include <filesystem>
#include <string>

namespace aether::core {

/**
 * @brief Laufzeitmodus der Engine.
 */
enum class AppMode {
    Editor,   ///< Voller Editor
    Runtime,  ///< Game.exe
    TestPlay, ///< Testspiel aus dem Editor
    Headless, ///< Ohne Fenster (Tools/Tests)
};

/**
 * @brief Fenster- und Grafik-Einstellungen (anwenderrelevant).
 */
struct GraphicsConfig {
    std::string title        = "AetherRPG";
    i32  width               = 1280;
    i32  height              = 720;
    bool fullscreen          = false;
    bool vsync               = true;
    i32  frame_rate          = 60;
    bool resizable           = true;
};

/**
 * @brief Audio-Master-Pegel (0–100).
 */
struct AudioConfig {
    i32 bgm_volume = 80;
    i32 bgs_volume = 64;
    i32 me_volume  = 80;
    i32 se_volume  = 80;
};

/**
 * @brief Pfade relativ zum Arbeits- bzw. Projektverzeichnis.
 */
struct PathConfig {
    std::filesystem::path assets_dir   = "assets/engine";
    std::filesystem::path logs_dir     = "logs";
    std::filesystem::path project_dir  = ".";
};

/**
 * @brief Logging-Einstellungen.
 */
struct LogConfig {
    bool console           = true;
    bool file              = true;
    std::string level      = "info"; ///< trace|debug|info|warn|error|fatal|off
};

/**
 * @brief Vollständige Engine-Konfiguration beim Boot.
 */
struct EngineConfig {
    AppMode mode = AppMode::Runtime;
    GraphicsConfig graphics{};
    AudioConfig audio{};
    PathConfig paths{};
    LogConfig log{};
    u32 worker_threads = 0; ///< 0 = hardware_concurrency - 1 (min 1)
    bool enable_ruby   = true;
};

/**
 * @brief Lädt EngineConfig aus einer JSON-Datei.
 * @return Result mit Config oder Fehlertext
 */
[[nodiscard]] Result<EngineConfig> load_engine_config(const std::filesystem::path& path);

/**
 * @brief Speichert EngineConfig als JSON.
 */
[[nodiscard]] Result<void> save_engine_config(const std::filesystem::path& path,
                                              const EngineConfig& config);

/**
 * @brief Parst Log-Level-String (case-insensitive).
 */
[[nodiscard]] LogLevel log_level_from_string(std::string_view s) noexcept;

/**
 * @brief Liefert lesbaren Namen für AppMode.
 */
[[nodiscard]] const char* to_string(AppMode mode) noexcept;

} // namespace aether::core
