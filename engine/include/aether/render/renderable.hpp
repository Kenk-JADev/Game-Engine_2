/**
 * @file renderable.hpp
 * @brief Einreichbare Render-Objekte und Queue.
 */
#pragma once

#include <aether/render/material.hpp>
#include <aether/render/math.hpp>
#include <aether/render/mesh.hpp>

#include <memory>
#include <vector>

namespace aether::res {
struct TextureData;
}

namespace aether::render {

/**
 * @brief Ein Objekt, das der Renderer zeichnen soll.
 *
 * Wird vom Scene-/Game-Layer befüllt – der User sieht das nicht als „Component“.
 */
struct Renderable {
    std::shared_ptr<Mesh> mesh;
    Material material = Material::make_default();
    Transform transform{};
    std::shared_ptr<res::TextureData> texture; ///< optionale Albedo-Textur
    bool visible = true;
    bool cast_shadows = false; ///< reserviert, Phase-1 ungenutzt
    i32 layer = 0;
};

/**
 * @brief Sortierter Draw-Item nach Culling/LOD.
 */
struct DrawItem {
    const Renderable* source = nullptr;
    usize lod_level = 0;
    f32  distance = 0.0f;
    Mat4 model{1.0f};
    AABB world_bounds{};
    std::shared_ptr<res::TextureData> texture; ///< wie Renderable
    bool transparent = false;
};

/**
 * @brief Frame-Statistiken.
 */
struct RenderStats {
    u32 submitted = 0;
    u32 culled = 0;
    u32 drawn = 0;
    u32 triangles = 0;
    u32 textured_draws = 0;   ///< Draws mit gebundener Textur
    u32 textures_uploaded = 0; ///< GPU-/Null-Uploads
    u32 lod_histogram[Mesh::kMaxLods]{};

    void reset() { *this = {}; }
};

} // namespace aether::render
