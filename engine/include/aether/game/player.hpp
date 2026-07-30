/**
 * @file player.hpp
 * @brief Spieler-Controller (Bewegung, Interaktion) – Engine-intern.
 */
#pragma once

#include <aether/core/types.hpp>
#include <aether/input/input_manager.hpp>
#include <aether/phys/collision.hpp>
#include <aether/render/camera.hpp>
#include <aether/render/math.hpp>
#include <aether/scene/scene.hpp>

namespace aether::game {

struct PlayerConfig {
    f32 move_speed = 4.5f;
    f32 interaction_range = 1.6f;
    f32 height = 0.0f; ///< Y fix für stilisiertes RPG
};

/**
 * @brief Steuert eine Scene-Entity als Spieler.
 */
class PlayerController {
public:
    void bind(scene::Scene* scene, EntityId entity_id, u32 collision_id = 0);
    void set_config(PlayerConfig cfg) { cfg_ = cfg; }

    /**
     * @brief Fixed-update Bewegung anhand Input-Actions.
     * @return true wenn Position geändert
     */
    bool update_movement(const input::InputManager& input, f64 dt,
                         phys::CollisionWorld* world);

    /**
     * @brief Sucht nächstes Event-Objekt in Blick-/Interaktionsrichtung.
     */
    [[nodiscard]] EntityId find_interact_target(const scene::Scene& scene) const;

    [[nodiscard]] EntityId entity_id() const noexcept { return entity_id_; }
    [[nodiscard]] render::Vec3 position() const noexcept { return position_; }
    [[nodiscard]] i32 facing() const noexcept { return facing_; } ///< 2 down 4 left 6 right 8 up
    void set_position(const render::Vec3& p);

private:
    scene::Scene* scene_ = nullptr;
    EntityId entity_id_ = kInvalidEntity;
    u32 collision_id_ = 0;
    render::Vec3 position_{0.0f};
    i32 facing_ = 2;
    PlayerConfig cfg_{};
};

/**
 * @brief Einfache Follow-Kamera (RPG-Stil, etwas erhöht + Abstand).
 */
class FollowCamera {
public:
    void set_offsets(f32 height, f32 back, f32 lag = 8.0f) {
        height_ = height;
        back_ = back;
        lag_ = lag;
    }
    void snap(const render::Vec3& target);
    void update(const render::Vec3& target, f64 dt);
    void apply(render::Camera& camera) const;

private:
    render::Vec3 eye_{0, 10, 12};
    render::Vec3 look_{0, 0, 0};
    f32 height_ = 10.0f;
    f32 back_ = 12.0f;
    f32 lag_ = 8.0f;
};

} // namespace aether::game
