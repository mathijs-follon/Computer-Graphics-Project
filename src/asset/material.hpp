#ifndef CG_OPENGL_PROJECT_MATERIAL_HPP
#define CG_OPENGL_PROJECT_MATERIAL_HPP

#include "texture.hpp"

#include <array>
#include <string>
#include <vector>

namespace asset {

enum class MaterialTextureSlot {
    // Koppelt een texture aan een vaste shader-rol.
    BaseColor,
    Normal,
    MetallicRoughness,
    Occlusion,
    Emissive,
    Specular,
};

struct MaterialTextureBinding {
    MaterialTextureSlot slot = MaterialTextureSlot::BaseColor;
    Texture texture{};
    std::string path;
};

struct Material {
    // Basiswaarden zodat een model ook zonder textures zichtbaar blijft.
    std::string name;
    std::array<float, 4> baseColorFactor{1.0F, 1.0F, 1.0F, 1.0F};
    std::array<float, 3> emissiveFactor{0.0F, 0.0F, 0.0F};
    float metallicFactor = 1.0F;
    float roughnessFactor = 1.0F;
    float opacity = 1.0F;
    float shininess = 0.0F;

    std::vector<MaterialTextureBinding> textures;
};

}  // namespace asset

#endif  // CG_OPENGL_PROJECT_MATERIAL_HPP
