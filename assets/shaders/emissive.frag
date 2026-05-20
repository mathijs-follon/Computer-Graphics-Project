#version 460 core

in vec3 v_worldNormal;
in vec2 v_uv;
in vec3 v_frag_pos;

uniform vec3 u_color;
uniform float u_alpha;

out vec4 fragColor;

void main() {
    fragColor = vec4(u_color, u_alpha);
}
