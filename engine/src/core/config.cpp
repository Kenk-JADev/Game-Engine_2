/**
 * @file config.cpp
 * @brief JSON-Laden/Speichern der EngineConfig.
 */
#include <aether/core/config.hpp>

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cctype>
#include <fstream>

namespace aether::core {
namespace {

using nlohmann::json;

std::string to_lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

const char* mode_to_json(AppMode mode) {
    switch (mode) {
    case AppMode::Editor:   return "editor";
    case AppMode::Runtime:  return "runtime";
    case AppMode::TestPlay: return "testplay";
    case AppMode::Headless: return "headless";
    }
    return "runtime";
}

AppMode mode_from_json(std::string_view s) {
    const std::string v = to_lower(std::string(s));
    if (v == "editor")   return AppMode::Editor;
    if (v == "testplay") return AppMode::TestPlay;
    if (v == "headless") return AppMode::Headless;
    return AppMode::Runtime;
}

json graphics_to_json(const GraphicsConfig& g) {
    return json{
        {"title", g.title},
        {"width", g.width},
        {"height", g.height},
        {"fullscreen", g.fullscreen},
        {"vsync", g.vsync},
        {"frame_rate", g.frame_rate},
        {"resizable", g.resizable},
    };
}

void graphics_from_json(const json& j, GraphicsConfig& g) {
    if (!j.is_object()) return;
    if (j.contains("title"))       g.title = j.at("title").get<std::string>();
    if (j.contains("width"))       g.width = j.at("width").get<i32>();
    if (j.contains("height"))      g.height = j.at("height").get<i32>();
    if (j.contains("fullscreen")) g.fullscreen = j.at("fullscreen").get<bool>();
    if (j.contains("vsync"))       g.vsync = j.at("vsync").get<bool>();
    if (j.contains("frame_rate"))  g.frame_rate = j.at("frame_rate").get<i32>();
    if (j.contains("resizable"))   g.resizable = j.at("resizable").get<bool>();
}

json to_json(const EngineConfig& c) {
    return json{
        {"mode", mode_to_json(c.mode)},
        {"graphics", graphics_to_json(c.graphics)},
        {"audio",
         {{"bgm_volume", c.audio.bgm_volume},
          {"bgs_volume", c.audio.bgs_volume},
          {"me_volume", c.audio.me_volume},
          {"se_volume", c.audio.se_volume}}},
        {"paths",
         {{"assets_dir", c.paths.assets_dir.string()},
          {"logs_dir", c.paths.logs_dir.string()},
          {"project_dir", c.paths.project_dir.string()}}},
        {"log",
         {{"console", c.log.console},
          {"file", c.log.file},
          {"level", c.log.level}}},
        {"worker_threads", c.worker_threads},
        {"enable_ruby", c.enable_ruby},
    };
}

void from_json_obj(const json& j, EngineConfig& c) {
    if (j.contains("mode") && j.at("mode").is_string()) {
        c.mode = mode_from_json(j.at("mode").get<std::string>());
    }
    if (j.contains("graphics")) {
        graphics_from_json(j.at("graphics"), c.graphics);
    }
    if (j.contains("audio") && j.at("audio").is_object()) {
        const auto& a = j.at("audio");
        if (a.contains("bgm_volume")) c.audio.bgm_volume = a.at("bgm_volume").get<i32>();
        if (a.contains("bgs_volume")) c.audio.bgs_volume = a.at("bgs_volume").get<i32>();
        if (a.contains("me_volume"))  c.audio.me_volume  = a.at("me_volume").get<i32>();
        if (a.contains("se_volume"))  c.audio.se_volume  = a.at("se_volume").get<i32>();
    }
    if (j.contains("paths") && j.at("paths").is_object()) {
        const auto& p = j.at("paths");
        if (p.contains("assets_dir"))
            c.paths.assets_dir = p.at("assets_dir").get<std::string>();
        if (p.contains("logs_dir"))
            c.paths.logs_dir = p.at("logs_dir").get<std::string>();
        if (p.contains("project_dir"))
            c.paths.project_dir = p.at("project_dir").get<std::string>();
    }
    if (j.contains("log") && j.at("log").is_object()) {
        const auto& l = j.at("log");
        if (l.contains("console")) c.log.console = l.at("console").get<bool>();
        if (l.contains("file"))    c.log.file = l.at("file").get<bool>();
        if (l.contains("level"))   c.log.level = l.at("level").get<std::string>();
    }
    if (j.contains("worker_threads"))
        c.worker_threads = j.at("worker_threads").get<u32>();
    if (j.contains("enable_ruby"))
        c.enable_ruby = j.at("enable_ruby").get<bool>();
}

} // namespace

LogLevel log_level_from_string(std::string_view s) noexcept {
    const std::string v = to_lower(std::string(s));
    if (v == "trace") return LogLevel::Trace;
    if (v == "debug") return LogLevel::Debug;
    if (v == "info")  return LogLevel::Info;
    if (v == "warn" || v == "warning") return LogLevel::Warn;
    if (v == "error") return LogLevel::Error;
    if (v == "fatal") return LogLevel::Fatal;
    if (v == "off" || v == "none") return LogLevel::Off;
    return LogLevel::Info;
}

const char* to_string(AppMode mode) noexcept {
    return mode_to_json(mode);
}

Result<EngineConfig> load_engine_config(const std::filesystem::path& path) {
    std::ifstream in(path);
    if (!in) {
        return Result<EngineConfig>::fail("Cannot open config: " + path.string());
    }
    try {
        json j;
        in >> j;
        EngineConfig cfg;
        from_json_obj(j, cfg);
        return Result<EngineConfig>::ok(std::move(cfg));
    } catch (const std::exception& ex) {
        return Result<EngineConfig>::fail(std::string("Config parse error: ") + ex.what());
    }
}

Result<void> save_engine_config(const std::filesystem::path& path,
                                const EngineConfig& config) {
    try {
        if (path.has_parent_path()) {
            std::error_code ec;
            std::filesystem::create_directories(path.parent_path(), ec);
        }
        std::ofstream out(path);
        if (!out) {
            return Result<void>::fail("Cannot write config: " + path.string());
        }
        out << to_json(config).dump(2) << '\n';
        return Result<void>::ok();
    } catch (const std::exception& ex) {
        return Result<void>::fail(std::string("Config write error: ") + ex.what());
    }
}

} // namespace aether::core
