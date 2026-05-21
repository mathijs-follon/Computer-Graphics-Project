#version 460 core

in vec2 v_uv;

uniform sampler2D u_image;

out vec4 fragColor;

void main() {
    vec3 result = texture(u_image, v_uv).rgb ;
    fragColor = vec4(result, 1.0);
}
