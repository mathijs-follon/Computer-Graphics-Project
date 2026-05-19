#version 460 core

in vec2 v_uv;

uniform sampler2D u_image;
uniform vec2 u_texelSize;
uniform int u_horizontal;

out vec4 fragColor;

const float kWeights[5] =
    float[](0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);

void main() {
    vec2 dir = (u_horizontal == 1) ? vec2(u_texelSize.x, 0.0) : vec2(0.0, u_texelSize.y);
    vec3 result = texture(u_image, v_uv).rgb * kWeights[0];
    for (int i = 1; i < 5; ++i) {
        vec2 offset = dir * float(i);
        result += texture(u_image, v_uv + offset).rgb * kWeights[i];
        result += texture(u_image, v_uv - offset).rgb * kWeights[i];
    }
    fragColor = vec4(result, 1.0);
}
