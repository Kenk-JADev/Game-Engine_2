/**
 * @file gl_renderer.hpp
 * @brief OpenGL 3.3 Forward-Renderer.
 */
#pragma once

#include <aether/render/renderer.hpp>

#include <unordered_map>

#if defined(AETHER_WITH_OPENGL)

namespace aether::render {

struct GlMeshGpu {
    u32 vao = 0;
    u32 vbo = 0;
    u32 ebo = 0;
    u32 index_count = 0;
    usize lod = 0;
};

class GlRenderer final : public Renderer {
public:
    explicit GlRenderer(RendererDesc desc, window::Window* window);
    ~GlRenderer() override;

    void set_viewport(i32 x, i32 y, i32 width, i32 height) override;
    void begin_frame() override;
    void end_frame() override;
    void upload_mesh(Mesh& mesh) override;
    Result<void> compile_shader(ShaderProgram& program) override;

protected:
    void backend_draw_items(const Camera& camera,
                            const std::vector<DrawItem>& items) override;

private:
    bool init_gl();
    void destroy_mesh_gpu(GlMeshGpu& m);
    u32 compile_stage(u32 type, const char* src, std::string& err);
    void ensure_default_shaders();
    void bind_program(u32 program);

    window::Window* window_ = nullptr;
    bool ready_ = false;
    i32 vp_x_ = 0, vp_y_ = 0, vp_w_ = 1, vp_h_ = 1;

    u32 prog_unlit_ = 0;
    u32 prog_lit_ = 0;
    i32 loc_model_ = -1;
    i32 loc_vp_ = -1;
    i32 loc_albedo_ = -1;
    i32 loc_normal_mat_ = -1;
    i32 loc_light_dir_ = -1;
    i32 loc_light_color_ = -1;
    i32 loc_ambient_ = -1;

    // mesh pointer identity → lod gpu
    std::unordered_map<const Mesh*, std::vector<GlMeshGpu>> gpu_meshes_;
};

} // namespace aether::render

#endif
