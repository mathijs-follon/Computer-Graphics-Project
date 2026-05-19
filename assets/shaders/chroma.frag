#version 460 core

in vec2 v_uv;

uniform sampler2D u_albedo;

// Chroma keying is performed in the YCbCr color space (per the opgave).
// u_enableKey toggles between "raw overlay" (false) and "chroma-keyed overlay" (true).
uniform bool u_enableKey;
// Key color (default = pure green) and tolerance window in the Cb/Cr chroma plane.
uniform vec3 u_keyColor;
uniform float u_keyThreshold;
uniform float u_keySoftness;

out vec4 fragColor;

// ITU-R BT.601 RGB -> YCbCr. We only need Cb and Cr for chroma keying,
// but we compute the full Y too in case we ever want a luma-aware version.
vec3 rgbToYCbCr(vec3 rgb) {
    float y  =  0.299 * rgb.r + 0.587 * rgb.g + 0.114 * rgb.b;
    float cb = -0.168736 * rgb.r - 0.331264 * rgb.g + 0.500 * rgb.b + 0.5;
    float cr =  0.500 * rgb.r - 0.418688 * rgb.g - 0.081312 * rgb.b + 0.5;
    return vec3(y, cb, cr);
}

void main() {
    vec4 sampled = texture(u_albedo, v_uv);
    vec3 rgb = sampled.rgb;

    if (!u_enableKey) {
        // Raw overlay: show the texture untouched (state 1 of the L cycle).
        fragColor = vec4(rgb, 1.0);
        return;
    }

    // Chroma-key in YCbCr: distance to the key color in the (Cb, Cr) plane.
    // Pixels close to the key chroma get discarded; a small softness band fades the edges.
    vec3 pixelYCbCr = rgbToYCbCr(rgb);
    vec3 keyYCbCr   = rgbToYCbCr(u_keyColor);

    float chromaDist = distance(pixelYCbCr.yz, keyYCbCr.yz);

    if (chromaDist < u_keyThreshold) {
        discard;
    }

    float alpha = smoothstep(u_keyThreshold, u_keyThreshold + max(u_keySoftness, 1e-4), chromaDist);
    fragColor = vec4(rgb, alpha);
}
