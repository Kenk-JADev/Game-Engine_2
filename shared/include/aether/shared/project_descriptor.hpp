/**
 * @file project_descriptor.hpp
 * @brief Gemeinsames Projektformat (project.json) für Editor und Runtime.
 */
#pragma once

#include <nlohmann/json.hpp>

#include <filesystem>
#include <string>
#include <vector>

namespace aether::shared {

namespace fs = std::filesystem;

struct ProjectStart {
    int map_id = 1;
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
    int direction = 2;
};

struct ProjectGraphics {
    std::string title = "New Project";
    int width = 1280;
    int height = 720;
    bool fullscreen = false;
    bool vsync = true;
    int frame_rate = 60;
};

struct ProjectAudio {
    int bgm_volume = 80;
    int bgs_volume = 64;
    int me_volume = 80;
    int se_volume = 80;
};

struct ProjectScripts {
    std::string entry = "scripts/main.rb";
};

/**
 * @brief Inhalt von project.json.
 */
struct ProjectDescriptor {
    int format_version = 1;
    std::string name = "New Project";
    std::string version = "0.0.1";
    std::string author;
    std::string description;
    int engine_api_version = 1;

    ProjectStart start{};
    ProjectGraphics graphics{};
    ProjectAudio audio{};
    ProjectScripts scripts{};
    std::vector<std::string> plugins;

    std::string data_path = "data";
    std::string maps_path = "maps";
    std::string graphics_path = "graphics";
    std::string audio_path = "audio";

    /** @brief Absoluter/relativer Projektstamm (nicht in JSON, gesetzt beim Laden). */
    fs::path root_dir;
};

/**
 * @brief Lädt project.json.
 * @param path Pfad zur Datei oder zum Projektordner (sucht project.json)
 */
[[nodiscard]] bool load_project_descriptor(const fs::path& path, ProjectDescriptor& out,
                                           std::string* error = nullptr);

/**
 * @brief Speichert project.json in out_path.
 */
[[nodiscard]] bool save_project_descriptor(const fs::path& out_path,
                                           const ProjectDescriptor& desc,
                                           std::string* error = nullptr);

/**
 * @brief Konvertiert Descriptor ↔ JSON.
 */
[[nodiscard]] nlohmann::json to_json(const ProjectDescriptor& d);
void from_json(const nlohmann::json& j, ProjectDescriptor& d);

} // namespace aether::shared
