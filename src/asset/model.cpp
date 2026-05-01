#include "asset/model.hpp"

#include <filesystem>
#include <utility>

#include <assimp/Importer.hpp>
#include <assimp/material.h>
#include <assimp/matrix4x4.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace asset {
namespace {

[[nodiscard]] glm::mat4 assimpMatrixToGlm(const aiMatrix4x4& from) {
    // Assimp gebruikt row-major layout; transpose geeft correcte GLM matrix.
    return glm::transpose(glm::make_mat4(&from.a1));
}

void collectMeshWorldFromRoot(const aiNode* node, const glm::mat4& parentWorld, std::vector<glm::mat4>& meshWorld) {
    const glm::mat4 local = assimpMatrixToGlm(node->mTransformation);
    const glm::mat4 world = parentWorld * local;
    for (unsigned int i = 0; i < node->mNumMeshes; ++i) {
        const unsigned int meshIndex = node->mMeshes[i];
        if (meshIndex < meshWorld.size()) {
            meshWorld[meshIndex] = world;
        }
    }
    for (unsigned int i = 0; i < node->mNumChildren; ++i) {
        collectMeshWorldFromRoot(node->mChildren[i], world, meshWorld);
    }
}

void bakeVerticesToWorldSpace(Mesh& mesh, const glm::mat4& worldFromLocal) {
    // Normal matrix voorkomt fout licht bij non-uniforme schaal.
    const glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(worldFromLocal)));
    for (Vertex& vertex : mesh.vertices) {
        const glm::vec4 pLocal(vertex.position[0], vertex.position[1], vertex.position[2], 1.0f);
        const glm::vec3 pWorld = glm::vec3(worldFromLocal * pLocal);
        vertex.position = {pWorld.x, pWorld.y, pWorld.z};

        glm::vec3 normal(vertex.normal[0], vertex.normal[1], vertex.normal[2]);
        if (glm::dot(normal, normal) > 1.0e-20f) {
            normal = glm::normalize(normalMatrix * normal);
            vertex.normal = {normal.x, normal.y, normal.z};
        }
        glm::vec3 tangent(vertex.tangent[0], vertex.tangent[1], vertex.tangent[2]);
        if (glm::dot(tangent, tangent) > 1.0e-20f) {
            tangent = glm::normalize(normalMatrix * tangent);
            vertex.tangent = {tangent.x, tangent.y, tangent.z};
        }
        glm::vec3 bitangent(vertex.bitangent[0], vertex.bitangent[1], vertex.bitangent[2]);
        if (glm::dot(bitangent, bitangent) > 1.0e-20f) {
            bitangent = glm::normalize(normalMatrix * bitangent);
            vertex.bitangent = {bitangent.x, bitangent.y, bitangent.z};
        }
    }
}

}  // namespace

std::optional<Model> loadModelFromPath(std::string_view path) {
    namespace fs = std::filesystem;
    const fs::path sourcePath(path);
    if (sourcePath.empty()) {
        return std::nullopt;
    }

    Assimp::Importer importer;
    constexpr unsigned int importFlags = aiProcess_Triangulate | aiProcess_JoinIdenticalVertices |
                                         aiProcess_ImproveCacheLocality | aiProcess_GenNormals |
                                         aiProcess_CalcTangentSpace | aiProcess_SortByPType;
    const aiScene* scene = importer.ReadFile(sourcePath.string(), importFlags);
    if (scene == nullptr || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) != 0U || scene->mRootNode == nullptr) {
        return std::nullopt;
    }

    Model model{};
    model.sourcePath = sourcePath.string();
    model.materials.reserve(scene->mNumMaterials == 0U ? 1U : scene->mNumMaterials);
    const fs::path baseDirectory = sourcePath.parent_path();
    const auto resolveTexturePath = [&baseDirectory](const aiString& texturePath) {
        fs::path candidate(texturePath.C_Str());
        if (candidate.is_absolute()) {
            return candidate;
        }
        // Relatieve paden in modelbestanden oplossen vanaf modelmap.
        return (baseDirectory / candidate).lexically_normal();
    };

    for (unsigned int materialIndex = 0; materialIndex < scene->mNumMaterials; ++materialIndex) {
        const aiMaterial* aiMaterialData = scene->mMaterials[materialIndex];
        Material material{};
        if (aiString materialName; aiMaterialData->Get(AI_MATKEY_NAME, materialName) == aiReturn_SUCCESS) {
            material.name = materialName.C_Str();
        }
        aiColor4D diffuse{};
        if (aiGetMaterialColor(aiMaterialData, AI_MATKEY_COLOR_DIFFUSE, &diffuse) == aiReturn_SUCCESS) {
            material.baseColorFactor = {diffuse.r, diffuse.g, diffuse.b, diffuse.a};
        }
        aiColor4D emissive{};
        if (aiGetMaterialColor(aiMaterialData, AI_MATKEY_COLOR_EMISSIVE, &emissive) == aiReturn_SUCCESS) {
            material.emissiveFactor = {emissive.r, emissive.g, emissive.b};
        }
        ai_real opacity = 1.0;
        if (aiGetMaterialFloat(aiMaterialData, AI_MATKEY_OPACITY, &opacity) == aiReturn_SUCCESS) {
            material.opacity = static_cast<float>(opacity);
        }
        ai_real shininess = 0.0;
        if (aiGetMaterialFloat(aiMaterialData, AI_MATKEY_SHININESS, &shininess) == aiReturn_SUCCESS) {
            material.shininess = static_cast<float>(shininess);
        }

        const auto addTexturesByType = [&](aiTextureType textureType, MaterialTextureSlot slot) {
            // Laad per slot alleen geldige textures; ontbrekende bestanden mogen overgeslagen worden.
            for (unsigned int textureIndex = 0; textureIndex < aiMaterialData->GetTextureCount(textureType);
                 ++textureIndex) {
                aiString texturePath;
                if (aiMaterialData->GetTexture(textureType, textureIndex, &texturePath) != aiReturn_SUCCESS) {
                    continue;
                }
                const fs::path resolvedPath = resolveTexturePath(texturePath);
                const std::optional<Texture> loadedTexture = loadTextureFromPath(resolvedPath.string());
                if (!loadedTexture.has_value()) {
                    continue;
                }
                MaterialTextureBinding binding{};
                binding.slot = slot;
                binding.texture = loadedTexture.value();
                binding.path = resolvedPath.string();
                material.textures.push_back(std::move(binding));
            }
        };

        addTexturesByType(aiTextureType_DIFFUSE, MaterialTextureSlot::BaseColor);
        addTexturesByType(aiTextureType_NORMALS, MaterialTextureSlot::Normal);
        addTexturesByType(aiTextureType_SPECULAR, MaterialTextureSlot::Specular);
        addTexturesByType(aiTextureType_EMISSIVE, MaterialTextureSlot::Emissive);
        addTexturesByType(aiTextureType_AMBIENT_OCCLUSION, MaterialTextureSlot::Occlusion);
        addTexturesByType(aiTextureType_BASE_COLOR, MaterialTextureSlot::BaseColor);
        addTexturesByType(aiTextureType_NORMAL_CAMERA, MaterialTextureSlot::Normal);
        addTexturesByType(aiTextureType_METALNESS, MaterialTextureSlot::MetallicRoughness);
        addTexturesByType(aiTextureType_DIFFUSE_ROUGHNESS, MaterialTextureSlot::MetallicRoughness);
        model.materials.push_back(std::move(material));
    }
    if (model.materials.empty()) {
        model.materials.emplace_back();
    }

    std::vector<glm::mat4> meshWorld(scene->mNumMeshes, glm::mat4(1.0f));
    // Mesh-transforms uit de scene-hiërarchie verzamelen om vertices naar world te bakken.
    collectMeshWorldFromRoot(scene->mRootNode, glm::mat4(1.0f), meshWorld);

    model.meshes.reserve(scene->mNumMeshes);
    for (unsigned int meshIndex = 0; meshIndex < scene->mNumMeshes; ++meshIndex) {
        const aiMesh* aiMeshData = scene->mMeshes[meshIndex];
        if (aiMeshData == nullptr || aiMeshData->mPrimitiveTypes != aiPrimitiveType_TRIANGLE) {
            continue;
        }
        Mesh mesh{};
        mesh.name = aiMeshData->mName.C_Str();
        mesh.materialIndex = static_cast<std::size_t>(
            aiMeshData->mMaterialIndex < model.materials.size() ? aiMeshData->mMaterialIndex : 0);
        mesh.vertices.reserve(aiMeshData->mNumVertices);
        for (unsigned int vertexIndex = 0; vertexIndex < aiMeshData->mNumVertices; ++vertexIndex) {
            Vertex vertex{};
            const aiVector3D& position = aiMeshData->mVertices[vertexIndex];
            vertex.position = {position.x, position.y, position.z};
            if (aiMeshData->HasNormals()) {
                const aiVector3D& normal = aiMeshData->mNormals[vertexIndex];
                vertex.normal = {normal.x, normal.y, normal.z};
            }
            if (aiMeshData->HasTextureCoords(0)) {
                const aiVector3D& uv = aiMeshData->mTextureCoords[0][vertexIndex];
                vertex.texCoord = {uv.x, uv.y};
            }
            if (aiMeshData->HasTangentsAndBitangents()) {
                const aiVector3D& tangent = aiMeshData->mTangents[vertexIndex];
                const aiVector3D& bitangent = aiMeshData->mBitangents[vertexIndex];
                vertex.tangent = {tangent.x, tangent.y, tangent.z};
                vertex.bitangent = {bitangent.x, bitangent.y, bitangent.z};
            }
            mesh.vertices.push_back(vertex);
        }
        bakeVerticesToWorldSpace(mesh, meshWorld[meshIndex]);

        mesh.indices.reserve(aiMeshData->mNumFaces * 3U);
        for (unsigned int faceIndex = 0; faceIndex < aiMeshData->mNumFaces; ++faceIndex) {
            const aiFace& face = aiMeshData->mFaces[faceIndex];
            if (face.mNumIndices != 3U) {
                continue;
            }
            mesh.indices.push_back(face.mIndices[0]);
            mesh.indices.push_back(face.mIndices[1]);
            mesh.indices.push_back(face.mIndices[2]);
        }
        if (!mesh.vertices.empty() && !mesh.indices.empty()) {
            model.meshes.push_back(std::move(mesh));
        }
    }

    if (model.meshes.empty()) {
        return std::nullopt;
    }
    return model;
}

}  // namespace asset
