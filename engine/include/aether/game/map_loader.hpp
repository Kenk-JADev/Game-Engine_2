/**
 * @file map_loader.hpp
 * @brief Lädt/ speichert Karten (maps/mapXXX.json) und richtet Default-Meshes ein.
 */
#pragma once

#include <aether/core/types.hpp>
#include <aether/render/renderer.hpp>
#include <aether/res/resource_manager.hpp>
#include <aether/scene/scene.hpp>

#include <filesystem>
#include <memory>
#include <string>

namespace aether::game {

struct MapLoadResult {
    std::unique_ptr<scene::Scene> scene;
    std::string message;
    bool ok = false;
};

/**
 * @brief Lädt eine Karte; fehlt die Datei, wird eine Default-Karte erzeugt.
 */
[[nodiscard]] MapLoadResult load_map(const std::filesystem::path& map_file,
                                     res::ResourceManager* resources = nullptr,
                                     render::Renderer* renderer = nullptr);

/**
 * @brief Speichert Scene als map JSON.
 */
[[nodiscard]] Result<void> save_map(const std::filesystem::path& map_file,
                                    const scene::Scene& scene);

/**
 * @brief Erzeugt Default-Startkarte mit Boden + Spieler-Spawn + Beispiel-NPC/Event.
 */
[[nodiscard]] std::unique_ptr<scene::Scene> create_default_map(
    const std::string& name,
    render::Renderer* renderer = nullptr);

/**
 * @brief map_id → maps/map001.json Pfadkonvention.
 */
[[nodiscard]] std::filesystem::path map_path_for_id(const std::filesystem::path& maps_dir,
                                                   u32 map_id);

} // namespace aether::game
