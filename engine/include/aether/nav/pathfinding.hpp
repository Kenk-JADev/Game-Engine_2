/**
 * @file pathfinding.hpp
 * @brief Grid-basierte Pfadfindung für NPCs (automatisch aus Kollision).
 */
#pragma once

#include <aether/core/types.hpp>
#include <aether/phys/collision.hpp>
#include <aether/render/math.hpp>

#include <cmath>
#include <optional>
#include <vector>

namespace aether::nav {

using render::Vec3;

struct GridCoord {
    i32 x = 0;
    i32 z = 0;
    bool operator==(const GridCoord& o) const noexcept { return x == o.x && z == o.z; }
};

struct NavGrid {
    f32 origin_x = 0.0f;
    f32 origin_z = 0.0f;
    f32 cell_size = 1.0f;
    i32 width = 0;
    i32 height = 0;
    std::vector<u8> walkable; ///< 1 = begehbar

    [[nodiscard]] bool in_bounds(GridCoord c) const noexcept {
        return c.x >= 0 && c.z >= 0 && c.x < width && c.z < height;
    }
    [[nodiscard]] bool is_walkable(GridCoord c) const noexcept {
        return in_bounds(c) && walkable[static_cast<usize>(c.z * width + c.x)] != 0;
    }
    [[nodiscard]] GridCoord world_to_grid(const Vec3& p) const noexcept {
        return GridCoord{
            static_cast<i32>(std::floor((p.x - origin_x) / cell_size)),
            static_cast<i32>(std::floor((p.z - origin_z) / cell_size)),
        };
    }
    [[nodiscard]] Vec3 grid_to_world(GridCoord c) const noexcept {
        return Vec3{
            origin_x + (static_cast<f32>(c.x) + 0.5f) * cell_size,
            0.0f,
            origin_z + (static_cast<f32>(c.z) + 0.5f) * cell_size,
        };
    }
};

/**
 * @brief Baut ein Grid aus der Kollisionswelt (Static-Bodies blockieren).
 */
[[nodiscard]] NavGrid bake_nav_grid(const phys::CollisionWorld& world,
                                    f32 min_x, f32 min_z, f32 max_x, f32 max_z,
                                    f32 cell_size = 1.0f,
                                    f32 agent_radius = 0.35f);

/**
 * @brief A*-Pfad auf dem Grid. Liefert Weltpunkte (Zellmittelpunkte).
 */
[[nodiscard]] std::vector<Vec3> find_path(const NavGrid& grid, const Vec3& start,
                                          const Vec3& goal);

/**
 * @brief Einfacher Agent, der einem Pfad folgt.
 */
class NavAgent {
public:
    void set_path(std::vector<Vec3> path);
    void clear();

    /**
     * @brief Bewegt current Richtung nächstem Waypoint.
     * @return Bewegungsdelta für diesen Tick
     */
    Vec3 update(const Vec3& current, f32 speed, f64 dt);

    [[nodiscard]] bool has_path() const noexcept { return !path_.empty(); }
    [[nodiscard]] const std::vector<Vec3>& path() const noexcept { return path_; }

private:
    std::vector<Vec3> path_;
    usize index_ = 0;
};

} // namespace aether::nav
