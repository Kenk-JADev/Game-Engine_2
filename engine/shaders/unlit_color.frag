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
