#ifndef CG_OPENGL_PROJECT_ISLAND_HPP
#define CG_OPENGL_PROJECT_ISLAND_HPP

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

        LOG_INFO("Spawned '{}' from '{}' ({} meshes, texture={})", request.namePrefix, candidate,
                 result.meshCount, textureSelectionToString(result.textureSelection));
        return;
    }

    LOG_WARN("Could not find sea_keep model. Tried {} candidate path(s).", kModelCandidates.size());
}

}  // namespace island

#endif  // CG_OPENGL_PROJECT_ISLAND_HPP
