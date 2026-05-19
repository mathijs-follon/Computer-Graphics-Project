#version 460 core

in vec2 v_local;

uniform vec3 u_color;
uniform float u_alpha;
uniform float u_thickness;
uniform float u_gap;

out vec4 fragColor;

void main() {
    float ax = abs(v_local.x);
    float ay = abs(v_local.y);

    bool onHorizontalArm = (ay < u_thickness) && (ax > u_gap) && (ax < 1.0);
    bool onVerticalArm   = (ax < u_thickness) && (ay > u_gap) && (ay < 1.0);

    if (!onHorizontalArm && !onVerticalArm) {
        discard;
    }

    fragColor = vec4(u_color, u_alpha);
}
