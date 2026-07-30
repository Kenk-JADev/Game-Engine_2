/**
 * @file mesh.cpp
 */
#include <aether/render/mesh.hpp>

#include <aether/core/assert.hpp>

namespace aether::render {

void MeshLod::recompute_bounds() {
    if (vertices.empty()) {
        local_bounds = {};
        return;
    }
    local_bounds.min = Vec3(std::numeric_limits<f32>::max());
    local_bounds.max = Vec3(std::numeric_limits<f32>::lowest());
    for (const auto& v : vertices) {
        local_bounds.expand(v.position);
    }
}

Mesh::Mesh(std::string name) : name_(std::move(name)) {}

void Mesh::set_lod(usize level, MeshLod lod) {
    if (level >= kMaxLods) {
        return;
    }
    if (lods_.size() <= level) {
        lods_.resize(level + 1);
    }
    lod.recompute_bounds();
    lods_[level] = std::move(lod);
    recompute_combined_bounds();
    gpu_ready_ = false;
    gpu_handle_ = 0;
}

const MeshLod& Mesh::lod(usize level) const {
    AETHER_ASSERT(level < lods_.size());
    return lods_[level];
}

MeshLod& Mesh::lod(usize level) {
    AETHER_ASSERT(level < lods_.size());
    return lods_[level];
}

usize Mesh::select_lod(f32 distance) const noexcept {
    if (lods_.empty()) {
        return 0;
    }
    for (usize i = 0; i < lods_.size(); ++i) {
        if (distance <= lods_[i].max_distance) {
            return i;
        }
    }
    return lods_.size() - 1;
}

void Mesh::recompute_combined_bounds() {
    if (lods_.empty()) {
        bounds_ = {};
        return;
    }
    bounds_ = lods_[0].local_bounds;
    for (usize i = 1; i < lods_.size(); ++i) {
        bounds_.merge(lods_[i].local_bounds);
    }
}

std::shared_ptr<Mesh> Mesh::create_cube(f32 size) {
    const f32 h = size * 0.5f;
    MeshLod lod0;
    lod0.max_distance = 1.0e9f;

    // 6 faces * 4 verts, indexed
    struct Face {
        Vec3 n;
        Vec3 p[4];
    };
    const Face faces[6] = {
        {{0, 0, 1},  {{-h, -h, h}, {h, -h, h}, {h, h, h}, {-h, h, h}}},
        {{0, 0, -1}, {{h, -h, -h}, {-h, -h, -h}, {-h, h, -h}, {h, h, -h}}},
        {{0, 1, 0},  {{-h, h, h}, {h, h, h}, {h, h, -h}, {-h, h, -h}}},
        {{0, -1, 0}, {{-h, -h, -h}, {h, -h, -h}, {h, -h, h}, {-h, -h, h}}},
        {{1, 0, 0},  {{h, -h, h}, {h, -h, -h}, {h, h, -h}, {h, h, h}}},
        {{-1, 0, 0}, {{-h, -h, -h}, {-h, -h, h}, {-h, h, h}, {-h, h, -h}}},
    };
    const Vec2 uvs[4] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};

    lod0.vertices.reserve(24);
    lod0.indices.reserve(36);
    for (const auto& f : faces) {
        const u32 base = static_cast<u32>(lod0.vertices.size());
        for (int i = 0; i < 4; ++i) {
            Vertex v;
            v.position = f.p[i];
            v.normal = f.n;
            v.uv = uvs[i];
            v.color = Vec4(1.0f);
            lod0.vertices.push_back(v);
        }
        lod0.indices.push_back(base + 0);
        lod0.indices.push_back(base + 1);
        lod0.indices.push_back(base + 2);
        lod0.indices.push_back(base + 0);
        lod0.indices.push_back(base + 2);
        lod0.indices.push_back(base + 3);
    }

    auto mesh = std::make_shared<Mesh>("cube");
    mesh->set_lod(0, std::move(lod0));
    return mesh;
}

std::shared_ptr<Mesh> Mesh::create_plane(f32 size) {
    const f32 h = size * 0.5f;
    MeshLod lod0;
    lod0.max_distance = 1.0e9f;
    lod0.vertices = {
        {{-h, 0, -h}, {0, 1, 0}, {0, 0}, {1, 1, 1, 1}},
        {{ h, 0, -h}, {0, 1, 0}, {1, 0}, {1, 1, 1, 1}},
        {{ h, 0,  h}, {0, 1, 0}, {1, 1}, {1, 1, 1, 1}},
        {{-h, 0,  h}, {0, 1, 0}, {0, 1}, {1, 1, 1, 1}},
    };
    lod0.indices = {0, 1, 2, 0, 2, 3};

    auto mesh = std::make_shared<Mesh>("plane");
    mesh->set_lod(0, std::move(lod0));
    return mesh;
}

} // namespace aether::render
