#version 460 core

// Shared vertex shader for fullscreen post process passes.
// Vertex positions are already in([-1, 1]) emit UVs to sample the previous pass color texture.
layout (location = 0) in vec2 a_pos;
layout (location = 1) in vec2 a_uv;

out vec2 v_uv;

void main() {
    v_uv = a_uv;
    gl_Position = vec4(a_pos, 0.0, 1.0);
}
