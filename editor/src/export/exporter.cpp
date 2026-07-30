/**
 * @file exporter.cpp
 */
#include "exporter.hpp"

#include <aether/core/logger.hpp>

#include <fstream>

namespace aether::editor {

ExportResult export_project(const ExportOptions& options) {
    ExportResult result;
    std::error_code ec;

    if (options.project_dir.empty() || options.output_dir.empty()) {
        result.message = "project_dir and output_dir required";
        return result;
    }
    if (!fs::exists(options.project_dir / "project.json", ec)) {
        result.message = "project.json not found";
        return result;
    }

    fs::create_directories(options.output_dir, ec);
    if (ec) {
        result.message = ec.message();
        return result;
    }

    // Projektinhalte
    const char* dirs[] = {"data", "maps", "graphics", "audio", "scripts", "plugins"};
    for (const char* d : dirs) {
        const fs::path src = options.project_dir / d;
        if (fs::exists(src, ec)) {
            fs::copy(src, options.output_dir / d,
                     fs::copy_options::recursive | fs::copy_options::overwrite_existing, ec);
        } else {
            fs::create_directories(options.output_dir / d, ec);
        }
    }
    fs::copy_file(options.project_dir / "project.json",
                  options.output_dir / "project.json",
                  fs::copy_options::overwrite_existing, ec);

    // Runtime-Binary
    if (!options.game_binary.empty() && fs::exists(options.game_binary, ec)) {
        const auto dest_name = options.game_binary.filename();
        fs::copy_file(options.game_binary, options.output_dir / dest_name,
                      fs::copy_options::overwrite_existing, ec);
#if !defined(_WIN32)
        fs::permissions(options.output_dir / dest_name,
                        fs::perms::owner_all | fs::perms::group_read | fs::perms::group_exec |
                            fs::perms::others_read | fs::perms::others_exec,
                        ec);
#endif
    } else {
        core::log_warn("Export", "Game binary not copied – path missing");
    }

    // Engine-Assets neben Binary
    if (options.copy_engine_assets) {
        const fs::path eng = fs::path("assets") / "engine";
        if (fs::exists(eng, ec)) {
            fs::copy(eng, options.output_dir / "assets" / "engine",
                     fs::copy_options::recursive | fs::copy_options::overwrite_existing, ec);
        }
    }

    // Start-Hinweis
    {
        std::ofstream readme(options.output_dir / "README_PLAY.txt");
        readme << "AetherRPG – exported game package\n"
               << "Run the Game executable in this folder.\n"
               << "The runtime loads project.json automatically from the working directory.\n";
    }

    result.ok = true;
    result.package_dir = options.output_dir;
    result.message = "Export complete";
    core::log_info("Export", "Exported to " + options.output_dir.string());
    return result;
}

} // namespace aether::editor
