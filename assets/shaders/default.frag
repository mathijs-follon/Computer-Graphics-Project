#version 460 core
#define LIGHT_COUNT_MAX 16

struct Material {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float shininess;
}; 

struct PointLight
{
    int on;
    vec3 pos;
    vec3 color;
    
    float linAtt;
    float quadAtt;  

    float ambientFac;
    float diffuseFac;
    float specularFac;
};

in vec3 v_worldNormal;
in vec2 v_uv;
in vec3 v_frag_pos;

uniform sampler2D u_albedo;
uniform Material u_material;
uniform vec3 u_viewPos;
uniform PointLight u_lights[LIGHT_COUNT_MAX];

out vec4 fragColor;

vec3 addPointLighting(PointLight light, vec3 normal, vec3 viewDir)
{
    vec3 lightDir = normalize(light.pos - v_frag_pos);
    float diffuseStrength = max(dot(normal, lightDir), 0.0);
   
    vec3 reflectDir = reflect(-lightDir, normal);  
    float specularStrength = pow(max(dot(viewDir, reflectDir), 0.0), u_material.shininess);

    float distance = length(light.pos - v_frag_pos);
    float attenuation = 1.0 / (1.0 + light.linAtt * distance + light.quadAtt * distance * distance);    

    vec3 ambient = attenuation * light.ambientFac * light.color;
    vec3 diffuse  = attenuation * light.diffuseFac * diffuseStrength *  light.color;
    vec3 specular = attenuation * light.specularFac *  specularStrength * light.color;

    return (ambient+diffuse+specular);
} 

void main() {
    vec4 sampled = texture(u_albedo, v_uv);
    // Alpha-test cutout: model materials flagged BLEND in the gltf (foliage, kelp,
    // lattice grates) carry per-pixel cutout masks. Without this, the texture's
    // padding regions render as opaque black rectangles around leaves/grates.
    if (sampled.a < 0.5) {
        discard;
    }
    vec3 albedo = sampled.rgb;

    vec3 viewDir = normalize(u_viewPos - v_frag_pos);
    vec3 n = normalize(v_worldNormal);

    vec3 shaded = vec3(0.0);
    for (int i = 0; i < LIGHT_COUNT_MAX; i++) {
        if (u_lights[i].on == 1) {
            shaded += addPointLighting(u_lights[i], n, viewDir); 
        }
    }

    shaded *= albedo;
    fragColor = vec4(shaded, 1.0);
}

