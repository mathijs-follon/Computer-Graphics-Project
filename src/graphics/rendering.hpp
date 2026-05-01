#ifndef CG_OPENGL_PROJECT_RENDERING_HPP
#define CG_OPENGL_PROJECT_RENDERING_HPP

#include "graphics/camera.hpp"
#include "graphics/geometry.hpp"
#include "world/registry.hpp"

#include <glad/gl.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <memory>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace rendering {

struct GpuVertex {
    float px{};
    float py{};
    float pz{};
    float nx{};
    float ny{};
    float nz{};
    float u{};
    float v{};
};

inline constexpr auto kRenderFrameScratchName = "render.frame_scratch";

enum class RenderLayer : std::uint8_t {
    Sky = 0,
    Opaque = 20,
    AlphaTest = 40,
    Transparent = 60,
    Overlay = 100,
};

struct RenderMeshInstance {
    struct MeshBuffers {
        GLuint vao = 0U;
        GLuint vbo = 0U;
        GLuint ebo = 0U;

        ~MeshBuffers() {
            if (ebo != 0U) {
                glDeleteBuffers(1, &ebo);
            }
            if (vbo != 0U) {
                glDeleteBuffers(1, &vbo);
            }
            if (vao != 0U) {
                glDeleteVertexArrays(1, &vao);
            }
        }
    };

    std::shared_ptr<MeshBuffers> buffers{};
    GLsizei indexCount = 0;
    GLenum indexType = GL_UNSIGNED_INT;
    const void* indexOffset = nullptr;

    GLuint shaderProgram = 0;
    GLuint diffuseTexture = 0;
    std::shared_ptr<void> shaderLifetime{};
    GLint locMvp = -1;
    GLint locModel = -1;
    GLint locView = -1;
    GLint locProjection = -1;
    GLint locColor = -1;
    GLint locAlbedo = -1;

    glm::mat4 modelMatrix{1.0f};
    RenderLayer layer = RenderLayer::Opaque;

    AABB modelBounds{{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}};
    bool useFrustumCull = true;

    GLsizei drawArraysVertexCount = 0;
    GLenum drawArraysPrimitive = GL_LINES;
    glm::vec3 lineColorRgb{1.0f, 0.88f, 0.2f};
    bool visible = true;
};

struct RenderFrameScratch {
    struct DrawRef {
        const RenderMeshInstance* mesh = nullptr;
        float sortDepth = 0.0f;
    };

    std::vector<DrawRef> opaque;
    std::vector<DrawRef> transparent;

    void clear() {
        opaque.clear();
        transparent.clear();
    }
};

inline RenderFrameScratch* frameScratch(Registry& registry) {
    return registry.getObject<RenderFrameScratch>(kRenderFrameScratchName);
}

inline const RenderFrameScratch* frameScratch(const Registry& registry) {
    return registry.getObject<RenderFrameScratch>(kRenderFrameScratchName);
}

inline void setupSystem(Registry& registry) {
    if (registry.getObject<RenderFrameScratch>(kRenderFrameScratchName) != nullptr) {
        return;
    }
    registry.registerObject(kRenderFrameScratchName, RenderFrameScratch{});
}

inline bool boundsAreDegenerate(const AABB& bounds) {
    return bounds.min.x == bounds.max.x && bounds.min.y == bounds.max.y && bounds.min.z == bounds.max.z;
}

inline void prepareRenderStateSystem(Registry&) {
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_TRUE);
    glDisable(GL_CULL_FACE);
}

inline void gatherCullSortDrawablesSystem(Registry& registry) {
    auto* scratch = frameScratch(registry);
    if (scratch == nullptr) {
        return;
    }
    scratch->clear();

    const camera::Camera* cam = camera::activeCamera(registry);
    if (cam == nullptr) {
        return;
    }

    const glm::vec3 cameraPos = cam->position;

    for (const RenderMeshInstance* mesh : registry.getObjects<RenderMeshInstance>()) {
        if (mesh == nullptr || mesh->buffers == nullptr || mesh->buffers->vao == 0U || mesh->shaderProgram == 0U) {
            continue;
        }
        const bool drawsIndexed = mesh->indexCount > 0 && mesh->drawArraysVertexCount == 0;
        const bool drawsArrays = mesh->drawArraysVertexCount > 0;
        if ((!drawsIndexed && !drawsArrays) || !mesh->visible) {
            continue;
        }

        if (mesh->useFrustumCull && !boundsAreDegenerate(mesh->modelBounds)) {
            AABB worldBounds = mesh->modelBounds.transform(mesh->modelMatrix);
            const glm::vec3 extent = worldBounds.max - worldBounds.min;
            const float pad = glm::length(extent) * 0.08f + 8.0f;
            worldBounds.min -= glm::vec3(pad);
            worldBounds.max += glm::vec3(pad);
            if (!cam->frustum.intersectsWith(worldBounds)) {
                continue;
            }
        }

        const glm::vec4 worldOrigin = mesh->modelMatrix * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
        const glm::vec3 worldPos = glm::vec3(worldOrigin) / std::max(worldOrigin.w, 1e-6f);
        const float distance = glm::length(worldPos - cameraPos);

        const glm::vec4 clip = cam->viewProjMatrix * mesh->modelMatrix * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
        const float ndcZ = (std::fabs(clip.w) > 1e-6f) ? (clip.z / clip.w) : 0.0f;

        RenderFrameScratch::DrawRef ref{};
        ref.mesh = mesh;
        ref.sortDepth = ndcZ;
        if (mesh->layer >= RenderLayer::Transparent && mesh->layer < RenderLayer::Overlay) {
            ref.sortDepth = distance;
            scratch->transparent.push_back(ref);
        } else {
            scratch->opaque.push_back(ref);
        }
    }

    std::ranges::sort(scratch->opaque, [](const RenderFrameScratch::DrawRef& a, const RenderFrameScratch::DrawRef& b) {
        if (a.mesh->layer != b.mesh->layer) {
            return static_cast<int>(a.mesh->layer) < static_cast<int>(b.mesh->layer);
        }
        if (a.mesh->shaderProgram != b.mesh->shaderProgram) {
            return a.mesh->shaderProgram < b.mesh->shaderProgram;
        }
        if (a.mesh->buffers->vao != b.mesh->buffers->vao) {
            return a.mesh->buffers->vao < b.mesh->buffers->vao;
        }
        return a.sortDepth < b.sortDepth;
    });

    std::ranges::sort(scratch->transparent, [](const RenderFrameScratch::DrawRef& a, const RenderFrameScratch::DrawRef& b) {
        if (a.mesh->layer != b.mesh->layer) {
            return static_cast<int>(a.mesh->layer) < static_cast<int>(b.mesh->layer);
        }
        return a.sortDepth > b.sortDepth;
    });
}

inline void drawMeshList(const std::vector<RenderFrameScratch::DrawRef>& list, const camera::Camera& cam) {
    GLuint currentProgram = 0U;
    GLuint currentVao = 0U;
    GLuint currentTexture = 0U;

    for (const RenderFrameScratch::DrawRef& ref : list) {
        const RenderMeshInstance* mesh = ref.mesh;
        if (mesh == nullptr) {
            continue;
        }

        if (mesh->shaderProgram != currentProgram) {
            glUseProgram(mesh->shaderProgram);
            currentProgram = mesh->shaderProgram;
        }

        const glm::mat4 mvp = cam.viewProjMatrix * mesh->modelMatrix;
        if (mesh->locMvp >= 0) {
            glUniformMatrix4fv(mesh->locMvp, 1, GL_FALSE, glm::value_ptr(mvp));
        }
        if (mesh->locModel >= 0) {
            glUniformMatrix4fv(mesh->locModel, 1, GL_FALSE, glm::value_ptr(mesh->modelMatrix));
        }
        if (mesh->locView >= 0) {
            glUniformMatrix4fv(mesh->locView, 1, GL_FALSE, glm::value_ptr(cam.viewMatrix));
        }
        if (mesh->locProjection >= 0) {
            glUniformMatrix4fv(mesh->locProjection, 1, GL_FALSE, glm::value_ptr(cam.projMatrix));
        }

        if (mesh->buffers->vao != currentVao) {
            glBindVertexArray(mesh->buffers->vao);
            currentVao = mesh->buffers->vao;
        }

        if (mesh->drawArraysVertexCount > 0) {
            if (mesh->locColor >= 0) {
                glUniform3fv(mesh->locColor, 1, glm::value_ptr(mesh->lineColorRgb));
            }
            glDrawArrays(mesh->drawArraysPrimitive, 0, mesh->drawArraysVertexCount);
            continue;
        }

        if (mesh->diffuseTexture != currentTexture) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, mesh->diffuseTexture);
            currentTexture = mesh->diffuseTexture;
        }
        if (mesh->locAlbedo >= 0) {
            glUniform1i(mesh->locAlbedo, 0);
        }

        glDrawElements(GL_TRIANGLES, mesh->indexCount, mesh->indexType, mesh->indexOffset);
    }

    glBindVertexArray(0);
    glUseProgram(0);
}

inline void drawOpaqueMeshesSystem(Registry& registry) {
    const auto* scratch = frameScratch(registry);
    const camera::Camera* cam = camera::activeCamera(registry);
    if (scratch == nullptr || cam == nullptr || scratch->opaque.empty()) {
        return;
    }

    glDisable(GL_BLEND);
    drawMeshList(scratch->opaque, *cam);
}

inline void drawTransparentMeshesSystem(Registry& registry) {
    const auto* scratch = frameScratch(registry);
    const camera::Camera* cam = camera::activeCamera(registry);
    if (scratch == nullptr || cam == nullptr || scratch->transparent.empty()) {
        return;
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    drawMeshList(scratch->transparent, *cam);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

inline void endRenderStateSystem(Registry&) {
    glBindVertexArray(0);
    glUseProgram(0);
}

inline void renderFrameSystem(Registry& registry) {
    prepareRenderStateSystem(registry);
    gatherCullSortDrawablesSystem(registry);
    drawOpaqueMeshesSystem(registry);
    drawTransparentMeshesSystem(registry);
    endRenderStateSystem(registry);
}

}  // namespace rendering

#endif  // CG_OPENGL_PROJECT_RENDERING_HPP
