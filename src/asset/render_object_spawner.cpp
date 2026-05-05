#include "asset/render_object_spawner.hpp"

#include "asset/model.hpp"
#include "asset/shader.hpp"
#include "asset/texture.hpp"
#include "graphics/geometry.hpp"
#include "log/log.hpp"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <glm/ext/vector_float3.hpp>
#include <limits>
#include <optional>
#include <vector>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat4x4.hpp>

namespace asset {
namespace {

struct SpawnAssets {
    Model model{};
    ShaderProgram program{};
    Texture overrideTexture{};
};

[[nodiscard]] Texture createSolidWhiteTexture() {
    GLuint texture = 0U;
    glGenTextures(1, &texture);
    if (texture == 0U) {
        return Texture{};
    }
    glBindTexture(GL_TEXTURE_2D, texture);
    constexpr std::uint8_t kWhite[4] = {255U, 255U, 255U, 255U};
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, kWhite);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glBindTexture(GL_TEXTURE_2D, 0);
    return Texture{texture};
}

[[nodiscard]] std::optional<Texture> tryGetMaterialBaseColorTexture(const Model& model,
                                                                    std::size_t meshIndex) {
    if (meshIndex >= model.meshes.size()) {
        return std::nullopt;
    }
    const Mesh& mesh = model.meshes[meshIndex];
    if (mesh.materialIndex >= model.materials.size()) {
        return std::nullopt;
    }
    const Material& material = model.materials[mesh.materialIndex];
    for (const MaterialTextureBinding& binding : material.textures) {
        if (binding.slot == MaterialTextureSlot::BaseColor && binding.texture.textureHdl != 0U) {
            return binding.texture;
        }
    }
    return std::nullopt;
}

[[nodiscard]] glm::mat4 buildModelMatrix(const Model& model,
                                         const RenderObjectSpawnRequest& request) {
    glm::vec3 boundsMin(std::numeric_limits<float>::max());
    glm::vec3 boundsMax(std::numeric_limits<float>::lowest());
    for (const Mesh& mesh : model.meshes) {
        for (const Vertex& v : mesh.vertices) {
            const glm::vec3 p(v.position[0], v.position[1], v.position[2]);
            boundsMin = glm::min(boundsMin, p);
            boundsMax = glm::max(boundsMax, p);
        }
    }
    const glm::vec3 center = 0.5f * (boundsMin + boundsMax);
    const glm::vec3 extent = boundsMax - boundsMin;
    const float maxExtent = std::max({extent.x, extent.y, extent.z, 1.0e-4f});
    const float uniformScale = request.uniformTargetSize / maxExtent;

    glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), request.worldPosition) *
                            glm::scale(glm::mat4(1.0f), glm::vec3(uniformScale));
    if (request.centerModel) {
        modelMatrix = modelMatrix * glm::translate(glm::mat4(1.0f), -center);
    }
    return modelMatrix;
}

[[nodiscard]] std::optional<SpawnAssets> loadSpawnAssets(const RenderObjectSpawnRequest& request,
                                                         std::string& error) {
    SpawnAssets assets{};
    const std::filesystem::path modelPath = resolveAssetPath(request.modelPath);
    const std::filesystem::path vertPath = resolveAssetPath(request.vertexShaderPath);
    const std::filesystem::path fragPath = resolveAssetPath(request.fragmentShaderPath);

    if (!request.spawnFromRawVertices) {
        assets.model = AssetLoader::loadModel(modelPath.string());
        if (assets.model.meshes.empty()) {
            error = "Model has no meshes: " + modelPath.string();
            return std::nullopt;
        }
    }

    const auto programOpt = loadShaderProgramFromPaths(vertPath.string(), fragPath.string());
    if (!programOpt.has_value()) {
        error = "Failed to load shaders: " + vertPath.string() + " / " + fragPath.string();
        return std::nullopt;
    }
    assets.program = *programOpt;

    if (!request.overrideTexturePath.empty()) {
        const std::filesystem::path overridePath = resolveAssetPath(request.overrideTexturePath);
        assets.overrideTexture = AssetLoader::loadTexture(overridePath.string());
    }
    return assets;
}

std::vector<rendering::RenderMeshInstance>
buildGpuMeshInstances(const SpawnAssets& assets, const RenderObjectSpawnRequest& request,
                      TextureSelection& selectedTexture) {
    std::vector<rendering::RenderMeshInstance> out;
    out.reserve(assets.model.meshes.size());
    const glm::mat4 modelMatrix = buildModelMatrix(assets.model, request);
    Texture generatedWhiteTexture{};

    for (std::size_t meshIndex = 0; meshIndex < assets.model.meshes.size(); ++meshIndex) {
        const Mesh& mesh = assets.model.meshes[meshIndex];
        std::vector<rendering::GpuVertex> interleaved;
        interleaved.reserve(mesh.vertices.size());
        for (const Vertex& v : mesh.vertices) {
            interleaved.push_back(rendering::GpuVertex{
                .px = v.position[0],
                .py = v.position[1],
                .pz = v.position[2],
                .nx = v.normal[0],
                .ny = v.normal[1],
                .nz = v.normal[2],
                .u = v.texCoord[0],
                .v = v.texCoord[1],
            });
        }

        auto buffers = std::make_shared<rendering::RenderMeshInstance::MeshBuffers>();
        glGenVertexArrays(1, &buffers->vao);
        glGenBuffers(1, &buffers->vbo);
        glGenBuffers(1, &buffers->ebo);
        if (buffers->vao == 0U || buffers->vbo == 0U || buffers->ebo == 0U) {
            LOG_ERROR("OpenGL buffer creation failed for object '{}' mesh {}", request.namePrefix,
                      meshIndex);
            continue;
        }

        glBindVertexArray(buffers->vao);
        glBindBuffer(GL_ARRAY_BUFFER, buffers->vbo);
        glBufferData(GL_ARRAY_BUFFER,
                     static_cast<GLsizeiptr>(interleaved.size() * sizeof(rendering::GpuVertex)),
                     interleaved.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffers->ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                     static_cast<GLsizeiptr>(mesh.indices.size() * sizeof(std::uint32_t)),
                     mesh.indices.data(), GL_STATIC_DRAW);

        constexpr GLsizei stride = sizeof(rendering::GpuVertex);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride,
                              reinterpret_cast<const void*>(offsetof(rendering::GpuVertex, px)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride,
                              reinterpret_cast<const void*>(offsetof(rendering::GpuVertex, nx)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride,
                              reinterpret_cast<const void*>(offsetof(rendering::GpuVertex, u)));
        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        glm::vec3 meshMin(std::numeric_limits<float>::max());
        glm::vec3 meshMax(std::numeric_limits<float>::lowest());
        for (const Vertex& v : mesh.vertices) {
            const glm::vec3 p(v.position[0], v.position[1], v.position[2]);
            meshMin = glm::min(meshMin, p);
            meshMax = glm::max(meshMax, p);
        }

        Texture diffuseTexture{};
        if (request.useModelMaterialTexture) {
            diffuseTexture =
                tryGetMaterialBaseColorTexture(assets.model, meshIndex).value_or(Texture{});
        }
        if (diffuseTexture.textureHdl != 0U) {
            selectedTexture = TextureSelection::ModelMaterial;
        } else if (assets.overrideTexture.textureHdl != 0U) {
            diffuseTexture = assets.overrideTexture;
            selectedTexture = TextureSelection::OverridePath;
        } else {
            if (generatedWhiteTexture.textureHdl == 0U) {
                generatedWhiteTexture = createSolidWhiteTexture();
            }
            diffuseTexture = generatedWhiteTexture;
            selectedTexture = TextureSelection::GeneratedWhite;
        }

        rendering::RenderMeshInstance instance{};
        instance.buffers = std::move(buffers);
        instance.indexCount = static_cast<GLsizei>(mesh.indices.size());
        instance.indexType = GL_UNSIGNED_INT;
        instance.shaderProgram = assets.program.id;
        instance.shaderLifetime = assets.program.resource;
        instance.locMvp = assets.program.resource ? assets.program.resource->locMvp : -1;
        instance.locModel = assets.program.resource ? assets.program.resource->locModel : -1;
        instance.locView = assets.program.resource ? assets.program.resource->locView : -1;
        instance.locProjection =
            assets.program.resource ? assets.program.resource->locProjection : -1;
        instance.locColor = assets.program.resource ? assets.program.resource->locColor : -1;
        instance.locAlbedo = assets.program.resource ? assets.program.resource->locAlbedo : -1;
        instance.diffuseTexture = diffuseTexture.textureHdl;
        instance.modelMatrix = modelMatrix;
        instance.layer = request.layer;
        instance.modelBounds = AABB{meshMin, meshMax};
        instance.useFrustumCull = request.enableFrustumCull;
        out.push_back(std::move(instance));
    }
    return out;
}

std::vector<rendering::RenderMeshInstance>
buildGpuRawMeshInstances(const SpawnAssets& assets, const std::vector<glm::vec3>* rawVertices,
                         const RenderObjectSpawnRequest& request) {
    std::vector<rendering::RenderMeshInstance> out;

    // calc model matrix for raw vertices
    glm::vec3 boundsMin(std::numeric_limits<float>::max());
    glm::vec3 boundsMax(std::numeric_limits<float>::lowest());
    for (const glm::vec3& v : *rawVertices) {
        boundsMin = glm::min(boundsMin, v);
        boundsMax = glm::max(boundsMax, v);
    }
    const glm::vec3 center = 0.5f * (boundsMin + boundsMax);
    const glm::vec3 extent = boundsMax - boundsMin;
    const float maxExtent = std::max({extent.x, extent.y, extent.z, 1.0e-4f});
    const float uniformScale = request.uniformTargetSize / maxExtent;

    glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), request.worldPosition) *
                            glm::scale(glm::mat4(1.0f), glm::vec3(uniformScale));
    if (request.centerModel) {
        modelMatrix = modelMatrix * glm::translate(glm::mat4(1.0f), -center);
    }

    //
    auto buffers = std::make_shared<rendering::RenderMeshInstance::MeshBuffers>();
    glGenVertexArrays(1, &buffers->vao);
    glGenBuffers(1, &buffers->vbo);
    if (buffers->vao == 0U || buffers->vbo == 0U) {
        LOG_ERROR("OpenGL buffer creation failed for object '{}'", request.namePrefix);
        return std::vector<rendering::RenderMeshInstance>{};
    }

    glBindVertexArray(buffers->vao);
    glBindBuffer(GL_ARRAY_BUFFER, buffers->vbo);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(rawVertices->size() * sizeof(glm::vec3)),
                 rawVertices->data(), GL_STATIC_DRAW);

    constexpr GLsizei stride = sizeof(glm::vec3);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glm::vec3 meshMin(std::numeric_limits<float>::max());
    glm::vec3 meshMax(std::numeric_limits<float>::lowest());
    for (const glm::vec3& v : *rawVertices) {
        meshMin = glm::min(meshMin, v);
        meshMax = glm::max(meshMax, v);
    }

    rendering::RenderMeshInstance instance{};
    instance.buffers = std::move(buffers);
    instance.drawArraysVertexCount = rawVertices->size();

    instance.shaderProgram = assets.program.id;
    instance.shaderLifetime = assets.program.resource;

    instance.locMvp = assets.program.resource ? assets.program.resource->locMvp : -1;
    instance.locModel = assets.program.resource ? assets.program.resource->locModel : -1;
    instance.modelMatrix = modelMatrix;

    instance.drawArraysPrimitive = GL_LINE_STRIP;
    instance.layer = request.layer;
    instance.modelBounds = AABB{meshMin, meshMax};
    instance.useFrustumCull = request.enableFrustumCull;
    out.push_back(std::move(instance));

    return out;
};

std::size_t registerSpawnedMeshes(Registry& registry, const std::string& namePrefix,
                                  std::vector<rendering::RenderMeshInstance>&& instances) {
    std::size_t meshCount = 0U;
    for (std::size_t i = 0; i < instances.size(); ++i) {
        registry.registerObject(namePrefix + ".mesh." + std::to_string(i), std::move(instances[i]));
        ++meshCount;
    }
    return meshCount;
}

}  // namespace

RenderObjectSpawnResult spawnModelAsRenderMeshes(Registry& registry,
                                                 const RenderObjectSpawnRequest& request) {
    RenderObjectSpawnResult result{};

    const auto assetsOpt = loadSpawnAssets(request, result.error);
    if (!assetsOpt.has_value()) {
        return result;
    }
    result.programId = assetsOpt->program.id;

    auto built = (request.spawnFromRawVertices)
                     ? buildGpuRawMeshInstances(*assetsOpt, request.rawVertices, request)
                     : buildGpuMeshInstances(*assetsOpt, request, result.textureSelection);

    result.meshCount = registerSpawnedMeshes(registry, request.namePrefix, std::move(built));
    if (result.meshCount == 0U) {
        result.error = "No renderable meshes were spawned for " + request.namePrefix;
    }
    return result;
}

}  // namespace asset
