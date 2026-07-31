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

std::shared_ptr<Mesh> Mesh::create_quad(f32 width, f32 height) {
    const f32 w = width * 0.5f;
    MeshLod lod0;
    lod0.max_distance = 1.0e9f;
    // Vertikales Sprite: Fußpunkt y=0, nach +Z gerichtet; UV (0,0) = oben-links
    lod0.vertices = {
        {{-w, 0.0f, 0.0f}, {0, 0, 1}, {0, 1}, {1, 1, 1, 1}}, // unten-links
        {{ w, 0.0f, 0.0f}, {0, 0, 1}, {1, 1}, {1, 1, 1, 1}}, // unten-rechts
        {{ w, height, 0.0f}, {0, 0, 1}, {1, 0}, {1, 1, 1, 1}}, // oben-rechts
        {{-w, height, 0.0f}, {0, 0, 1}, {0, 0}, {1, 1, 1, 1}}, // oben-links
    };
    lod0.indices = {0, 1, 2, 0, 2, 3};

    auto mesh = std::make_shared<Mesh>("quad");
    mesh->set_lod(0, std::move(lod0));
    return mesh;
}

std::shared_ptr<Mesh> Mesh::create_terrain(i32 width, i32 depth, f32 cell,
                                           const std::vector<f32>& heights) {
    MeshLod lod0;
    lod0.max_distance = 1.0e9f;

    const i32 gw = width + 1;
    const i32 gd = depth + 1;
    if (width <= 0 || depth <= 0 || cell <= 0.0f ||
        heights.size() < static_cast<usize>(gw * gd)) {
        return create_plane(1.0f); // Fallback
    }
    const f32 half_w = static_cast<f32>(width) * cell * 0.5f;
    const f32 half_d = static_cast<f32>(depth) * cell * 0.5f;

    auto height_at = [&](i32 gx, i32 gz) {
        return heights[static_cast<usize>(gz) * static_cast<usize>(gw) +
                       static_cast<usize>(gx)];
    };

    lod0.vertices.reserve(static_cast<usize>(width * depth * 4));
    lod0.indices.reserve(static_cast<usize>(width * depth * 6));
    for (i32 z = 0; z < depth; ++z) {
        for (i32 x = 0; x < width; ++x) {
            const f32 px0 = -half_w + static_cast<f32>(x) * cell;
            const f32 px1 = px0 + cell;
            const f32 pz0 = -half_d + static_cast<f32>(z) * cell;
            const f32 pz1 = pz0 + cell;

            const f32 h00 = height_at(x, z);
            const f32 h10 = height_at(x + 1, z);
            const f32 h01 = height_at(x, z + 1);
            const f32 h11 = height_at(x + 1, z + 1);

            const u32 base = static_cast<u32>(lod0.vertices.size());
            lod0.vertices.push_back({{px0, h00, pz0}, {0, 1, 0}, {0, 0}, {1, 1, 1, 1}});
            lod0.vertices.push_back({{px1, h10, pz0}, {0, 1, 0}, {1, 0}, {1, 1, 1, 1}});
            lod0.vertices.push_back({{px1, h11, pz1}, {0, 1, 0}, {1, 1}, {1, 1, 1, 1}});
            lod0.vertices.push_back({{px0, h01, pz1}, {0, 1, 0}, {0, 1}, {1, 1, 1, 1}});
            lod0.indices.push_back(base);
            lod0.indices.push_back(base + 1);
            lod0.indices.push_back(base + 2);
            lod0.indices.push_back(base);
            lod0.indices.push_back(base + 2);
            lod0.indices.push_back(base + 3);
        }
    }

    // Flache Normalen je Dreieck (bessere Beleuchtung als (0,1,0)).
    // Windungsreihenfolge wie create_plane (CW von oben) → Normale invertieren,
    // damit sie nach oben (+Y) zeigt und das Lighting stimmt.
    for (usize t = 0; t + 2 < lod0.indices.size(); t += 3) {
        const auto& a = lod0.vertices[lod0.indices[t]].position;
        const auto& b = lod0.vertices[lod0.indices[t + 1]].position;
        const auto& c = lod0.vertices[lod0.indices[t + 2]].position;
        Vec3 n = glm::cross(c - a, b - a);
        const f32 len = glm::length(n);
        if (len > 1.0e-6f) n = n / len;
        lod0.vertices[lod0.indices[t]].normal = n;
        lod0.vertices[lod0.indices[t + 1]].normal = n;
        lod0.vertices[lod0.indices[t + 2]].normal = n;
    }

    lod0.recompute_bounds();
    auto mesh = std::make_shared<Mesh>("terrain");
    mesh->set_lod(0, std::move(lod0));
    return mesh;
}

} // namespace aether::render
