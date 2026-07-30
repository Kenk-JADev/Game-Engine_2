/**
 * @file gl_renderer.cpp
 */
#include "gl_renderer.hpp"

#if defined(AETHER_WITH_OPENGL)

#include <aether/core/logger.hpp>

#include <glad/glad.h>

#include <glm/gtc/type_ptr.hpp>

namespace aether::render {
namespace {

void gl_debug_clear() {
    while (glGetError() != GL_NO_ERROR) {
    }
}

} // namespace

GlRenderer::GlRenderer(RendererDesc desc, window::Window* window)
    : Renderer(RendererBackend::OpenGL, std::move(desc))
    , window_(window) {
    ready_ = init_gl();
    if (ready_) {
        ensure_default_shaders();
        core::log_info("Renderer", "GlRenderer ready (OpenGL 3.3)");
    } else {
        core::log_error("Renderer", "GlRenderer failed to initialize GL");
    }
}

GlRenderer::~GlRenderer() {
    for (auto& [mesh, lods] : gpu_meshes_) {
        (void)mesh;
        for (auto& m : lods) destroy_mesh_gpu(m);
    }
    gpu_meshes_.clear();
    if (prog_unlit_) glDeleteProgram(prog_unlit_);
    if (prog_lit_) glDeleteProgram(prog_lit_);
}

bool GlRenderer::init_gl() {
    if (!window_ || !window_->native_handle()) {
        core::log_error("Renderer", "No native window for GL context");
        return false;
    }
    window_->make_context_current();

    // gladLoadGLLoader needs a proc address function – GLFW provides glfwGetProcAddress
#if defined(AETHER_WITH_GLFW)
    extern void* aether_glfw_get_proc_address(const char* name);
    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(aether_glfw_get_proc_address))) {
        // Fallback: try gladLoadGL if available
        if (!gladLoadGL()) {
            core::log_error("Renderer", "gladLoadGLLoader failed");
            return false;
        }
    }
#else
    if (!gladLoadGL()) {
        core::log_error("Renderer", "gladLoadGL failed");
        return false;
    }
#endif

    core::log_info("Renderer", std::string("GL Vendor: ") +
                                   reinterpret_cast<const char*>(glGetString(GL_VENDOR)));
    core::log_info("Renderer", std::string("GL Renderer: ") +
                                   reinterpret_cast<const char*>(glGetString(GL_RENDERER)));
    core::log_info("Renderer", std::string("GL Version: ") +
                                   reinterpret_cast<const char*>(glGetString(GL_VERSION)));

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    vp_w_ = window_->framebuffer_width() > 0 ? window_->framebuffer_width() : window_->width();
    vp_h_ = window_->framebuffer_height() > 0 ? window_->framebuffer_height() : window_->height();
    set_viewport(0, 0, vp_w_, vp_h_);
    return true;
}

void GlRenderer::set_viewport(i32 x, i32 y, i32 width, i32 height) {
    vp_x_ = x;
    vp_y_ = y;
    vp_w_ = std::max(1, width);
    vp_h_ = std::max(1, height);
    if (ready_) {
        glViewport(vp_x_, vp_y_, vp_w_, vp_h_);
    }
}

void GlRenderer::begin_frame() {
    stats_.reset();
    if (!ready_) return;
    glViewport(vp_x_, vp_y_, vp_w_, vp_h_);
    glClearColor(clear_color_.r, clear_color_.g, clear_color_.b, clear_color_.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void GlRenderer::end_frame() {
    if (ready_) {
        glFlush();
    }
}

void GlRenderer::destroy_mesh_gpu(GlMeshGpu& m) {
    if (m.ebo) glDeleteBuffers(1, &m.ebo);
    if (m.vbo) glDeleteBuffers(1, &m.vbo);
    if (m.vao) glDeleteVertexArrays(1, &m.vao);
    m = {};
}

void GlRenderer::upload_mesh(Mesh& mesh) {
    if (!ready_) {
        mesh.set_gpu_ready(false);
        return;
    }
    auto& lods = gpu_meshes_[&mesh];
    for (auto& m : lods) destroy_mesh_gpu(m);
    lods.clear();
    lods.resize(mesh.lod_count());

    for (usize li = 0; li < mesh.lod_count(); ++li) {
        const auto& lod = mesh.lod(li);
        GlMeshGpu gpu;
        gpu.lod = li;
        gpu.index_count = static_cast<u32>(lod.indices.size());

        glGenVertexArrays(1, &gpu.vao);
        glGenBuffers(1, &gpu.vbo);
        glGenBuffers(1, &gpu.ebo);
        glBindVertexArray(gpu.vao);

        glBindBuffer(GL_ARRAY_BUFFER, gpu.vbo);
        glBufferData(GL_ARRAY_BUFFER,
                     static_cast<GLsizeiptr>(lod.vertices.size() * sizeof(Vertex)),
                     lod.vertices.data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gpu.ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                     static_cast<GLsizeiptr>(lod.indices.size() * sizeof(u32)),
                     lod.indices.data(), GL_STATIC_DRAW);

        const GLsizei stride = static_cast<GLsizei>(sizeof(Vertex));
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride,
                              reinterpret_cast<void*>(offsetof(Vertex, position)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride,
                              reinterpret_cast<void*>(offsetof(Vertex, normal)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride,
                              reinterpret_cast<void*>(offsetof(Vertex, uv)));
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, stride,
                              reinterpret_cast<void*>(offsetof(Vertex, color)));

        glBindVertexArray(0);
        lods[li] = gpu;
    }
    mesh.set_gpu_ready(true);
    mesh.set_gpu_handle(lods.empty() ? 0 : lods[0].vao);
}

u32 GlRenderer::compile_stage(u32 type, const char* src, std::string& err) {
    const u32 sh = glCreateShader(type);
    glShaderSource(sh, 1, &src, nullptr);
    glCompileShader(sh);
    i32 ok = 0;
    glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetShaderInfoLog(sh, 1024, nullptr, log);
        err = log;
        glDeleteShader(sh);
        return 0;
    }
    return sh;
}

Result<void> GlRenderer::compile_shader(ShaderProgram& program) {
    if (!ready_) return Result<void>::fail("GL not ready");
    std::string err;
    const u32 vs = compile_stage(GL_VERTEX_SHADER, program.source().vertex.c_str(), err);
    if (!vs) return Result<void>::fail("VS: " + err);
    const u32 fs = compile_stage(GL_FRAGMENT_SHADER, program.source().fragment.c_str(), err);
    if (!fs) {
        glDeleteShader(vs);
        return Result<void>::fail("FS: " + err);
    }
    const u32 prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    glDeleteShader(vs);
    glDeleteShader(fs);
    i32 ok = 0;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetProgramInfoLog(prog, 1024, nullptr, log);
        glDeleteProgram(prog);
        return Result<void>::fail(std::string("Link: ") + log);
    }
    program.set_gpu_id(prog);
    program.set_ready(true);
    shaders_.add(program);
    return Result<void>::ok();
}

void GlRenderer::ensure_default_shaders() {
    {
        ShaderProgram p("unlit_color");
        p.set_source(ShaderProgram::builtin_unlit_color());
        auto r = compile_shader(p);
        if (r) prog_unlit_ = p.gpu_id();
    }
    {
        ShaderProgram p("lit_basic");
        p.set_source(ShaderProgram::builtin_lit_basic());
        auto r = compile_shader(p);
        if (r) {
            prog_lit_ = p.gpu_id();
            loc_model_ = glGetUniformLocation(prog_lit_, "u_model");
            loc_vp_ = glGetUniformLocation(prog_lit_, "u_view_proj");
            loc_albedo_ = glGetUniformLocation(prog_lit_, "u_albedo");
            loc_normal_mat_ = glGetUniformLocation(prog_lit_, "u_normal_mat");
            loc_light_dir_ = glGetUniformLocation(prog_lit_, "u_light_dir");
            loc_light_color_ = glGetUniformLocation(prog_lit_, "u_light_color");
            loc_ambient_ = glGetUniformLocation(prog_lit_, "u_ambient");
        }
    }
}

void GlRenderer::bind_program(u32 program) {
    glUseProgram(program);
}

void GlRenderer::backend_draw_items(const Camera& camera,
                                    const std::vector<DrawItem>& items) {
    if (!ready_) return;

    const Mat4 vp = camera.view_projection_matrix();
    const u32 prog = prog_lit_ ? prog_lit_ : prog_unlit_;
    if (!prog) return;
    bind_program(prog);

    if (prog == prog_lit_) {
        if (loc_vp_ >= 0) glUniformMatrix4fv(loc_vp_, 1, GL_FALSE, glm::value_ptr(vp));
        if (loc_light_dir_ >= 0) {
            const float d[3] = {-0.4f, -1.0f, -0.3f};
            glUniform3fv(loc_light_dir_, 1, d);
        }
        if (loc_light_color_ >= 0) {
            const float c[3] = {1.0f, 0.98f, 0.9f};
            glUniform3fv(loc_light_color_, 1, c);
        }
        if (loc_ambient_ >= 0) {
            const float a[3] = {0.35f, 0.38f, 0.42f};
            glUniform3fv(loc_ambient_, 1, a);
        }
    } else {
        const i32 u_vp = glGetUniformLocation(prog, "u_view_proj");
        if (u_vp >= 0) glUniformMatrix4fv(u_vp, 1, GL_FALSE, glm::value_ptr(vp));
    }

    for (const auto& item : items) {
        if (!item.source || !item.source->mesh) continue;
        auto it = gpu_meshes_.find(item.source->mesh.get());
        if (it == gpu_meshes_.end()) {
            upload_mesh(*item.source->mesh);
            it = gpu_meshes_.find(item.source->mesh.get());
            if (it == gpu_meshes_.end()) continue;
        }
        const auto& lods = it->second;
        if (item.lod_level >= lods.size()) continue;
        const GlMeshGpu& gpu = lods[item.lod_level];
        if (!gpu.vao || gpu.index_count == 0) continue;

        if (prog == prog_lit_) {
            if (loc_model_ >= 0)
                glUniformMatrix4fv(loc_model_, 1, GL_FALSE, glm::value_ptr(item.model));
            if (loc_normal_mat_ >= 0) {
                const Mat3 nm = glm::transpose(glm::inverse(Mat3(item.model)));
                glUniformMatrix3fv(loc_normal_mat_, 1, GL_FALSE, glm::value_ptr(nm));
            }
            if (loc_albedo_ >= 0) {
                const auto& c = item.source->material.albedo;
                const float a[4] = {c.r, c.g, c.b, c.a};
                glUniform4fv(loc_albedo_, 1, a);
            }
        } else {
            const i32 u_m = glGetUniformLocation(prog, "u_model");
            const i32 u_a = glGetUniformLocation(prog, "u_albedo");
            if (u_m >= 0) glUniformMatrix4fv(u_m, 1, GL_FALSE, glm::value_ptr(item.model));
            if (u_a >= 0) {
                const auto& c = item.source->material.albedo;
                const float a[4] = {c.r, c.g, c.b, c.a};
                glUniform4fv(u_a, 1, a);
            }
        }

        if (item.transparent) {
            glDepthMask(GL_FALSE);
        } else {
            glDepthMask(GL_TRUE);
        }

        glBindVertexArray(gpu.vao);
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(gpu.index_count), GL_UNSIGNED_INT,
                       nullptr);
        glBindVertexArray(0);

        stats_.drawn += 1;
        stats_.triangles += gpu.index_count / 3;
        if (item.lod_level < Mesh::kMaxLods) {
            stats_.lod_histogram[item.lod_level] += 1;
        }
    }
    glDepthMask(GL_TRUE);
    gl_debug_clear();
}

} // namespace aether::render

#endif // AETHER_WITH_OPENGL
