/**
 * @file collision.cpp
 */
#include <aether/phys/collision.hpp>
#include <aether/core/logger.hpp>

#include <algorithm>
#include <cmath>

namespace aether::phys {

CollisionBody make_body_from_mesh_bounds(u32 id, ObjectKind kind, const AABB& mesh_bounds,
                                         const render::Transform& transform) {
    CollisionBody b;
    b.id = id;
    b.kind = kind;
    b.local_bounds = mesh_bounds;
    b.transform = transform;

    switch (kind) {
    case ObjectKind::Player:
    case ObjectKind::Npc:
    case ObjectKind::Enemy:
        b.type = BodyType::Kinematic;
        b.block_movement = true;
        // Character: oft schlankere Kapsel aus Bounds (XZ + Höhe)
        break;
    case ObjectKind::Event:
        b.type = BodyType::Trigger;
        b.block_movement = false;
        break;
    case ObjectKind::Terrain:
        b.type = BodyType::Static;
        break;
    case ObjectKind::Prop:
        b.type = BodyType::Static;
        break;
    case ObjectKind::Generic:
    default:
        b.type = BodyType::Static;
        break;
    }
    return b;
}

bool CollisionWorld::aabb_overlap(const AABB& a, const AABB& b) noexcept {
    return (a.min.x <= b.max.x && a.max.x >= b.min.x) &&
           (a.min.y <= b.max.y && a.max.y >= b.min.y) &&
           (a.min.z <= b.max.z && a.max.z >= b.min.z);
}

f32 CollisionWorld::aabb_penetration_xz(const AABB& a, const AABB& b, Vec3& out_normal) noexcept {
    const f32 ox = std::min(a.max.x, b.max.x) - std::max(a.min.x, b.min.x);
    const f32 oz = std::min(a.max.z, b.max.z) - std::max(a.min.z, b.min.z);
    if (ox <= 0.0f || oz <= 0.0f) {
        out_normal = Vec3(0);
        return 0.0f;
    }
    if (ox < oz) {
        const f32 acx = (a.min.x + a.max.x) * 0.5f;
        const f32 bcx = (b.min.x + b.max.x) * 0.5f;
        out_normal = Vec3(acx < bcx ? -1.0f : 1.0f, 0.0f, 0.0f);
        return ox;
    }
    const f32 acz = (a.min.z + a.max.z) * 0.5f;
    const f32 bcz = (b.min.z + b.max.z) * 0.5f;
    out_normal = Vec3(0.0f, 0.0f, acz < bcz ? -1.0f : 1.0f);
    return oz;
}

u32 CollisionWorld::add(CollisionBody body) {
    if (body.id == 0) {
        body.id = next_id_++;
    } else {
        next_id_ = std::max(next_id_, body.id + 1);
    }
    bodies_.push_back(std::move(body));
    return bodies_.back().id;
}

void CollisionWorld::remove(u32 id) {
    bodies_.erase(std::remove_if(bodies_.begin(), bodies_.end(),
                                 [id](const CollisionBody& b) { return b.id == id; }),
                  bodies_.end());
}

void CollisionWorld::clear() {
    bodies_.clear();
}

CollisionBody* CollisionWorld::find(u32 id) {
    for (auto& b : bodies_) {
        if (b.id == id) return &b;
    }
    return nullptr;
}

const CollisionBody* CollisionWorld::find(u32 id) const {
    for (const auto& b : bodies_) {
        if (b.id == id) return &b;
    }
    return nullptr;
}

void CollisionWorld::set_transform(u32 id, const render::Transform& t) {
    if (auto* b = find(id)) {
        b->transform = t;
    }
}

Vec3 CollisionWorld::move_and_collide(u32 id, const Vec3& delta) {
    auto* body = find(id);
    if (!body) {
        return Vec3(0);
    }
    if (!body->block_movement) {
        body->transform.position += delta;
        return body->transform.position;
    }

    // Sub-step large deltas to avoid tunneling through thin walls
    const f32 max_step = 0.25f;
    const f32 len = glm::length(Vec3{delta.x, 0.0f, delta.z});
    const int steps = std::max(1, static_cast<int>(std::ceil(len / max_step)));
    const Vec3 step = delta / static_cast<f32>(steps);

    for (int s = 0; s < steps; ++s) {
        body->transform.position += step;

        for (int iter = 0; iter < 4; ++iter) {
            const AABB a = body->world_bounds();
            bool any = false;
            for (const auto& other : bodies_) {
                if (other.id == id || !other.enabled || !other.block_movement) continue;
                if (other.type == BodyType::Trigger) continue;
                const AABB b = other.world_bounds();
                AABB aa = a, bb = b;
                aa.min.y = 0;
                aa.max.y = 1;
                bb.min.y = 0;
                bb.max.y = 1;
                if (!aabb_overlap(aa, bb)) continue;
                Vec3 n;
                const f32 pen = aabb_penetration_xz(aa, bb, n);
                if (pen > 0.0f) {
                    body->transform.position += n * (pen + 1.0e-4f);
                    any = true;
                }
            }
            if (!any) break;
        }
    }
    return body->transform.position;
}

std::vector<CollisionHit> CollisionWorld::query_overlaps() const {
    std::vector<CollisionHit> hits;
    for (usize i = 0; i < bodies_.size(); ++i) {
        if (!bodies_[i].enabled) continue;
        const AABB a = bodies_[i].world_bounds();
        for (usize j = i + 1; j < bodies_.size(); ++j) {
            if (!bodies_[j].enabled) continue;
            const AABB b = bodies_[j].world_bounds();
            if (!aabb_overlap(a, b)) continue;
            CollisionHit h;
            h.body_a = bodies_[i].id;
            h.body_b = bodies_[j].id;
            h.trigger = (bodies_[i].type == BodyType::Trigger ||
                         bodies_[j].type == BodyType::Trigger);
            Vec3 n;
            h.penetration = aabb_penetration_xz(a, b, n);
            h.normal = n;
            hits.push_back(h);
        }
    }
    return hits;
}

} // namespace aether::phys
