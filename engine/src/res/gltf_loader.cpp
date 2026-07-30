/**
 * @file gltf_loader.cpp
 */
#include <aether/res/gltf_loader.hpp>
#include <aether/core/logger.hpp>
#include <aether/render/math.hpp>

#include <cgltf.h>

#include <vector>

namespace aether::res {

std::shared_ptr<render::Mesh> load_gltf_mesh(const std::filesystem::path& path) {
    cgltf_options options{};
    cgltf_data* data = nullptr;
    const std::string p = path.string();
    cgltf_result res = cgltf_parse_file(&options, p.c_str(), &data);
    if (res != cgltf_result_success) {
        core::log_warn("glTF", "parse failed: " + p);
        return nullptr;
    }
    res = cgltf_load_buffers(&options, data, p.c_str());
    if (res != cgltf_result_success) {
        core::log_warn("glTF", "buffers failed: " + p);
        cgltf_free(data);
        return nullptr;
    }

    render::MeshLod lod;
    lod.max_distance = 1.0e9f;

    for (cgltf_size mi = 0; mi < data->meshes_count; ++mi) {
        const cgltf_mesh& mesh = data->meshes[mi];
        for (cgltf_size pi = 0; pi < mesh.primitives_count; ++pi) {
            const cgltf_primitive& prim = mesh.primitives[pi];
            if (prim.type != cgltf_primitive_type_triangles) {
                continue;
            }
            const cgltf_accessor* pos = nullptr;
            const cgltf_accessor* nrm = nullptr;
            const cgltf_accessor* uvs = nullptr;
            for (cgltf_size ai = 0; ai < prim.attributes_count; ++ai) {
                const cgltf_attribute& a = prim.attributes[ai];
                if (a.type == cgltf_attribute_type_position) pos = a.data;
                if (a.type == cgltf_attribute_type_normal) nrm = a.data;
                if (a.type == cgltf_attribute_type_texcoord && a.index == 0) uvs = a.data;
            }
            if (!pos) {
                continue;
            }
            const u32 base = static_cast<u32>(lod.vertices.size());
            for (cgltf_size vi = 0; vi < pos->count; ++vi) {
                render::Vertex v;
                float tmp[3] = {0, 0, 0};
                cgltf_accessor_read_float(pos, vi, tmp, 3);
                v.position = {tmp[0], tmp[1], tmp[2]};
                if (nrm) {
                    cgltf_accessor_read_float(nrm, vi, tmp, 3);
                    v.normal = {tmp[0], tmp[1], tmp[2]};
                } else {
                    v.normal = {0, 1, 0};
                }
                if (uvs) {
                    float t[2] = {0, 0};
                    cgltf_accessor_read_float(uvs, vi, t, 2);
                    v.uv = {t[0], t[1]};
                }
                v.color = {1, 1, 1, 1};
                lod.vertices.push_back(v);
            }
            if (prim.indices) {
                for (cgltf_size ii = 0; ii < prim.indices->count; ++ii) {
                    const cgltf_size idx = cgltf_accessor_read_index(prim.indices, ii);
                    lod.indices.push_back(base + static_cast<u32>(idx));
                }
            } else {
                for (u32 i = 0; i < static_cast<u32>(pos->count); ++i) {
                    lod.indices.push_back(base + i);
                }
            }
        }
        if (!lod.vertices.empty()) {
            break; // first mesh enough for maker props
        }
    }

    cgltf_free(data);

    if (lod.vertices.empty() || lod.indices.empty()) {
        core::log_warn("glTF", "no geometry: " + p);
        return nullptr;
    }

    // flat normals if missing
    bool any_n = false;
    for (const auto& v : lod.vertices) {
        if (glm::length(v.normal) > 0.1f) {
            any_n = true;
            break;
        }
    }
    if (!any_n) {
        for (usize i = 0; i + 2 < lod.indices.size(); i += 3) {
            auto& v0 = lod.vertices[lod.indices[i]];
            auto& v1 = lod.vertices[lod.indices[i + 1]];
            auto& v2 = lod.vertices[lod.indices[i + 2]];
            const render::Vec3 n =
                glm::normalize(glm::cross(v1.position - v0.position, v2.position - v0.position));
            v0.normal = v1.normal = v2.normal = n;
        }
    }

    auto out = std::make_shared<render::Mesh>(path.filename().string());
    out->set_lod(0, std::move(lod));
    core::log_info("glTF", "Loaded " + p + " verts=" + std::to_string(out->lod(0).vertices.size()));
    return out;
}

} // namespace aether::res
