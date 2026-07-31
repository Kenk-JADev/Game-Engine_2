/**
 * @file fbx_loader.hpp
 * @brief FBX (ASCII + binär 7.x) → Mesh-Import ohne externe SDKs.
 *
 * Eigenständiger, schlanker FBX-Reader. Unterstützt den für stilisierte
 * 3D-RPGs üblichen Teilbereich:
 *   - ASCII-FBX und binäres FBX 7.x (Version 7000–7400)
 *   - Geometrie: Positionen, Normalen, UVs, Polygon-Indizes (Fan-Triangulation)
 *   - LayerElementNormal / LayerElementUV (Direct + IndexToDirect)
 *   - Model-Transform (Lcl Translation / Rotation / Scaling)
 *   - Material-Namen (werden geloggt; Mesh selbst bleibt materialfrei)
 *
 * Bewusst NICHT unterstützt (Fehler mit klarer Meldung):
 *   - zlib-komprimierte Arrays (Export ohne Kompression verwenden)
 *   - FBX-Animationen, Kameras, Lichter, Skinning
 */
#pragma once

#include <aether/core/types.hpp>
#include <aether/render/mesh.hpp>

#include <filesystem>
#include <memory>

namespace aether::res {

/**
 * @brief Lädt das erste/alle Meshes aus einer FBX-Datei (alle Model-Geometrien
 *        werden in ein einzelnes render::Mesh zusammengeführt).
 * @param path Pfad zur .fbx-Datei
 * @return nullptr bei Fehler (Fehlerdetails werden geloggt)
 */
[[nodiscard]] std::shared_ptr<render::Mesh> load_fbx_mesh(
    const std::filesystem::path& path);

} // namespace aether::res
