/**
 * @file collision.hpp
 * @brief Einfache AABB/Capsule-Kollision – automatisch aus Objektbounds.
 *
 * Der Autor konfiguriert keine Collider manuell; die Engine leitet
 * Kollisionskörper aus Mesh-Bounds und Objekttyp ab.
 */
#pragma once

#include <aether/core/types.hpp>
#include <aether/render/math.hpp>

#include <vector>

namespace aether::phys {

using render::AABB;
using render::Vec3;

enum class BodyType {
    Static,    ///< Weltgeometrie
    Dynamic,   ///< bewegliche Props
    Kinematic, ///< NPCs / Player (scripted)
    Trigger,   ///< Event-Auslöser ohne physischen Block
};

enum class ObjectKind {
    Generic,
    Player,
    Npc,
    Enemy,
    Prop,
    Terrain,
    Event,
};

/**
 * @brief Kollisionskörper (automatisch erzeugt).
 */
struct CollisionBody {
    u32 id = 0;
    BodyType type = BodyType::Static;
    ObjectKind kind = ObjectKind::Generic;
    AABB local_bounds{};
    render::Transform transform{};
    bool enabled = true;
    bool block_movement = true; ///< false bei Triggern
    std::string tag;

    [[nodiscard]] AABB world_bounds() const {
        return local_bounds.transformed(transform.matrix());
    }
};

struct CollisionHit {
    u32 body_a = 0;
    u32 body_b = 0;
    Vec3 normal{0, 1, 0};
    f32 penetration = 0.0f;
    bool trigger = false;
};

/**
 * @brief Erzeugt Standard-Bounds aus Mesh-AABB und Objektart.
 */
[[nodiscard]] CollisionBody make_body_from_mesh_bounds(
    u32 id,
    ObjectKind kind,
    const AABB& mesh_bounds,
    const render::Transform& transform);

/**
 * @brief Einfache Kollisionswelt (Breitensuche O(n²) – ok für RPG-Szenen).
 */
class CollisionWorld : public aether::NonMovable {
public:
    u32 add(CollisionBody body);
    void remove(u32 id);
    void clear();

    CollisionBody* find(u32 id);
    [[nodiscard]] const CollisionBody* find(u32 id) const;

    void set_transform(u32 id, const render::Transform& t);

    /**
     * @brief Bewegt einen Body und löst Static-Kollisionen auf.
     * @return finale Position
     */
    Vec3 move_and_collide(u32 id, const Vec3& delta);

    /**
     * @brief Sammelt alle Overlaps (inkl. Trigger).
     */
    [[nodiscard]] std::vector<CollisionHit> query_overlaps() const;

    [[nodiscard]] const std::vector<CollisionBody>& bodies() const noexcept {
        return bodies_;
    }

    static bool aabb_overlap(const AABB& a, const AABB& b) noexcept;
    static f32 aabb_penetration_xz(const AABB& a, const AABB& b, Vec3& out_normal) noexcept;

private:
    std::vector<CollisionBody> bodies_;
    u32 next_id_ = 1;
};

} // namespace aether::phys
