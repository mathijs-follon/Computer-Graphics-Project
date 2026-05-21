#version 460 core

in vec2 v_uv;

uniform sampler2D u_image;
uniform vec2 u_texelSize;


const float kKernel[25] =
    float[](0,0,-1,0,0,0,-1,-2,-1,0,-1,-2,16,-2,-1,0,-1,-2,-1,0,0,0,-1,0,0);


out vec4 fragColor;

void main() {
    vec3 result = vec3(0.0);
    for (int i = 0; i < 5; ++i) {
        for (int j = 0; j < 5; ++j) {
            vec2 offset = vec2(j-2,i-2);
            result += texture(u_image, v_uv + offset * u_texelSize).rgb * kKernel[i*5+j];
        }
    }

    fragColor = vec4(result, 1.0);
}
