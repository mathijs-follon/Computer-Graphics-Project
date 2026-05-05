#version 460 core

layout (location = 0) in vec3 a_pos;
layout (location = 1) in vec3 a_normal;
layout (location = 2) in vec2 a_uv;

uniform mat4 u_mvp;
uniform mat4 u_model;

out vec3 v_worldNormal;
out vec2 v_uv;

void main() {
    mat3 normalMatrix = mat3(transpose(inverse(u_model)));
    v_worldNormal = normalize(normalMatrix * a_normal);
    v_uv = a_uv;
    gl_Position = u_mvp * vec4(a_pos, 1.0);
}

