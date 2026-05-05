#ifndef CG_OPENGL_PROJECT_CURVES_HPP
#define CG_OPENGL_PROJECT_CURVES_HPP

#include "asset/render_object_spawner.hpp"

#include "world/registry.hpp"
#include "graphics/geometry.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/vec3.hpp>
#include <vector>

namespace curves {
struct Transform {
    glm::vec3 position{0.0f, -20.0f, -80.0f};
    glm::vec3 rotationDeg{0.0f, 0.0f, 0.0f};
    glm::vec3 scale{1.0f, 1.0f, 1.0f};
    float uniformTargetSize = 1200.0f;
};

inline void setupSystem(Registry& registry) {
    static const Transform kTransform{};
    static constexpr std::string_view kSpawnPrefix = "scene.island.curveChain";

    CubicBezierCurve a =
        CubicBezierCurve(glm::vec3{1000, 0, 0}, glm::vec3{1000, 1000, 0},
                         glm::vec3{1000, -1000, 1000}, glm::vec3{1000, 500, 0}, 500);

    asset::RenderObjectSpawnRequest request{};
    request.namePrefix = std::string(kSpawnPrefix);
    request.vertexShaderPath = "assets/shaders/raw_vert.vert";
    request.fragmentShaderPath = "assets/shaders/raw_vert.frag";
    request.worldPosition = kTransform.position;
    request.uniformTargetSize = kTransform.uniformTargetSize;
    request.centerModel = true;
    request.layer = rendering::RenderLayer::DebugOverlay;
    request.enableFrustumCull = true;
    request.modelPath = "";

    request.spawnFromRawVertices = true;
    request.rawVertices = &a.samplePoints;

    const asset::RenderObjectSpawnResult result =
        asset::spawnModelAsRenderMeshes(registry, request);
    if (!result.error.empty()) {
        LOG_WARN("failed: {}", result.error);
    }

    glm::mat4 rotate = glm::mat4(1.0f);
    rotate =
        glm::rotate(rotate, glm::radians(kTransform.rotationDeg.x), glm::vec3(1.0f, 0.0f, 0.0f));
    rotate =
        glm::rotate(rotate, glm::radians(kTransform.rotationDeg.y), glm::vec3(0.0f, 1.0f, 0.0f));
    rotate =
        glm::rotate(rotate, glm::radians(kTransform.rotationDeg.z), glm::vec3(0.0f, 0.0f, 1.0f));
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

    LOG_INFO("Spawned '{}' ({} meshes)", request.namePrefix, result.meshCount);
    return;
}
}  // namespace curves

#endif  //  CG_OPENGL_PROJECT_CURVES_HPP