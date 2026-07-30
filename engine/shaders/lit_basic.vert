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
