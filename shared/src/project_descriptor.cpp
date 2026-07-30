/**
 * @file project_descriptor.cpp
 */
#include <aether/shared/project_descriptor.hpp>

#include <fstream>

namespace aether::shared {

nlohmann::json to_json(const ProjectDescriptor& d) {
    return nlohmann::json{
        {"format_version", d.format_version},
        {"name", d.name},
        {"version", d.version},
        {"author", d.author},
        {"description", d.description},
        {"engine_api_version", d.engine_api_version},
        {"start",
         {{"map_id", d.start.map_id},
          {"x", d.start.x},
          {"y", d.start.y},
          {"z", d.start.z},
          {"direction", d.start.direction}}},
        {"graphics",
         {{"title", d.graphics.title},
          {"width", d.graphics.width},
          {"height", d.graphics.height},
          {"fullscreen", d.graphics.fullscreen},
          {"vsync", d.graphics.vsync},
          {"frame_rate", d.graphics.frame_rate}}},
        {"audio",
         {{"bgm_volume", d.audio.bgm_volume},
          {"bgs_volume", d.audio.bgs_volume},
          {"me_volume", d.audio.me_volume},
          {"se_volume", d.audio.se_volume}}},
        {"scripts", {{"entry", d.scripts.entry}}},
        {"plugins", d.plugins},
        {"data_path", d.data_path},
        {"maps_path", d.maps_path},
        {"graphics_path", d.graphics_path},
        {"audio_path", d.audio_path},
    };
}

void from_json(const nlohmann::json& j, ProjectDescriptor& d) {
    if (j.contains("format_version")) d.format_version = j["format_version"].get<int>();
    if (j.contains("name")) d.name = j["name"].get<std::string>();
    if (j.contains("version")) d.version = j["version"].get<std::string>();
    if (j.contains("author")) d.author = j["author"].get<std::string>();
    if (j.contains("description")) d.description = j["description"].get<std::string>();
    if (j.contains("engine_api_version"))
        d.engine_api_version = j["engine_api_version"].get<int>();

    if (j.contains("start") && j["start"].is_object()) {
        const auto& s = j["start"];
        if (s.contains("map_id")) d.start.map_id = s["map_id"].get<int>();
        if (s.contains("x")) d.start.x = s["x"].get<double>();
        if (s.contains("y")) d.start.y = s["y"].get<double>();
        if (s.contains("z")) d.start.z = s["z"].get<double>();
        if (s.contains("direction")) d.start.direction = s["direction"].get<int>();
    }
    if (j.contains("graphics") && j["graphics"].is_object()) {
        const auto& g = j["graphics"];
        if (g.contains("title")) d.graphics.title = g["title"].get<std::string>();
        if (g.contains("width")) d.graphics.width = g["width"].get<int>();
        if (g.contains("height")) d.graphics.height = g["height"].get<int>();
        if (g.contains("fullscreen")) d.graphics.fullscreen = g["fullscreen"].get<bool>();
        if (g.contains("vsync")) d.graphics.vsync = g["vsync"].get<bool>();
        if (g.contains("frame_rate")) d.graphics.frame_rate = g["frame_rate"].get<int>();
    }
    if (j.contains("audio") && j["audio"].is_object()) {
        const auto& a = j["audio"];
        if (a.contains("bgm_volume")) d.audio.bgm_volume = a["bgm_volume"].get<int>();
        if (a.contains("bgs_volume")) d.audio.bgs_volume = a["bgs_volume"].get<int>();
        if (a.contains("me_volume")) d.audio.me_volume = a["me_volume"].get<int>();
        if (a.contains("se_volume")) d.audio.se_volume = a["se_volume"].get<int>();
    }
    if (j.contains("scripts") && j["scripts"].is_object()) {
        if (j["scripts"].contains("entry"))
            d.scripts.entry = j["scripts"]["entry"].get<std::string>();
    }
    if (j.contains("plugins") && j["plugins"].is_array()) {
        d.plugins = j["plugins"].get<std::vector<std::string>>();
    }
    if (j.contains("data_path")) d.data_path = j["data_path"].get<std::string>();
    if (j.contains("maps_path")) d.maps_path = j["maps_path"].get<std::string>();
    if (j.contains("graphics_path")) d.graphics_path = j["graphics_path"].get<std::string>();
    if (j.contains("audio_path")) d.audio_path = j["audio_path"].get<std::string>();
}

bool load_project_descriptor(const fs::path& path, ProjectDescriptor& out, std::string* error) {
    fs::path file = path;
    std::error_code ec;
    if (fs::is_directory(path, ec)) {
        file = path / "project.json";
    }
    std::ifstream in(file);
    if (!in) {
        if (error) *error = "Cannot open " + file.string();
        return false;
    }
    try {
        nlohmann::json j;
        in >> j;
        from_json(j, out);
        out.root_dir = file.parent_path().empty() ? fs::path(".") : file.parent_path();
        return true;
    } catch (const std::exception& ex) {
        if (error) *error = ex.what();
        return false;
    }
}

bool save_project_descriptor(const fs::path& out_path, const ProjectDescriptor& desc,
                             std::string* error) {
    try {
        if (out_path.has_parent_path()) {
            std::error_code ec;
            fs::create_directories(out_path.parent_path(), ec);
        }
        std::ofstream out(out_path);
        if (!out) {
            if (error) *error = "Cannot write " + out_path.string();
            return false;
        }
        out << to_json(desc).dump(2) << '\n';
        return true;
    } catch (const std::exception& ex) {
        if (error) *error = ex.what();
        return false;
    }
}

} // namespace aether::shared
