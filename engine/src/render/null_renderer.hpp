/**
 * @file null_renderer.hpp
 * @brief Headless-Renderer: Culling/LOD/Stats ohne GPU.
 */
#pragma once

#include <aether/render/renderer.hpp>

#include <unordered_set>

namespace aether::res {
struct TextureData;
}

namespace aether::render {

class NullRenderer final : public Renderer {
public:
    explicit NullRenderer(RendererDesc desc);

    void set_viewport(i32 x, i32 y, i32 width, i32 height) override;
    void begin_frame() override;
    void end_frame() override;
    void upload_mesh(Mesh& mesh) override;
    void upload_texture(const res::TextureData& tex) override;
    Result<void> compile_shader(ShaderProgram& program) override;

protected:
    void backend_draw_items(const Camera& camera,
                            const std::vector<DrawItem>& items) override;

private:
    i32 vp_w_ = 0;
    i32 vp_h_ = 0;
    std::unordered_set<const res::TextureData*> uploaded_; ///< Dedup wie GL-Cache
};

} // namespace aether::render
