/**
 * @file scene.hpp
 * @brief Einfacher Szenengraph für Kartenobjekte (kein User-ECS).
 *
 * Objekte werden per Drag&Drop platziert; Kollision/Nav entstehen automatisch.
 */
#pragma once

#include <aether/core/types.hpp>
#include <aether/game/event_system.hpp>
#include <aether/nav/pathfinding.hpp>
#include <aether/phys/collision.hpp>
#include <aether/render/material.hpp>
#include <aether/render/mesh.hpp>
#include <aether/render/renderable.hpp>

#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace aether::scene {

enum class ObjectType {
    Prop,
    Character,
    Npc,
    Enemy,
    Event,
    Light,
    Spawn,
};

struct SceneObject {
    EntityId id = kInvalidEntity;
    std::string name;
    ObjectType type = ObjectType::Prop;
    render::Transform transform{};
    std::shared_ptr<render::Mesh> mesh;
    render::Material material = render::Material::make_default();
    bool visible = true;
    u32 collision_id = 0;
    std::optional<game::MapEvent> map_event;
};

/**
 * @brief Eine Spielkarte / Szene.
 */
class Scene {
public:
    explicit Scene(std::string name = "Map");

    [[nodiscard]] const std::string& name() const noexcept { return name_; }
    void set_name(std::string n) { name_ = std::move(n); }

    EntityId add_object(SceneObject obj);
    void remove_object(EntityId id);
    SceneObject* find(EntityId id);
    [[nodiscard]] const SceneObject* find(EntityId id) const;
    [[nodiscard]] std::vector<SceneObject>& objects() noexcept { return objects_; }
    [[nodiscard]] const std::vector<SceneObject>& objects() const noexcept {
        return objects_;
    }

    /**
     * @brief Platziert Objekt und richtet Kollision automatisch ein.
     */
    EntityId place(ObjectType type, std::string name,
                   std::shared_ptr<render::Mesh> mesh,
                   const render::Transform& transform);

    void rebuild_collision();
    void bake_navigation(f32 padding = 5.0f, f32 cell = 1.0f);

    [[nodiscard]] phys::CollisionWorld& collision() noexcept { return collision_; }
    [[nodiscard]] const phys::CollisionWorld& collision() const noexcept {
        return collision_;
    }
    [[nodiscard]] const nav::NavGrid& nav_grid() const noexcept { return nav_grid_; }
    [[nodiscard]] bool has_nav() const noexcept { return nav_ready_; }

    /**
     * @brief Sammelt Renderables für den Renderer.
     */
    void collect_renderables(std::vector<render::Renderable>& out) const;

    // JSON
    [[nodiscard]] nlohmann::json to_json() const;
    static void load_from_json(Scene& out, const nlohmann::json& j);
    [[nodiscard]] static std::unique_ptr<Scene> create_from_json(const nlohmann::json& j);

private:
    phys::ObjectKind to_phys_kind(ObjectType t) const;

    std::string name_;
    std::vector<SceneObject> objects_;
    EntityId next_id_ = 1;
    phys::CollisionWorld collision_;
    nav::NavGrid nav_grid_{};
    bool nav_ready_ = false;
};

class SceneManager {
public:
    void set_active(std::unique_ptr<Scene> scene);
    [[nodiscard]] Scene* active() noexcept { return active_.get(); }
    [[nodiscard]] const Scene* active() const noexcept { return active_.get(); }

private:
    std::unique_ptr<Scene> active_;
};

} // namespace aether::scene
