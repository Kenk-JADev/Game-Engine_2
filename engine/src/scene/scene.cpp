/**
 * @file scene.cpp
 */
#include <aether/scene/scene.hpp>
#include <aether/core/logger.hpp>

#include <limits>

namespace aether::scene {

Scene::Scene(std::string name) : name_(std::move(name)) {}

phys::ObjectKind Scene::to_phys_kind(ObjectType t) const {
    switch (t) {
    case ObjectType::Character: return phys::ObjectKind::Player;
    case ObjectType::Npc: return phys::ObjectKind::Npc;
    case ObjectType::Enemy: return phys::ObjectKind::Enemy;
    case ObjectType::Event: return phys::ObjectKind::Event;
    case ObjectType::Prop:
    case ObjectType::Light:
    case ObjectType::Spawn:
    default: return phys::ObjectKind::Prop;
    }
}

EntityId Scene::add_object(SceneObject obj) {
    if (obj.id == kInvalidEntity) {
        obj.id = next_id_++;
    } else {
        next_id_ = std::max(next_id_, obj.id + 1);
    }
    objects_.push_back(std::move(obj));
    return objects_.back().id;
}

void Scene::remove_object(EntityId id) {
    for (auto it = objects_.begin(); it != objects_.end(); ++it) {
        if (it->id == id) {
            if (it->collision_id) {
                collision_.remove(it->collision_id);
            }
            objects_.erase(it);
            return;
        }
    }
}

SceneObject* Scene::find(EntityId id) {
    for (auto& o : objects_) {
        if (o.id == id) return &o;
    }
    return nullptr;
}

const SceneObject* Scene::find(EntityId id) const {
    for (const auto& o : objects_) {
        if (o.id == id) return &o;
    }
    return nullptr;
}

EntityId Scene::place(ObjectType type, std::string name,
                      std::shared_ptr<render::Mesh> mesh,
                      const render::Transform& transform) {
    SceneObject obj;
    obj.name = std::move(name);
    obj.type = type;
    obj.mesh = std::move(mesh);
    obj.transform = transform;

    if (obj.mesh) {
        auto body = phys::make_body_from_mesh_bounds(
            0, to_phys_kind(type), obj.mesh->bounds(), transform);
        if (type == ObjectType::Event) {
            body.block_movement = false;
            body.type = phys::BodyType::Trigger;
        }
        obj.collision_id = collision_.add(std::move(body));
    }

    // Auto event shell for Event type
    if (type == ObjectType::Event) {
        game::MapEvent ev;
        ev.name = obj.name;
        ev.x = transform.position.x;
        ev.y = transform.position.y;
        ev.z = transform.position.z;
        game::EventPage page;
        page.name = "Page 1";
        page.commands.push_back(
            {game::EventCommandType::Message, {{"text", "…"}}, {}});
        ev.pages.push_back(std::move(page));
        obj.map_event = std::move(ev);
    }

    core::log_info("Scene", "Placed '" + obj.name + "' (auto collision)");
    return add_object(std::move(obj));
}

void Scene::rebuild_collision() {
    collision_.clear();
    for (auto& o : objects_) {
        if (!o.mesh) {
            o.collision_id = 0;
            continue;
        }
        auto body = phys::make_body_from_mesh_bounds(
            0, to_phys_kind(o.type), o.mesh->bounds(), o.transform);
        if (o.type == ObjectType::Event) {
            body.block_movement = false;
            body.type = phys::BodyType::Trigger;
        }
        o.collision_id = collision_.add(std::move(body));
    }
    nav_ready_ = false;
}

void Scene::bake_navigation(f32 padding, f32 cell) {
    f32 min_x = std::numeric_limits<f32>::max();
    f32 min_z = std::numeric_limits<f32>::max();
    f32 max_x = std::numeric_limits<f32>::lowest();
    f32 max_z = std::numeric_limits<f32>::lowest();
    bool any = false;
    for (const auto& b : collision_.bodies()) {
        const auto wb = b.world_bounds();
        min_x = std::min(min_x, wb.min.x);
        min_z = std::min(min_z, wb.min.z);
        max_x = std::max(max_x, wb.max.x);
        max_z = std::max(max_z, wb.max.z);
        any = true;
    }
    if (!any) {
        min_x = min_z = -10;
        max_x = max_z = 10;
    }
    min_x -= padding;
    min_z -= padding;
    max_x += padding;
    max_z += padding;
    nav_grid_ = nav::bake_nav_grid(collision_, min_x, min_z, max_x, max_z, cell);
    nav_ready_ = true;
    core::log_info("Scene", "Nav grid " + std::to_string(nav_grid_.width) + "x" +
                                std::to_string(nav_grid_.height));
}

void Scene::collect_renderables(std::vector<render::Renderable>& out) const {
    out.clear();
    out.reserve(objects_.size());
    for (const auto& o : objects_) {
        if (!o.visible || !o.mesh) continue;
        render::Renderable r;
        r.mesh = o.mesh;
        r.material = o.material;
        r.transform = o.transform;
        r.visible = true;
        out.push_back(std::move(r));
    }
}

nlohmann::json Scene::to_json() const {
    nlohmann::json arr = nlohmann::json::array();
    for (const auto& o : objects_) {
        nlohmann::json jo{
            {"id", o.id},
            {"name", o.name},
            {"type", static_cast<int>(o.type)},
            {"position", {o.transform.position.x, o.transform.position.y,
                          o.transform.position.z}},
            {"scale", {o.transform.scale.x, o.transform.scale.y, o.transform.scale.z}},
            {"visible", o.visible},
            {"mesh", o.mesh ? o.mesh->name() : ""},
        };
        if (o.map_event) {
            nlohmann::json je;
            game::to_json(je, *o.map_event);
            jo["event"] = je;
        }
        arr.push_back(std::move(jo));
    }
    return nlohmann::json{{"name", name_}, {"objects", arr}};
}

void Scene::load_from_json(Scene& out, const nlohmann::json& j) {
    out.name_ = j.value("name", "Map");
    out.objects_.clear();
    out.collision_.clear();
    out.next_id_ = 1;
    out.nav_ready_ = false;
    if (!j.contains("objects")) return;
    for (const auto& jo : j["objects"]) {
        SceneObject o;
        o.id = jo.value("id", 0u);
        o.name = jo.value("name", "Object");
        o.type = static_cast<ObjectType>(jo.value("type", 0));
        if (jo.contains("position") && jo["position"].is_array() &&
            jo["position"].size() >= 3) {
            o.transform.position = {jo["position"][0].get<f32>(),
                                    jo["position"][1].get<f32>(),
                                    jo["position"][2].get<f32>()};
        }
        if (jo.contains("scale") && jo["scale"].is_array() && jo["scale"].size() >= 3) {
            o.transform.scale = {jo["scale"][0].get<f32>(), jo["scale"][1].get<f32>(),
                                 jo["scale"][2].get<f32>()};
        }
        o.visible = jo.value("visible", true);
        if (jo.contains("event")) {
            game::MapEvent ev;
            game::from_json(jo["event"], ev);
            o.map_event = std::move(ev);
        }
        out.add_object(std::move(o));
    }
}

std::unique_ptr<Scene> Scene::create_from_json(const nlohmann::json& j) {
    auto s = std::make_unique<Scene>(j.value("name", "Map"));
    load_from_json(*s, j);
    return s;
}

void SceneManager::set_active(std::unique_ptr<Scene> scene) {
    active_ = std::move(scene);
}

} // namespace aether::scene
