/**
 * @file shader.cpp
 */
#include <aether/render/shader.hpp>

namespace aether::render {

ShaderProgram::ShaderProgram(std::string name) : name_(std::move(name)) {}

ShaderSource ShaderProgram::builtin_unlit_color() {
    ShaderSource s;
    s.name = "unlit_color";
    s.vertex = R"GLSL(
#version 330 core
layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec2 a_uv;
layout(location = 3) in vec4 a_color;

uniform mat4 u_model;
uniform mat4 u_view_proj;

out vec4 v_color;
out vec2 v_uv;

void main() {
    v_color = a_color;
    v_uv = a_uv;
    gl_Position = u_view_proj * u_model * vec4(a_position, 1.0);
}
)GLSL";
    s.fragment = R"GLSL(
#version 330 core
in vec4 v_color;
in vec2 v_uv;

uniform vec4 u_albedo;
uniform sampler2D u_tex;
uniform int u_has_tex;

out vec4 frag_color;

void main() {
    vec4 base = v_color * u_albedo;
    if (u_has_tex == 1) {
        vec4 t = texture(u_tex, v_uv);
        base = vec4(t.rgb, base.a * t.a);
    }
    frag_color = base;
}
)GLSL";
    return s;
}

ShaderSource ShaderProgram::builtin_lit_basic() {
    ShaderSource s;
    s.name = "lit_basic";
    s.vertex = R"GLSL(
#version 330 core
layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec2 a_uv;
layout(location = 3) in vec4 a_color;

uniform mat4 u_model;
uniform mat4 u_view_proj;
uniform mat3 u_normal_mat;

out vec3 v_normal;
out vec4 v_color;
out vec2 v_uv;

void main() {
    v_normal = normalize(u_normal_mat * a_normal);
    v_color = a_color;
    v_uv = a_uv;
    gl_Position = u_view_proj * u_model * vec4(a_position, 1.0);
}
)GLSL";
    s.fragment = R"GLSL(
#version 330 core
in vec3 v_normal;
in vec4 v_color;
in vec2 v_uv;

uniform vec4 u_albedo;
uniform vec3 u_light_dir;
uniform vec3 u_light_color;
uniform vec3 u_ambient;
uniform sampler2D u_tex;
uniform int u_has_tex;

out vec4 frag_color;

void main() {
    vec3 n = normalize(v_normal);
    float ndl = max(dot(n, normalize(-u_light_dir)), 0.0);
    vec3 lighting = u_ambient + u_light_color * ndl;

    vec4 base = vec4(v_color.rgb * u_albedo.rgb, u_albedo.a * v_color.a);
    if (u_has_tex == 1) {
        vec4 t = texture(u_tex, v_uv);
        base = vec4(t.rgb, base.a * t.a);
    }
    frag_color = vec4(base.rgb * lighting, base.a);
}
)GLSL";
    return s;
}

ShaderProgram& ShaderLibrary::add(ShaderProgram program) {
    const std::string key = program.name();
    auto [it, inserted] = programs_.insert_or_assign(key, std::move(program));
    (void)inserted;
    return it->second;
}

ShaderProgram* ShaderLibrary::find(std::string_view name) {
    const auto it = programs_.find(std::string(name));
    return it == programs_.end() ? nullptr : &it->second;
}

const ShaderProgram* ShaderLibrary::find(std::string_view name) const {
    const auto it = programs_.find(std::string(name));
    return it == programs_.end() ? nullptr : &it->second;
}

void ShaderLibrary::clear() {
    programs_.clear();
}

} // namespace aether::render
