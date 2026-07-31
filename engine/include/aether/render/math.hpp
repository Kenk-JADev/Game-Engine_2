/**
 * @file math.hpp
 * @brief Mathematik-Typen der Render-Pipeline (glm-basiert).
 */
#pragma once

#include <aether/core/types.hpp>

#include <cmath>
#include <limits>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>

namespace aether::render {

using Vec2 = glm::vec2;
using Vec3 = glm::vec3;
using Vec4 = glm::vec4;
using Mat3 = glm::mat3;
using Mat4 = glm::mat4;
using Quat = glm::quat;

inline constexpr f32 kPi      = 3.14159265358979323846f;
inline constexpr f32 kTwoPi   = 6.28318530717958647692f;
inline constexpr f32 kDeg2Rad = kPi / 180.0f;
inline constexpr f32 kRad2Deg = 180.0f / kPi;

[[nodiscard]] inline f32 radians(f32 deg) noexcept { return deg * kDeg2Rad; }
[[nodiscard]] inline f32 degrees(f32 rad) noexcept { return rad * kRad2Deg; }

/**
 * @brief Achsen-ausgerichtete Bounding Box.
 */
struct AABB {
    Vec3 min{0.0f};
    Vec3 max{0.0f};

    [[nodiscard]] static AABB from_center_extents(const Vec3& center, const Vec3& extents) {
        return AABB{center - extents, center + extents};
    }

    [[nodiscard]] Vec3 center() const noexcept { return (min + max) * 0.5f; }
    [[nodiscard]] Vec3 extents() const noexcept { return (max - min) * 0.5f; }
    [[nodiscard]] Vec3 size() const noexcept { return max - min; }

    [[nodiscard]] f32 radius() const noexcept {
        const Vec3 e = extents();
        return std::sqrt(e.x * e.x + e.y * e.y + e.z * e.z);
    }

    void expand(const Vec3& p) {
        min = glm::min(min, p);
        max = glm::max(max, p);
    }

    void merge(const AABB& o) {
        min = glm::min(min, o.min);
        max = glm::max(max, o.max);
    }

    [[nodiscard]] AABB transformed(const Mat4& m) const {
        const Vec3 corners[8] = {
            {min.x, min.y, min.z}, {max.x, min.y, min.z},
            {min.x, max.y, min.z}, {max.x, max.y, min.z},
            {min.x, min.y, max.z}, {max.x, min.y, max.z},
            {min.x, max.y, max.z}, {max.x, max.y, max.z},
        };
        AABB out;
        out.min = Vec3(std::numeric_limits<f32>::max());
        out.max = Vec3(std::numeric_limits<f32>::lowest());
        for (const auto& c : corners) {
            const Vec4 t = m * Vec4(c, 1.0f);
            out.expand(Vec3(t) / t.w);
        }
        return out;
    }
};

/**
 * @brief Kugel für grobes Culling.
 */
struct BoundingSphere {
    Vec3 center{0.0f};
    f32  radius = 0.0f;

    [[nodiscard]] static BoundingSphere from_aabb(const AABB& box) {
        return BoundingSphere{box.center(), box.radius()};
    }
};

/**
 * @brief TRS-Transform.
 */
struct Transform {
    Vec3 position{0.0f};
    Quat rotation{1.0f, 0.0f, 0.0f, 0.0f}; // w, x, y, z
    Vec3 scale{1.0f};

    [[nodiscard]] Mat4 matrix() const {
        Mat4 m = glm::translate(Mat4(1.0f), position);
        m *= glm::mat4_cast(rotation);
        m = glm::scale(m, scale);
        return m;
    }

    [[nodiscard]] static Transform identity() { return {}; }
};

/**
 * @brief Modell-Matrix für ein Billboard (nur Yaw-Rotation Richtung Kamera).
 *
 * Charaktere/Sprites drehen sich um die Hochachse zur Kamera, bleiben aber
 * aufrecht – klassischer RPG-Stil.
 */
[[nodiscard]] inline Mat4 billboard_model(const Transform& t, const Vec3& camera_pos) {
    const Vec3 to_cam = camera_pos - t.position;
    const f32 yaw = std::atan2(to_cam.x, to_cam.z);
    Mat4 m = glm::translate(Mat4(1.0f), t.position);
    m *= glm::rotate(Mat4(1.0f), yaw, Vec3(0.0f, 1.0f, 0.0f));
    m = glm::scale(m, t.scale);
    return m;
}

} // namespace aether::render
