/**
 * @file material.hpp
 * @brief Einfaches Forward-Material (stilisierte RPGs).
 */
#pragma once

#include <aether/core/types.hpp>
#include <aether/render/color.hpp>

#include <string>

namespace aether::render {

/**
 * @brief Materialparameter – bewusst einfach, kein PBR-Zwang.
 */
struct Material {
    std::string name = "default";
    Color albedo = Color::white();
    f32  roughness = 0.8f;     ///< nur soft-tint, kein Raytracing
    f32  metallic  = 0.0f;
    bool transparent = false;
    f32  alpha_cutoff = 0.0f;  ///< 0 = opaque cutout aus
    i32  texture_id = -1;      ///< Resource-Id, -1 = keine
    i32  shader_id  = 0;       ///< 0 = Built-in unlit/lit

    [[nodiscard]] static Material make_default() {
        Material m;
        m.name = "default";
        return m;
    }
};

} // namespace aether::render
