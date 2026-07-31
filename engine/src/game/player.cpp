/**
 * @file player.cpp
 */
#include <aether/game/player.hpp>
#include <aether/render/camera.hpp>

#include <cmath>

namespace aether::game {

void PlayerController::bind(scene::Scene* scene, EntityId entity_id, u32 collision_id) {
    scene_ = scene;
    entity_id_ = entity_id;
    collision_id_ = collision_id;
    if (scene_) {
        if (auto* o = scene_->find(entity_id_)) {
            position_ = o->transform.position;
            if (collision_id_ == 0) {
                collision_id_ = o->collision_id;
            }
        }
    }
}

void PlayerController::set_position(const render::Vec3& p) {
    position_ = p;
    position_.y = cfg_.height;
    if (scene_) {
        if (auto* o = scene_->find(entity_id_)) {
            o->transform.position = position_;
            if (collision_id_) {
                scene_->collision().set_transform(collision_id_, o->transform);
            }
        }
    }
}

bool PlayerController::update_movement(const input::InputManager& input, f64 dt,
                                       phys::CollisionWorld* world) {
    render::Vec3 dir{0.0f};
    if (input.is_down("up")) dir.z -= 1.0f;
    if (input.is_down("down")) dir.z += 1.0f;
    if (input.is_down("left")) dir.x -= 1.0f;
    if (input.is_down("right")) dir.x += 1.0f;

    if (glm::length(dir) < 1.0e-4f) {
        return false;
    }
    dir = glm::normalize(dir);
    // facing
    if (std::fabs(dir.x) > std::fabs(dir.z)) {
        facing_ = dir.x < 0 ? 4 : 6;
    } else {
        facing_ = dir.z < 0 ? 8 : 2;
    }

    const render::Vec3 delta = dir * cfg_.move_speed * static_cast<f32>(dt);
    render::Vec3 next = position_ + delta;

    if (world && collision_id_) {
        next = world->move_and_collide(collision_id_, delta);
    }
    // Terrain-Höhe automatisch (stilisierte RPGs: keine Sprungphysik)
    if (scene_ && scene_->terrain().valid()) {
        next.y = scene_->terrain().height_at(next.x, next.z) + cfg_.height;
    } else {
        next.y = cfg_.height;
    }

    if (glm::length(next - position_) < 1.0e-5f) {
        return false;
    }
    position_ = next;
    if (scene_) {
        if (auto* o = scene_->find(entity_id_)) {
            o->transform.position = position_;
            // yaw
            f32 yaw = 0.0f;
            if (facing_ == 2) yaw = 0.0f;
            if (facing_ == 8) yaw = 180.0f;
            if (facing_ == 4) yaw = 90.0f;
            if (facing_ == 6) yaw = -90.0f;
            o->transform.rotation =
                glm::angleAxis(render::radians(yaw), render::Vec3{0, 1, 0});
            if (collision_id_) {
                world->set_transform(collision_id_, o->transform);
            }
        }
    }
    return true;
}

EntityId PlayerController::find_interact_target(const scene::Scene& scene) const {
    render::Vec3 fwd{0, 0, 1};
    if (facing_ == 8) fwd = {0, 0, -1};
    if (facing_ == 4) fwd = {-1, 0, 0};
    if (facing_ == 6) fwd = {1, 0, 0};

    EntityId best = kInvalidEntity;
    f32 best_d = cfg_.interaction_range;
    for (const auto& o : scene.objects()) {
        if (o.id == entity_id_ || !o.map_event) {
            continue;
        }
        const render::Vec3 to = o.transform.position - position_;
        const f32 dist = glm::length(render::Vec3{to.x, 0, to.z});
        if (dist > cfg_.interaction_range) {
            continue;
        }
        const f32 dot = glm::dot(glm::normalize(render::Vec3{to.x, 0, to.z} +
                                                render::Vec3{1e-4f, 0, 0}),
                                 fwd);
        if (dot < 0.25f && dist > 0.6f) {
            continue; // roughly in front
        }
        if (dist < best_d) {
            best_d = dist;
            best = o.id;
        }
    }
    return best;
}

void FollowCamera::snap(const render::Vec3& target) {
    look_ = target;
    eye_ = target + render::Vec3{0.0f, height_, back_};
}

void FollowCamera::update(const render::Vec3& target, f64 dt) {
    const render::Vec3 desired_look = target;
    const render::Vec3 desired_eye = target + render::Vec3{0.0f, height_, back_};
    const f32 k = 1.0f - std::exp(-lag_ * static_cast<f32>(dt));
    look_ = glm::mix(look_, desired_look, k);
    eye_ = glm::mix(eye_, desired_eye, k);
}

void FollowCamera::apply(render::Camera& camera) const {
    camera.look_at(eye_, look_, {0, 1, 0});
}

} // namespace aether::game
