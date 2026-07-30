#version 330 core
in vec4 v_color;
uniform vec4 u_albedo;
out vec4 frag_color;
void main() {
    frag_color = v_color * u_albedo;
}
