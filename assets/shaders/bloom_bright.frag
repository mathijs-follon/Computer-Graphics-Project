#version 460 core


in vec2 v_uv;

uniform sampler2D u_scene;
uniform float u_threshold;

out vec4 fragColor;

void main() {
    vec3 color = texture(u_scene, v_uv).rgb;
    float luma = dot(color, vec3(0.2126, 0.7152, 0.0722));
    float soft = smoothstep(u_threshold, u_threshold + 0.3, luma);
    fragColor = vec4(color * soft, 1.0);
}
