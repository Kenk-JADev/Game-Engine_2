/**
 * @file map_loader.cpp
 */
#include <aether/game/map_loader.hpp>
#include <aether/core/logger.hpp>
#include <aether/render/mesh.hpp>

#include <fstream>
#include <iomanip>
#include <sstream>

namespace aether::game {
namespace fs = std::filesystem;

namespace {

void assign_default_meshes(scene::Scene& sc, render::Renderer* renderer) {
    for (auto& o : sc.objects()) {
        if (o.mesh) {
            if (renderer) renderer->upload_mesh(*o.mesh);
            continue;
        }
        if (o.type == scene::ObjectType::Prop &&
            (o.name.find("Boden") != std::string::npos ||
             o.name.find("Ground") != std::string::npos ||
             o.name.find("Floor") != std::string::npos)) {
            o.mesh = render::Mesh::create_plane(40.0f);
            o.material.albedo = render::Color{0.35f, 0.55f, 0.30f, 1.0f};
        } else if (o.type == scene::ObjectType::Character) {
            o.mesh = render::Mesh::create_cube(1.0f);
            o.transform.scale = {0.6f, 1.2f, 0.6f};
            o.material.albedo = render::Color{0.2f, 0.55f, 1.0f, 1.0f};
        } else if (o.type == scene::ObjectType::Npc) {
            o.mesh = render::Mesh::create_cube(1.0f);
            o.transform.scale = {0.6f, 1.2f, 0.6f};
            o.material.albedo = render::Color{0.3f, 0.85f, 0.45f, 1.0f};
        } else if (o.type == scene::ObjectType::Enemy) {
            o.mesh = render::Mesh::create_cube(1.0f);
            o.transform.scale = {0.8f, 0.8f, 0.8f};
            o.material.albedo = render::Color{1.0f, 0.35f, 0.3f, 1.0f};
        } else if (o.type == scene::ObjectType::Event) {
            o.mesh = render::Mesh::create_cube(1.0f);
            o.transform.scale = {0.5f, 0.5f, 0.5f};
            o.material.albedo = render::Color{1.0f, 0.9f, 0.2f, 1.0f};
        } else {
            o.mesh = render::Mesh::create_cube(1.0f);
        }
        if (renderer && o.mesh) {
            renderer->upload_mesh(*o.mesh);
        }
    }
    sc.rebuild_collision();
}

} // namespace

std::filesystem::path map_path_for_id(const fs::path& maps_dir, u32 map_id) {
    std::ostringstream name;
    name << "map" << std::setw(3) << std::setfill('0') << map_id << ".json";
    return maps_dir / name.str();
}

std::unique_ptr<scene::Scene> create_default_map(const std::string& name,
                                                 render::Renderer* renderer) {
    auto sc = std::make_unique<scene::Scene>(name);

    render::Transform ground_t;
    auto ground = render::Mesh::create_plane(40.0f);
    if (renderer) renderer->upload_mesh(*ground);
    const auto gid =
        sc->place(scene::ObjectType::Prop, "Boden", ground, ground_t);
    if (auto* g = sc->find(gid)) {
        g->material.albedo = render::Color{0.35f, 0.55f, 0.30f, 1.0f};
    }

    // Player
    render::Transform pt;
    pt.position = {0.0f, 0.0f, 0.0f};
    pt.scale = {0.6f, 1.2f, 0.6f};
    auto pmesh = render::Mesh::create_cube(1.0f);
    if (renderer) renderer->upload_mesh(*pmesh);
    const auto pid =
        sc->place(scene::ObjectType::Character, "Player", pmesh, pt);
    if (auto* p = sc->find(pid)) {
        p->material.albedo = render::Color{0.2f, 0.55f, 1.0f, 1.0f};
    }

    // NPC
    render::Transform nt;
    nt.position = {3.0f, 0.0f, -2.0f};
    nt.scale = {0.6f, 1.2f, 0.6f};
    auto nmesh = render::Mesh::create_cube(1.0f);
    if (renderer) renderer->upload_mesh(*nmesh);
    const auto nid = sc->place(scene::ObjectType::Npc, "Elder", nmesh, nt);
    if (auto* n = sc->find(nid)) {
        n->material.albedo = render::Color{0.3f, 0.85f, 0.45f, 1.0f};
        // Attach a simple talk event
        game::MapEvent ev;
        ev.name = "Elder";
        ev.x = nt.position.x;
        ev.y = nt.position.y;
        ev.z = nt.position.z;
        game::EventPage page;
        page.name = "Talk";
        page.trigger = game::EventTrigger::ActionButton;
        page.commands.push_back(
            {game::EventCommandType::Message,
             {{"text", "Willkommen, Held! Erkunde die Welt."}},
             {}});
        page.commands.push_back(
            {game::EventCommandType::SetSwitch, {{"id", 1}, {"value", true}}, {}});
        ev.pages.push_back(std::move(page));
        n->map_event = std::move(ev);
        // Event bodies should not block – convert collision to trigger-ish by rebuilding?
        // Keep as kinematic blocker lightly – for talk we use interaction range.
    }

    // Prop rock
    render::Transform rt;
    rt.position = {-4.0f, 0.5f, 3.0f};
    auto rmesh = render::Mesh::create_cube(1.0f);
    if (renderer) renderer->upload_mesh(*rmesh);
    const auto rid = sc->place(scene::ObjectType::Prop, "Felsen", rmesh, rt);
    if (auto* r = sc->find(rid)) {
        r->material.albedo = render::Color{0.45f, 0.42f, 0.40f, 1.0f};
    }

    // Event marker
    render::Transform et;
    et.position = {0.0f, 0.0f, -5.0f};
    et.scale = {0.5f, 0.5f, 0.5f};
    auto emesh = render::Mesh::create_cube(1.0f);
    if (renderer) renderer->upload_mesh(*emesh);
    sc->place(scene::ObjectType::Event, "Sign", emesh, et);

    sc->bake_navigation(10.0f, 1.0f);
    return sc;
}

MapLoadResult load_map(const fs::path& map_file, res::ResourceManager* /*resources*/,
                       render::Renderer* renderer) {
    MapLoadResult out;
    std::error_code ec;
    if (!fs::exists(map_file, ec)) {
        out.scene = create_default_map("Map", renderer);
        out.ok = true;
        out.message = "Default map created (file missing)";
        core::log_info("Map", out.message + ": " + map_file.string());
        return out;
    }
    try {
        std::ifstream in(map_file);
        nlohmann::json j;
        in >> j;
        out.scene = scene::Scene::create_from_json(j);
        assign_default_meshes(*out.scene, renderer);
        out.scene->bake_navigation(10.0f, 1.0f);
        out.ok = true;
        out.message = "Loaded " + map_file.string();
        core::log_info("Map", out.message);
        return out;
    } catch (const std::exception& ex) {
        out.scene = create_default_map("Map", renderer);
        out.ok = true;
        out.message = std::string("Map parse failed, default used: ") + ex.what();
        core::log_warn("Map", out.message);
        return out;
    }
}

Result<void> save_map(const fs::path& map_file, const scene::Scene& scene) {
    try {
        if (map_file.has_parent_path()) {
            std::error_code ec;
            fs::create_directories(map_file.parent_path(), ec);
        }
        std::ofstream out(map_file);
        if (!out) {
            return Result<void>::fail("Cannot write " + map_file.string());
        }
        out << scene.to_json().dump(2) << '\n';
        return Result<void>::ok();
    } catch (const std::exception& ex) {
        return Result<void>::fail(ex.what());
    }
}

} // namespace aether::game
