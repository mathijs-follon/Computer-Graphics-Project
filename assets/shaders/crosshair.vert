#version 460 core

layout (location = 0) in vec2 a_pos;

uniform vec2 u_halfSizeNdc;

out vec2 v_local;

void main() {
    v_local = a_pos;
    gl_Position = vec4(a_pos * u_halfSizeNdc, 0.0, 1.0);
}
