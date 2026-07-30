/**
 * @file exporter.hpp
 * @brief Exportiert ein Projekt zu einem spielbaren Paket + Game-Binary.
 */
#pragma once

#include <aether/shared/project_descriptor.hpp>

#include <filesystem>
#include <string>

namespace aether::editor {

namespace fs = std::filesystem;

struct ExportOptions {
    fs::path project_dir;
    fs::path output_dir;
    fs::path game_binary; ///< Pfad zur gebauten Game-Runtime
    bool copy_engine_assets = true;
    bool include_debug_symbols = false;
};

struct ExportResult {
    bool ok = false;
    std::string message;
    fs::path package_dir;
};

/**
 * @brief Kopiert Projekt + Runtime in ein verteilbares Verzeichnis.
 *
 * Struktur:
 *   MyGame_Export/
 *     Game[.exe]
 *     project.json
 *     data/ maps/ graphics/ audio/ scripts/ plugins/
 *     assets/engine/
 */
ExportResult export_project(const ExportOptions& options);

} // namespace aether::editor
