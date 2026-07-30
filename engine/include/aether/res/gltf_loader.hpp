/**
 * @file gltf_loader.hpp
 * @brief glTF/GLB → Mesh (cgltf).
 */
#pragma once

#include <aether/core/types.hpp>
#include <aether/render/mesh.hpp>

#include <filesystem>
#include <memory>

namespace aether::res {

/**
 * @brief Lädt das erste Mesh aus einer glTF/GLB-Datei.
 * @return nullptr bei Fehler
 */
[[nodiscard]] std::shared_ptr<render::Mesh> load_gltf_mesh(
    const std::filesystem::path& path);

} // namespace aether::res
