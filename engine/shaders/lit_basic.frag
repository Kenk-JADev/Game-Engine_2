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
