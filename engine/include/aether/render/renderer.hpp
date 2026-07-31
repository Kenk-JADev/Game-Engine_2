/**
 * @file renderer.hpp
 * @brief Forward-Renderer mit Frustum-Culling und LOD.
 *
 * Backends:
 *  - NullRenderer  (Headless / Tests – nur Culling & Stats)
 *  - GlRenderer    (OpenGL 3.3, wenn AETHER_WITH_OPENGL)
 */
#pragma once

#include <aether/core/types.hpp>
#include <aether/render/camera.hpp>
#include <aether/render/color.hpp>
#include <aether/render/renderable.hpp>
#include <aether/render/shader.hpp>
#include <aether/window/window.hpp>

#include <memory>
#include <vector>

namespace aether::render {

enum class RendererBackend {
    Null,
    OpenGL,
};

struct RendererDesc {
    RendererBackend backend = RendererBackend::Null;
    bool enable_frustum_culling = true;
    bool enable_lod = true;
    Color clear_color = Color::cornflower();
    i32  msaa_samples = 0; ///< 0 = aus (HD 4600-freundlich)
};

/**
 * @brief Öffentliche Renderer-API.
 */
class Renderer : public aether::NonMovable {
public:
    [[nodiscard]] static std::unique_ptr<Renderer> create(const RendererDesc& desc,
                                                          window::Window* window = nullptr);

    virtual ~Renderer();

    [[nodiscard]] RendererBackend backend() const noexcept { return backend_; }
    [[nodiscard]] const RenderStats& stats() const noexcept { return stats_; }
    [[nodiscard]] ShaderLibrary& shaders() noexcept { return shaders_; }

    void set_clear_color(const Color& c) noexcept { clear_color_ = c; }
    [[nodiscard]] const Color& clear_color() const noexcept { return clear_color_; }

    void set_frustum_culling(bool v) noexcept { cull_enabled_ = v; }
    void set_lod_enabled(bool v) noexcept { lod_enabled_ = v; }

    /**
     * @brief Viewport in Pixeln (Framebuffer-Größe).
     */
    virtual void set_viewport(i32 x, i32 y, i32 width, i32 height) = 0;

    /**
     * @brief Beginnt einen Frame (Clear).
     */
    virtual void begin_frame() = 0;

    /**
     * @brief Reicht Renderables ein, cullt, wählt LOD, zeichnet.
     */
    void draw(const Camera& camera, const std::vector<Renderable>& items);

    /**
     * @brief Beendet Frame (Flush). Swap übernimmt Window.
     */
    virtual void end_frame() = 0;

    /**
     * @brief GPU-Ressourcen für Mesh vorbereiten (No-Op im Null-Backend).
     */
    virtual void upload_mesh(Mesh& mesh) = 0;

    /**
     * @brief Textur auf die GPU übertragen (No-Op im Null-Backend).
     *        Wird beim ersten Zeichnen automatisch ausgeführt.
     */
    virtual void upload_texture(const res::TextureData& tex) = 0;

    /**
     * @brief Shader kompilieren/laden (Null: markiert ready ohne GPU).
     */
    virtual Result<void> compile_shader(ShaderProgram& program) = 0;

protected:
    explicit Renderer(RendererBackend backend, RendererDesc desc);

    virtual void backend_draw_items(const Camera& camera,
                                    const std::vector<DrawItem>& items) = 0;

    /**
     * @brief Baut Draw-Liste aus Submit (Culling + LOD).
     */
    void build_draw_list(const Camera& camera,
                         const std::vector<Renderable>& items,
                         std::vector<DrawItem>& out);

    RendererBackend backend_;
    RendererDesc desc_{};
    Color clear_color_{};
    bool cull_enabled_ = true;
    bool lod_enabled_ = true;
    RenderStats stats_{};
    ShaderLibrary shaders_{};
    std::vector<DrawItem> draw_list_;
};

} // namespace aether::render
