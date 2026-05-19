#version 460 core

in vec3 v_worldNormal;
in vec2 v_uv;

uniform sampler2D u_albedo;

out vec4 fragColor;

void main() {
    vec4 sampled = texture(u_albedo, v_uv);
    // Alpha-test cutout: model materials flagged BLEND in the gltf (foliage, kelp,
    // lattice grates) carry per-pixel cutout masks. Without this, the texture's
    // padding regions render as opaque black rectangles around leaves/grates.
    if (sampled.a < 0.5) {
        discard;
    }
    vec3 albedo = sampled.rgb;

    vec3 n = normalize(v_worldNormal);
    vec3 lightDir = normalize(vec3(0.35, 1.0, 0.2));
    float ndotl = max(dot(n, lightDir), 0.0);
    float ambient = 0.22;
    float diffuse = 0.78 * ndotl;

    vec3 shaded = albedo * (ambient + diffuse);
    fragColor = vec4(shaded, 1.0);
}
