/**
 * @file plugin_loader.cpp
 */
#include <aether/plugin/plugin_loader.hpp>
#include <aether/core/logger.hpp>

#include <nlohmann/json.hpp>

#include <fstream>

namespace aether::plugin {

Result<PluginManifest> PluginLoader::load_manifest(const fs::path& plugin_root) {
    const fs::path file = plugin_root / "plugin.json";
    std::ifstream in(file);
    if (!in) {
        return Result<PluginManifest>::fail("Missing plugin.json in " + plugin_root.string());
    }
    try {
        nlohmann::json j;
        in >> j;
        PluginManifest m;
        m.name = j.value("name", plugin_root.filename().string());
        m.version = j.value("version", "1.0.0");
        m.author = j.value("author", "");
        m.description = j.value("description", "");
        m.entry = j.value("entry", "main.rb");
        m.api_version = j.value("api_version", 1);
        if (j.contains("dependencies") && j["dependencies"].is_array()) {
            m.dependencies = j["dependencies"].get<std::vector<std::string>>();
        }
        m.root_dir = plugin_root;
        return Result<PluginManifest>::ok(std::move(m));
    } catch (const std::exception& ex) {
        return Result<PluginManifest>::fail(std::string("plugin.json: ") + ex.what());
    }
}

usize PluginLoader::scan(const fs::path& plugins_dir) {
    plugins_.clear();
    std::error_code ec;
    if (!fs::exists(plugins_dir, ec) || !fs::is_directory(plugins_dir, ec)) {
        return 0;
    }
    for (const auto& entry : fs::directory_iterator(plugins_dir, ec)) {
        if (!entry.is_directory()) continue;
        auto man = load_manifest(entry.path());
        if (!man) {
            core::log_warn("Plugin", man.error().what());
            continue;
        }
        LoadedPlugin lp;
        lp.manifest = std::move(man.value());
        plugins_.push_back(std::move(lp));
        core::log_info("Plugin", "Found plugin '" + plugins_.back().manifest.name +
                                     "' v" + plugins_.back().manifest.version);
    }
    return plugins_.size();
}

Result<void> PluginLoader::activate_all(ruby::RubyVM& vm) {
    for (auto& p : plugins_) {
        if (p.active) continue;
        const fs::path entry = p.manifest.root_dir / p.manifest.entry;
        auto r = vm.load_file(entry.string());
        if (!r.ok) {
            core::log_error("Plugin", "Failed to load " + p.manifest.name + ": " + r.error);
            return Result<void>::fail(r.error);
        }
        p.active = true;
        core::log_info("Plugin", "Activated '" + p.manifest.name + "'");
    }
    return Result<void>::ok();
}

void PluginLoader::deactivate_all(ruby::RubyVM& vm) {
    for (auto& p : plugins_) {
        if (!p.active) continue;
        // Konvention: Module::<Name>.on_unload falls vorhanden
        vm.eval(p.manifest.name + ".on_unload rescue nil");
        p.active = false;
    }
}

} // namespace aether::plugin
