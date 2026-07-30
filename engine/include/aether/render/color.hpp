/**
 * @file color.hpp
 * @brief RGBA-Farbe.
 */
#pragma once

#include <aether/core/types.hpp>
#include <aether/render/math.hpp>

namespace aether::render {

struct Color {
    f32 r = 1.0f;
    f32 g = 1.0f;
    f32 b = 1.0f;
    f32 a = 1.0f;

    constexpr Color() = default;
    constexpr Color(f32 r_, f32 g_, f32 b_, f32 a_ = 1.0f) : r(r_), g(g_), b(b_), a(a_) {}

    [[nodiscard]] static constexpr Color white()   { return {1, 1, 1, 1}; }
    [[nodiscard]] static constexpr Color black()   { return {0, 0, 0, 1}; }
    [[nodiscard]] static constexpr Color red()     { return {1, 0, 0, 1}; }
    [[nodiscard]] static constexpr Color green()   { return {0, 1, 0, 1}; }
    [[nodiscard]] static constexpr Color blue()    { return {0, 0, 1, 1}; }
    [[nodiscard]] static constexpr Color cornflower() { return {0.392f, 0.584f, 0.929f, 1}; }

    [[nodiscard]] Vec4 to_vec4() const noexcept { return {r, g, b, a}; }
};

} // namespace aether::render
