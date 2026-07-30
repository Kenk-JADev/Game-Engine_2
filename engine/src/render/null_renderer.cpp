/**
 * @file null_renderer.cpp
 */
#include "null_renderer.hpp"

#include <aether/core/logger.hpp>

namespace aether::render {

NullRenderer::NullRenderer(RendererDesc desc)
    : Renderer(RendererBackend::Null, std::move(desc)) {
    // Built-ins als „ready“ registrieren (kein GPU)
    {
        ShaderProgram p("unlit_color");
        p.set_source(ShaderProgram::builtin_unlit_color());
        p.set_ready(true);
        shaders_.add(std::move(p));
    }
    {
        ShaderProgram p("lit_basic");
        p.set_source(ShaderProgram::builtin_lit_basic());
        p.set_ready(true);
        shaders_.add(std::move(p));
    }
    core::log_info("Renderer", "NullRenderer created (headless)");
}

void NullRenderer::set_viewport(i32 /*x*/, i32 /*y*/, i32 width, i32 height) {
    vp_w_ = width;
    vp_h_ = height;
}

void NullRenderer::begin_frame() {
    stats_.reset();
}

void NullRenderer::end_frame() {
    // nothing
}

void NullRenderer::upload_mesh(Mesh& mesh) {
    mesh.set_gpu_ready(true);
    mesh.set_gpu_handle(1); // dummy non-zero
}

Result<void> NullRenderer::compile_shader(ShaderProgram& program) {
    program.set_ready(true);
    program.set_gpu_id(1);
    shaders_.add(program);
    return Result<void>::ok();
}

void NullRenderer::backend_draw_items(const Camera& /*camera*/,
                                      const std::vector<DrawItem>& items) {
    for (const auto& item : items) {
        if (!item.source || !item.source->mesh) {
            continue;
        }
        const auto& lod = item.source->mesh->lod(item.lod_level);
        stats_.drawn += 1;
        stats_.triangles += static_cast<u32>(lod.indices.size() / 3);
        if (item.lod_level < Mesh::kMaxLods) {
            stats_.lod_histogram[item.lod_level] += 1;
        }
    }
}

} // namespace aether::render
