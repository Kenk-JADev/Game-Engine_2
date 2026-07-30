/**
 * @file camera.hpp
 * @brief Perspektiv-/Orthokamera und Frustum.
 */
#pragma once

#include <aether/render/math.hpp>

namespace aether::render {

/**
 * @brief Frustum als 6 Ebenen (nx,ny,nz,d) mit |n|≈1, n·x + d >= 0 innen.
 */
struct Frustum {
    Vec4 planes[6]{};

    enum PlaneIndex : int {
        Left = 0,
        Right,
        Bottom,
        Top,
        Near,
        Far
    };

    /**
     * @brief Testet eine Kugel gegen das Frustum.
     * @return false wenn vollständig außerhalb
     */
    [[nodiscard]] bool intersects_sphere(const BoundingSphere& s) const noexcept {
        for (const auto& p : planes) {
            const f32 dist = p.x * s.center.x + p.y * s.center.y + p.z * s.center.z + p.w;
            if (dist < -s.radius) {
                return false;
            }
        }
        return true;
    }

    [[nodiscard]] bool intersects_aabb(const AABB& box) const noexcept {
        return intersects_sphere(BoundingSphere::from_aabb(box));
    }
};

/**
 * @brief Extrahiert Frustum-Ebenen aus View-Projection (Gribb/Hartmann).
 */
[[nodiscard]] Frustum extract_frustum(const Mat4& view_proj);

/**
 * @brief 3D-Kamera für stilierte RPGs.
 */
class Camera {
public:
    enum class Projection {
        Perspective,
        Orthographic,
    };

    void set_perspective(f32 fov_y_degrees, f32 aspect, f32 near_plane, f32 far_plane);
    void set_orthographic(f32 left, f32 right, f32 bottom, f32 top, f32 near_plane, f32 far_plane);

    void set_position(const Vec3& p) noexcept { position_ = p; dirty_ = true; }
    void set_target(const Vec3& t) noexcept { target_ = t; dirty_ = true; }
    void set_up(const Vec3& u) noexcept { up_ = u; dirty_ = true; }

    void look_at(const Vec3& eye, const Vec3& center, const Vec3& up);

    [[nodiscard]] const Vec3& position() const noexcept { return position_; }
    [[nodiscard]] const Vec3& target() const noexcept { return target_; }
    [[nodiscard]] f32 near_plane() const noexcept { return near_; }
    [[nodiscard]] f32 far_plane() const noexcept { return far_; }
    [[nodiscard]] f32 fov_y_degrees() const noexcept { return fov_y_; }
    [[nodiscard]] f32 aspect() const noexcept { return aspect_; }
    [[nodiscard]] Projection projection_type() const noexcept { return proj_type_; }

    [[nodiscard]] const Mat4& view_matrix() const;
    [[nodiscard]] const Mat4& projection_matrix() const;
    [[nodiscard]] Mat4 view_projection_matrix() const;
    [[nodiscard]] Frustum frustum() const;

    /**
     * @brief LOD-Distanz: Entfernung Kamera → Punkt.
     */
    [[nodiscard]] f32 distance_to(const Vec3& point) const noexcept {
        return glm::length(point - position_);
    }

    /**
     * @brief Screen-Pixel (origin top-left) → Weltstrahl.
     */
    void screen_to_ray(f32 screen_x, f32 screen_y, f32 viewport_w, f32 viewport_h,
                       Vec3& out_origin, Vec3& out_dir) const;

    /**
     * @brief Schnittpunkt Strahl ↔ Ebene y = plane_y.
     * @return false wenn parallel / hinter Kamera
     */
    [[nodiscard]] static bool ray_plane_y(const Vec3& origin, const Vec3& dir, f32 plane_y,
                                          Vec3& out_hit) noexcept;

    /**
     * @brief Strahl ↔ AABB (slab method).
     * @return true bei Treffer; tmin Distanz
     */
    [[nodiscard]] static bool ray_aabb(const Vec3& origin, const Vec3& dir, const AABB& box,
                                       f32& tmin) noexcept;

private:
    void recompute() const;

    Vec3 position_{0.0f, 5.0f, 10.0f};
    Vec3 target_{0.0f, 0.0f, 0.0f};
    Vec3 up_{0.0f, 1.0f, 0.0f};

    Projection proj_type_ = Projection::Perspective;
    f32 fov_y_ = 45.0f;
    f32 aspect_ = 16.0f / 9.0f;
    f32 near_ = 0.1f;
    f32 far_ = 500.0f;
    f32 ortho_l_ = -10.0f, ortho_r_ = 10.0f, ortho_b_ = -10.0f, ortho_t_ = 10.0f;

    mutable Mat4 view_{1.0f};
    mutable Mat4 proj_{1.0f};
    mutable bool dirty_ = true;
};

} // namespace aether::render
