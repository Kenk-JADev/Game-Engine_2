/**
 * @file shader.hpp
 * @brief Shader-Quellen und Programm-Handle (Backend-neutral).
 */
#pragma once

#include <aether/core/types.hpp>

#include <string>
#include <unordered_map>

namespace aether::render {

/**
 * @brief GLSL-Quellenpaar.
 */
struct ShaderSource {
    std::string name;
    std::string vertex;
    std::string fragment;
};

/**
 * @brief Kompiliertes / registriertes Shader-Programm.
 *
 * Ohne OpenGL bleibt gpu_id == 0 und ready == false;
 * der Null-Renderer akzeptiert das für Culling-Tests.
 */
class ShaderProgram {
public:
    ShaderProgram() = default;
    explicit ShaderProgram(std::string name);

    [[nodiscard]] const std::string& name() const noexcept { return name_; }
    [[nodiscard]] u32 gpu_id() const noexcept { return gpu_id_; }
    [[nodiscard]] bool ready() const noexcept { return ready_; }

    void set_gpu_id(u32 id) noexcept { gpu_id_ = id; ready_ = id != 0; }
    void set_ready(bool v) noexcept { ready_ = v; }

    void set_source(ShaderSource src) { source_ = std::move(src); }
    [[nodiscard]] const ShaderSource& source() const noexcept { return source_; }

    /**
     * @brief Built-in einfacher Farb-Shader (GLSL 330).
     */
    [[nodiscard]] static ShaderSource builtin_unlit_color();

    /**
     * @brief Built-in einfacher Lit-Shader (1 direktionales Licht).
     */
    [[nodiscard]] static ShaderSource builtin_lit_basic();

private:
    std::string name_;
    ShaderSource source_{};
    u32 gpu_id_ = 0;
    bool ready_ = false;
};

/**
 * @brief Einfache Shader-Bibliothek nach Name.
 */
class ShaderLibrary {
public:
    ShaderProgram& add(ShaderProgram program);
    [[nodiscard]] ShaderProgram* find(std::string_view name);
    [[nodiscard]] const ShaderProgram* find(std::string_view name) const;

    void clear();

private:
    std::unordered_map<std::string, ShaderProgram> programs_;
};

} // namespace aether::render
