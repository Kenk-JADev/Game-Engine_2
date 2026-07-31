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

#include <cmath>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace aether::res {
struct TextureData;
}

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
    std::shared_ptr<res::TextureData> texture; ///< optionale Albedo-Textur (geladen)
    std::string texture_path;                  ///< logischer Pfad (JSON)
    bool billboard = false;                    ///< Sprite dreht sich zur Kamera
    bool visible = true;
    u32 collision_id = 0;
    std::optional<game::MapEvent> map_event;
};

/**
 * @brief Höhenfeld-Terrain der Karte (optional).
 *
 * Grid aus `width`×`depth` Zellen mit Zellgröße `cell`; `heights` enthält
 * (width+1)×(depth+1) Eck-Höhen (row-major, zuerst x dann z).
 */
struct TerrainData {
    i32 width = 8;
    i32 depth = 8;
    f32 cell = 4.0f;
    std::vector<f32> heights;
    std::string texture_path; ///< logischer Pfad (z. B. graphics/textures/grass.png)

    void reset(i32 w, i32 d, f32 c) {
        width = w;
        depth = d;
        cell = c;
        heights.assign(static_cast<usize>((w + 1) * (d + 1)), 0.0f);
    }
    [[nodiscard]] usize grid_w() const noexcept { return static_cast<usize>(width + 1); }
    [[nodiscard]] usize grid_h() const noexcept { return static_cast<usize>(depth + 1); }
    [[nodiscard]] bool valid() const noexcept {
        return width > 0 && depth > 0 && cell > 0.0f &&
               heights.size() == grid_w() * grid_h();
    }
    [[nodiscard]] f32 height_at(f32 x, f32 z) const noexcept {
        if (!valid()) return 0.0f;
        const f32 fx = (x + cell * 0.5f) / cell + static_cast<f32>(width) * 0.5f;
        const f32 fz = (z + cell * 0.5f) / cell + static_cast<f32>(depth) * 0.5f;
        const i32 ix = static_cast<i32>(std::floor(fx));
        const i32 iz = static_cast<i32>(std::floor(fz));
        const i32 iw = static_cast<i32>(width);
        const i32 id = static_cast<i32>(depth);
        if (ix < 0 || iz < 0 || ix >= iw || iz >= id) return 0.0f;
        const f32 tx = fx - static_cast<f32>(ix);
        const f32 tz = fz - static_cast<f32>(iz);
        const usize gw = grid_w();
        const auto h = [&](i32 gx, i32 gz) {
            return heights[static_cast<usize>(gz) * gw + static_cast<usize>(gx)];
        };
        const f32 h00 = h(ix, iz), h10 = h(ix + 1, iz);
        const f32 h01 = h(ix, iz + 1), h11 = h(ix + 1, iz + 1);
        const f32 top = h00 + (h10 - h00) * tx;
        const f32 bot = h01 + (h11 - h01) * tx;
        return top + (bot - top) * tz;
    }
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

    // --- Terrain (optionales Höhenfeld) -------------------------------------
    [[nodiscard]] const TerrainData& terrain() const noexcept { return terrain_; }
    [[nodiscard]] TerrainData& terrain() noexcept { return terrain_; }
    void set_terrain(TerrainData t) {
        terrain_ = std::move(t);
        terrain_mesh_.reset();
    }
    [[nodiscard]] const std::shared_ptr<render::Mesh>& terrain_mesh() const noexcept {
        return terrain_mesh_;
    }
    void set_terrain_mesh(std::shared_ptr<render::Mesh> m) { terrain_mesh_ = std::move(m); }
    [[nodiscard]] const std::shared_ptr<res::TextureData>& terrain_texture() const noexcept {
        return terrain_texture_;
    }
    void set_terrain_texture(std::shared_ptr<res::TextureData> t) {
        terrain_texture_ = std::move(t);
    }

    /**
     * @brief Sammelt Renderables für den Renderer.
     */
    void collect_renderables(std::vector<render::Renderable>& out) const;

    /**
     * @brief Picking: nächstes Objekt entlang Strahl (nach t).
     * @return kInvalidEntity wenn nichts
     */
    [[nodiscard]] EntityId pick_ray(const render::Vec3& origin,
                                    const render::Vec3& dir) const;

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
    TerrainData terrain_;
    std::shared_ptr<render::Mesh> terrain_mesh_;
    std::shared_ptr<res::TextureData> terrain_texture_;
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
