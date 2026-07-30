/**
 * @file plugin_loader.hpp
 * @brief Plugin-System: plugin.json + main.rb + assets/
 */
#pragma once

#include <aether/core/types.hpp>
#include <aether/ruby/ruby_vm.hpp>

#include <filesystem>
#include <string>
#include <vector>

namespace aether::plugin {

namespace fs = std::filesystem;

/**
 * @brief Inhalt von plugin.json.
 */
struct PluginManifest {
    std::string name;
    std::string version = "1.0.0";
    std::string author;
    std::string description;
    std::string entry = "main.rb";
    std::vector<std::string> dependencies;
    int api_version = 1;

    fs::path root_dir;
};

/**
 * @brief Geladenes Plugin.
 */
struct LoadedPlugin {
    PluginManifest manifest;
    bool active = false;
};

/**
 * @brief Lädt und aktiviert Plugins aus einem Verzeichnis.
 */
class PluginLoader : public aether::NonMovable {
public:
    /**
     * @brief Scannt `plugins_dir` nach Unterordnern mit plugin.json.
     */
    usize scan(const fs::path& plugins_dir);

    /**
     * @brief Lädt und führt entry-Scripts über die Ruby-VM aus.
     */
    Result<void> activate_all(ruby::RubyVM& vm);

    void deactivate_all(ruby::RubyVM& vm);

    [[nodiscard]] const std::vector<LoadedPlugin>& plugins() const noexcept {
        return plugins_;
    }

    [[nodiscard]] static Result<PluginManifest> load_manifest(const fs::path& plugin_root);

private:
    std::vector<LoadedPlugin> plugins_;
};

} // namespace aether::plugin
