#ifndef CG_OPENGL_PROJECT_LIGHTS_HPP
#define CG_OPENGL_PROJECT_LIGHTS_HPP

#include "asset/render_object_spawner.hpp"
#include "asset/shader.hpp"
#include "graphics/rendering.hpp"
#include "world/registry.hpp"
#include <glm/ext/vector_float3.hpp>
#include <vector>

namespace lights {

struct Transform {
    glm::vec3 position{0.0f, -20.0f, -20.0f};
    glm::vec3 scale{1.0f, 1.0f, 1.0f};
    float uniformTargetSize = 1000.0f;
};

struct Light {
    Transform transform;
    glm::vec3 color = {1.0f, 1.0f, 1.0f};

    float linAtt = 0.001f;
    float quadAtt = 0.0001f;

    float ambientFac = 0.10;
    float diffuseFac = 0.50;
    float specularFac = 0.40;
};

// spawn the models & set initial lighting
inline void setupSystem(Registry& registry) {
    static constexpr std::array<const char*, 1> kModelCandidates = {
        "assets/models/the_utah_teapot/scene.gltf",
    };
    static constexpr std::string_view kSpawnPrefix = "scene.island.light";

    static constexpr Light Sun = Light{{{500.0f, 0.0f, 0.0f}, {100.0f, 100.0f, 100.0f}},
                                       {1.0f, 0.8f, 0.8f},
                                       0.00000001f,
                                       0.0000000001f,
                                       0.2,
                                       0.5,
                                       0.3};

    std::vector<Light> lights{Sun};

    Transform kTransform{};
    // spawn pots
    for (uint i = 0; i < asset::ShaderProgram::MAX_LIGHT_COUNT; i++) {
        // init transforms
        kTransform.position *= 10 * (1 + i);
        lights.push_back({kTransform});

        asset::RenderObjectSpawnRequest request{};
        request.namePrefix = std::string(kSpawnPrefix);

        request.vertexShaderPath = "assets/shaders/default.vert";
        request.fragmentShaderPath = "assets/shaders/default.frag";
        request.overrideTexturePath = "assets/textures/fallback.png";

        request.useModelMaterialTexture = true;
        request.worldPosition = kTransform.position;
        request.uniformTargetSize = kTransform.uniformTargetSize;
        request.centerModel = true;
        request.layer = rendering::RenderLayer::Sky;
        request.enableFrustumCull = true;

        // spawn pots at light positions
        for (const char* candidate : kModelCandidates) {
            if (asset::resolveAssetPath(candidate) == std::filesystem::path(candidate) &&
                !std::filesystem::exists(candidate)) {
                continue;
            }

            request.modelPath = candidate;
            const asset::RenderObjectSpawnResult result =
                asset::spawnModelAsRenderMeshes(registry, request);
            if (!result.error.empty()) {
                LOG_WARN("light model spawn candidate '{}' failed: {}", candidate, result.error);
                continue;
            }

            const glm::mat4 pivotToOrigin = glm::translate(glm::mat4(1.0f), -kTransform.position);
            const glm::mat4 pivotBack = glm::translate(glm::mat4(1.0f), kTransform.position);
            const glm::mat4 worldScale =
                pivotBack * glm::scale(glm::mat4(1.0f), kTransform.scale) * pivotToOrigin;

            const std::string meshPrefix = request.namePrefix + ".mesh.";
            for (auto [name, mesh] : registry.getEntries<rendering::RenderMeshInstance>()) {
                if (mesh == nullptr || !name.starts_with(meshPrefix)) {
                    continue;
                }
                mesh->modelMatrix = worldScale * mesh->modelMatrix;
            }

            LOG_INFO("Spawned '{}' from '{}' ({} meshes)", request.namePrefix, candidate,
                     result.meshCount);
        }
    }

    // move lighting data into mesh shaders that support it
    for (const asset::ShaderProgram* meshShaderPrgm : registry.getObjects<asset::ShaderProgram>()) {
        GLint lightActiveLocation = glGetUniformLocation(meshShaderPrgm->id, "u_lights[0].on");
        if (lightActiveLocation >= 0) {
            glUseProgram(meshShaderPrgm->id);
            // move light data for each light
            for (uint i = 0; i < asset::ShaderProgram::MAX_LIGHT_COUNT; i++) {
                asset::ShaderProgram::ProgramResource::LightResource* lightResources =
                    &meshShaderPrgm->resource->LightResources[i];

                if (i < lights.size()) {
                    glUniform1i(lightResources->locActive, GL_TRUE);
                    glUniform3fv(lightResources->locPos, 1,
                                 glm::value_ptr(lights[i].transform.position));
                    glUniform3fv(lightResources->locColor, 1, glm::value_ptr(lights[i].color));
                    glUniform1f(lightResources->locLinAtt, lights[i].linAtt);
                    glUniform1f(lightResources->locQuadAtt, lights[i].quadAtt);

                    glUniform1f(lightResources->locAmbient, lights[i].ambientFac);
                    glUniform1f(lightResources->locDiffuse, lights[i].diffuseFac);
                    glUniform1f(lightResources->locSpecular, lights[i].specularFac);

                } else {
                    glUniform1i(lightResources->locActive, GL_FALSE);
                };
            }

            glUseProgram(0);
        }
    }
}

}  // namespace lights

#endif  // CG_OPENGL_PROJECT_LIGHTS_HPP