/**
 * @file mesh.hpp
 * @brief CPU-Meshdaten und optionale GPU-Ressourcen.
 */
#pragma once

#include <aether/core/types.hpp>
#include <aether/render/math.hpp>

#include <memory>
#include <string>
#include <vector>

namespace aether::render {

/**
 * @brief Vertex-Layout für stilisiertes Forward-Rendering.
 */
struct Vertex {
    Vec3 position{0.0f};
    Vec3 normal{0.0f, 1.0f, 0.0f};
    Vec2 uv{0.0f};
    Vec4 color{1.0f};
};

/**
 * @brief Eine LOD-Stufe eines Meshes.
 */
struct MeshLod {
    std::vector<Vertex> vertices;
    std::vector<u32> indices;
    AABB local_bounds{};
    f32 max_distance = 1.0e9f; ///< bis zu dieser Kameradistanz nutzen

    void recompute_bounds();
};

/**
 * @brief Mesh mit bis zu N LOD-Stufen (0 = höchste Qualität).
 */
class Mesh {
public:
    static constexpr usize kMaxLods = 4;

    Mesh() = default;
    explicit Mesh(std::string name);

    [[nodiscard]] const std::string& name() const noexcept { return name_; }
    void set_name(std::string n) { name_ = std::move(n); }

    /**
     * @brief Setzt/ersetzt eine LOD-Stufe.
     */
    void set_lod(usize level, MeshLod lod);

    [[nodiscard]] usize lod_count() const noexcept { return lods_.size(); }
    [[nodiscard]] const MeshLod& lod(usize level) const;
    [[nodiscard]] MeshLod& lod(usize level);

    /**
     * @brief Wählt LOD anhand Distanz (erste Stufe mit max_distance >= dist).
     */
    [[nodiscard]] usize select_lod(f32 distance) const noexcept;

    [[nodiscard]] const AABB& bounds() const noexcept { return bounds_; }

    /**
     * @brief Erzeugt ein Einheitswürfel-Mesh (1 LOD).
     */
    [[nodiscard]] static std::shared_ptr<Mesh> create_cube(f32 size = 1.0f);

    /**
     * @brief Erzeugt eine XZ-Plane.
     */
    [[nodiscard]] static std::shared_ptr<Mesh> create_plane(f32 size = 10.0f);

    /** @brief GPU-Upload-Flag (Renderer setzt dies). */
    void set_gpu_ready(bool v) noexcept { gpu_ready_ = v; }
    [[nodiscard]] bool gpu_ready() const noexcept { return gpu_ready_; }

    /** @brief Backend-spezifisches Handle (z. B. VAO-Id), 0 = keines. */
    void set_gpu_handle(u32 h) noexcept { gpu_handle_ = h; }
    [[nodiscard]] u32 gpu_handle() const noexcept { return gpu_handle_; }

private:
    void recompute_combined_bounds();

    std::string name_;
    std::vector<MeshLod> lods_;
    AABB bounds_{};
    bool gpu_ready_ = false;
    u32 gpu_handle_ = 0;
};

} // namespace aether::render
