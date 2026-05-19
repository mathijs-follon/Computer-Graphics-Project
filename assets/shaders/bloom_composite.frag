#version 460 core

in vec2 v_uv;

uniform sampler2D u_scene;
uniform sampler2D u_bloom;
uniform float u_intensity;

out vec4 fragColor;

void main() {
    vec3 scene = texture(u_scene, v_uv).rgb;
    vec3 bloom = texture(u_bloom, v_uv).rgb;
    fragColor = vec4(scene + u_intensity * bloom, 1.0);
}
