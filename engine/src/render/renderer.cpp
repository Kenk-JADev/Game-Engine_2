/**
 * @file renderer.cpp
 * @brief Gemeinsame Culling/LOD-Logik + Factory.
 */
#include <aether/render/renderer.hpp>

#include "null_renderer.hpp"

#if defined(AETHER_WITH_OPENGL)
#  include "gl_renderer.hpp"
#endif

#include <aether/core/logger.hpp>
#include <aether/window/window.hpp>

#include <algorithm>

namespace aether::render {

Renderer::Renderer(RendererBackend backend, RendererDesc desc)
    : backend_(backend)
    , desc_(std::move(desc))
    , clear_color_(desc_.clear_color)
    , cull_enabled_(desc_.enable_frustum_culling)
    , lod_enabled_(desc_.enable_lod) {}

Renderer::~Renderer() = default;

std::unique_ptr<Renderer> Renderer::create(const RendererDesc& desc,
                                           window::Window* window) {
    RendererDesc d = desc;

    // Auto-select OpenGL when window has a real GL context
    if (d.backend == RendererBackend::Null && window &&
        window->backend() == window::WindowBackend::Glfw) {
#if defined(AETHER_WITH_OPENGL)
        d.backend = RendererBackend::OpenGL;
#endif
    }

    if (d.backend == RendererBackend::OpenGL) {
#if defined(AETHER_WITH_OPENGL)
        if (window && window->backend() == window::WindowBackend::Glfw) {
            auto gl = std::make_unique<GlRenderer>(d, window);
            return gl;
        }
        core::log_warn("Renderer", "OpenGL requested but no GLFW window – NullRenderer");
        d.backend = RendererBackend::Null;
#else
        core::log_warn("Renderer", "AETHER_WITH_OPENGL not enabled – NullRenderer");
        d.backend = RendererBackend::Null;
#endif
    }

    return std::make_unique<NullRenderer>(std::move(d));
}

void Renderer::build_draw_list(const Camera& camera,
                               const std::vector<Renderable>& items,
                               std::vector<DrawItem>& out) {
    out.clear();
    out.reserve(items.size());

    const Frustum frustum = camera.frustum();
    stats_.submitted = static_cast<u32>(items.size());

    for (const auto& r : items) {
        if (!r.visible || !r.mesh || r.mesh->lod_count() == 0) {
            ++stats_.culled;
            continue;
        }

        const Mat4 model = r.transform.matrix();
        const AABB world_bounds = r.mesh->bounds().transformed(model);
        const BoundingSphere sphere = BoundingSphere::from_aabb(world_bounds);

        if (cull_enabled_ && !frustum.intersects_sphere(sphere)) {
            ++stats_.culled;
            continue;
        }

        const f32 dist = camera.distance_to(sphere.center);
        const usize lod = lod_enabled_ ? r.mesh->select_lod(dist) : 0;

        DrawItem item;
        item.source = &r;
        item.lod_level = lod;
        item.distance = dist;
        item.model = model;
        item.world_bounds = world_bounds;
        item.texture = r.texture;
        item.transparent = r.material.transparent;
        out.push_back(item);
    }

    std::stable_sort(out.begin(), out.end(), [](const DrawItem& a, const DrawItem& b) {
        if (a.transparent != b.transparent) {
            return !a.transparent && b.transparent;
        }
        if (!a.transparent) {
            return a.distance < b.distance;
        }
        return a.distance > b.distance;
    });
}

void Renderer::draw(const Camera& camera, const std::vector<Renderable>& items) {
    build_draw_list(camera, items, draw_list_);
    backend_draw_items(camera, draw_list_);
}

} // namespace aether::render
