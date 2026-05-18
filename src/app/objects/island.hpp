#ifndef CG_OPENGL_PROJECT_ISLAND_HPP
#define CG_OPENGL_PROJECT_ISLAND_HPP

#include "asset/asset.hpp"
#include "asset/asset_paths.hpp"
#include "asset/render_object_spawner.hpp"
#include "graphics/rendering.hpp"
#include "log/log.hpp"
#include "world/registry.hpp"

#include <array>
#include <filesystem>
#include <string>
#include <string_view>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/vec3.hpp>

namespace island {
struct Transform {
    glm::vec3 position{0.0f, -20.0f, -80.0f};
    glm::vec3 rotationDeg{0.0f, 0.0f, 0.0f};
    glm::vec3 scale{2.0f, 2.0f, 2.0f};
    float uniformTargetSize = 1200.0f;
};

inline bool isIslandWaterMeshName(const std::string_view meshName) {
    return meshName.starts_with("Sea");
}

inline void applyWaterShaderToIslandMeshes(Registry& registry, const std::string_view namePrefix,
                                           const asset::Model& model,
                                           const asset::ShaderProgram& waterProgram) {
    if (waterProgram.id == 0U || waterProgram.resource == nullptr) {
        return;
    }

    const std::string meshKeyPrefix = std::string(namePrefix) + ".mesh.";
    for (std::size_t meshIndex = 0; meshIndex < model.meshes.size(); ++meshIndex) {
        if (!isIslandWaterMeshName(model.meshes[meshIndex].name)) {
            continue;
        }

        auto* mesh =
            registry.getObject<rendering::RenderMeshInstance>(meshKeyPrefix + std::to_string(meshIndex));
        if (mesh == nullptr) {
            continue;
        }

        mesh->shaderProgram = waterProgram.id;
        mesh->shaderLifetime = waterProgram.resource;
        mesh->locMvp = waterProgram.resource->locMvp;
        mesh->locModel = waterProgram.resource->locModel;
        mesh->locView = waterProgram.resource->locView;
        mesh->locProjection = waterProgram.resource->locProjection;
        mesh->locColor = waterProgram.resource->locColor;
        mesh->locAlbedo = waterProgram.resource->locAlbedo;
        mesh->layer = rendering::RenderLayer::Transparent;
        mesh->transparentDepthBias = true;
        mesh->useFrustumCull = false;
    }
}

inline const char* textureSelectionToString(const asset::TextureSelection selection) {
    switch (selection) {
        case asset::TextureSelection::ModelMaterial:
            return "model_material";
        case asset::TextureSelection::OverridePath:
            return "override_path";
        case asset::TextureSelection::GeneratedWhite:
            return "generated_white";
    }
    return "unknown";
}

inline void setupSystem(Registry& registry) {
    static constexpr std::array<const char*, 4> kModelCandidates = {
        "assets/models/sea_keep/scene.gltf",
        "assets/models/sea_keep/sea_keep.gltf",
        "assets/models/island/sea_keep/scene.gltf",
        "assets/models/island/sea_keep/sea_keep.gltf",
    };

    static const Transform kTransform{};
    static constexpr std::string_view kSpawnPrefix = "scene.island.sea_keep";

    asset::RenderObjectSpawnRequest request{};
    request.namePrefix = std::string(kSpawnPrefix);
    request.vertexShaderPath = "assets/shaders/default.vert";
    request.fragmentShaderPath = "assets/shaders/default.frag";
    request.overrideTexturePath = "assets/textures/fallback.png";
    request.useModelMaterialTexture = true;
    request.worldPosition = kTransform.position;
    request.uniformTargetSize = kTransform.uniformTargetSize;
    request.centerModel = true;
    request.layer = rendering::RenderLayer::Opaque;
    request.enableFrustumCull = true;

    for (const char* candidate : kModelCandidates) {
        if (asset::resolveAssetPath(candidate) == std::filesystem::path(candidate) &&
            !std::filesystem::exists(candidate)) {
            continue;
        }

        request.modelPath = candidate;
        const asset::RenderObjectSpawnResult result =
            asset::spawnModelAsRenderMeshes(registry, request);
        if (!result.error.empty()) {
            LOG_WARN("Island spawn candidate '{}' failed: {}", candidate, result.error);
            continue;
        }

        glm::mat4 rotate = glm::mat4(1.0f);
        rotate = glm::rotate(rotate, glm::radians(kTransform.rotationDeg.x),
                             glm::vec3(1.0f, 0.0f, 0.0f));
        rotate = glm::rotate(rotate, glm::radians(kTransform.rotationDeg.y),
                             glm::vec3(0.0f, 1.0f, 0.0f));
        rotate = glm::rotate(rotate, glm::radians(kTransform.rotationDeg.z),
                             glm::vec3(0.0f, 0.0f, 1.0f));
        const glm::mat4 pivotToOrigin = glm::translate(glm::mat4(1.0f), -kTransform.position);
        const glm::mat4 pivotBack = glm::translate(glm::mat4(1.0f), kTransform.position);
        const glm::mat4 worldRotation = pivotBack * rotate * pivotToOrigin;
        const glm::mat4 worldScale =
            pivotBack * glm::scale(glm::mat4(1.0f), kTransform.scale) * pivotToOrigin;
        const glm::mat4 worldTransform = worldRotation * worldScale;

        const std::string meshPrefix = request.namePrefix + ".mesh.";
        for (auto [name, mesh] : registry.getEntries<rendering::RenderMeshInstance>()) {
            if (mesh == nullptr || !name.starts_with(meshPrefix)) {
                continue;
            }
            mesh->modelMatrix = worldTransform * mesh->modelMatrix;
        }

        const asset::Model islandModel = asset::AssetLoader::loadModel(candidate);
        const asset::ShaderProgram waterProgram = asset::AssetLoader::loadShaderProgram(
            "assets/shaders/water.vert", "assets/shaders/water.frag");
        applyWaterShaderToIslandMeshes(registry, request.namePrefix, islandModel, waterProgram);

        LOG_INFO("Spawned '{}' from '{}' ({} meshes, texture={})", request.namePrefix, candidate,
                 result.meshCount, textureSelectionToString(result.textureSelection));
        return;
    }

    LOG_WARN("Could not find sea_keep model. Tried {} candidate path(s).", kModelCandidates.size());
}

}  // namespace island

#endif  // CG_OPENGL_PROJECT_ISLAND_HPP
