#ifndef CG_OPENGL_PROJECT_RENDER_OBJECT_SPAWNER_HPP
#define CG_OPENGL_PROJECT_RENDER_OBJECT_SPAWNER_HPP

#include "asset.hpp"
#include "graphics/rendering.hpp"
#include "world/registry.hpp"

#include <cstddef>
#include <glm/ext/vector_float3.hpp>
#include <string>

#include <glm/vec3.hpp>

namespace asset {

enum class TextureSelection {
    ModelMaterial,
    OverridePath,
    GeneratedWhite,
};

struct RenderObjectSpawnRequest {
    std::string namePrefix;
    std::string modelPath;
    std::string vertexShaderPath;
    std::string fragmentShaderPath;
    std::string overrideTexturePath;
    bool useModelMaterialTexture = true;

    bool spawnFromRawVertices = false;
    std::vector<glm::vec3>* rawVertices = nullptr;

    glm::vec3 worldPosition{0.0f, 0.0f, 0.0f};
    float uniformTargetSize = 1.0f;
    bool centerModel = true;

    rendering::RenderLayer layer = rendering::RenderLayer::Opaque;
    bool enableFrustumCull = false;
};

struct RenderObjectSpawnResult {
    std::size_t meshCount = 0;
    GLuint programId = 0U;
    TextureSelection textureSelection = TextureSelection::GeneratedWhite;
    std::string error;
};

[[nodiscard]] RenderObjectSpawnResult
spawnModelAsRenderMeshes(Registry& registry, const RenderObjectSpawnRequest& request);

}  // namespace asset

#endif  // CG_OPENGL_PROJECT_RENDER_OBJECT_SPAWNER_HPP
