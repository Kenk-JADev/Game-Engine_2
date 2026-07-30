/**
 * @file undo_stack.cpp
 */
#include "undo_stack.hpp"

#include <aether/render/mesh.hpp>

namespace aether::editor {
namespace {

void assign_meshes(scene::Scene& sc, render::Renderer* renderer) {
    for (auto& o : sc.objects()) {
        if (o.mesh) {
            if (renderer) renderer->upload_mesh(*o.mesh);
            continue;
        }
        if (o.type == scene::ObjectType::Prop &&
            (o.name.find("Boden") != std::string::npos ||
             o.name.find("Ground") != std::string::npos)) {
            o.mesh = render::Mesh::create_plane(40.0f);
            o.material.albedo = render::Color{0.35f, 0.55f, 0.30f, 1.0f};
        } else {
            o.mesh = render::Mesh::create_cube(1.0f);
            if (o.type == scene::ObjectType::Character)
                o.material.albedo = render::Color{0.2f, 0.55f, 1.0f, 1};
            else if (o.type == scene::ObjectType::Npc)
                o.material.albedo = render::Color{0.3f, 0.85f, 0.45f, 1};
            else if (o.type == scene::ObjectType::Enemy)
                o.material.albedo = render::Color{1.0f, 0.35f, 0.3f, 1};
            else if (o.type == scene::ObjectType::Event)
                o.material.albedo = render::Color{1.0f, 0.9f, 0.2f, 1};
        }
        if (renderer && o.mesh) renderer->upload_mesh(*o.mesh);
    }
    sc.rebuild_collision();
}

} // namespace

void UndoStack::clear() {
    snaps_.clear();
    index_ = -1;
    last_label_.clear();
}

void UndoStack::push(const scene::Scene& scene, std::string label) {
    if (index_ + 1 < static_cast<int>(snaps_.size())) {
        snaps_.resize(static_cast<usize>(index_ + 1));
    }
    Snap s;
    s.json = scene.to_json();
    s.label = std::move(label);
    snaps_.push_back(std::move(s));
    index_ = static_cast<int>(snaps_.size()) - 1;
    if (snaps_.size() > 64) {
        snaps_.erase(snaps_.begin());
        --index_;
    }
    last_label_ = snaps_[static_cast<usize>(index_)].label;
}

std::unique_ptr<scene::Scene> UndoStack::restore(int idx, render::Renderer* renderer) {
    if (idx < 0 || idx >= static_cast<int>(snaps_.size())) {
        return nullptr;
    }
    auto sc = scene::Scene::create_from_json(snaps_[static_cast<usize>(idx)].json);
    if (!sc) {
        return nullptr;
    }
    assign_meshes(*sc, renderer);
    last_label_ = snaps_[static_cast<usize>(idx)].label;
    return sc;
}

std::unique_ptr<scene::Scene> UndoStack::undo(render::Renderer* renderer) {
    if (!can_undo()) {
        return nullptr;
    }
    --index_;
    return restore(index_, renderer);
}

std::unique_ptr<scene::Scene> UndoStack::redo(render::Renderer* renderer) {
    if (!can_redo()) {
        return nullptr;
    }
    ++index_;
    return restore(index_, renderer);
}

} // namespace aether::editor
