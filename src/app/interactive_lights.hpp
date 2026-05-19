#ifndef CG_OPENGL_PROJECT_INTERACTIVE_LIGHTS_HPP
#define CG_OPENGL_PROJECT_INTERACTIVE_LIGHTS_HPP


#include "app/app.hpp"
#include "asset/asset.hpp"
#include "graphics/camera.hpp"
#include "graphics/rendering.hpp"
#include "graphics/window.hpp"
#include "log/log.hpp"
#include "world/registry.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include <glad/gl.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace interactive_lights {

inline constexpr auto kStateName = "interactive_lights.state";
inline constexpr std::string_view kVertexShaderPath = "assets/shaders/default.vert";
inline constexpr std::string_view kEmissiveFragPath = "assets/shaders/emissive.frag";

inline constexpr int kSphereCount = 5;
inline constexpr float kSphereRadius = 25.0f;
inline constexpr int kSphereRings = 20;
inline constexpr int kSphereSegments = 20;

inline constexpr float kEmissiveBoost = 3.0f;
inline constexpr float kSphereAlpha = 0.55f;

inline constexpr std::array<glm::vec3, 7> kPalette = {
    glm::vec3{1.0f, 1.0f, 1.0f},   // white
    glm::vec3{1.0f, 0.15f, 0.15f}, // red
    glm::vec3{1.0f, 0.55f, 0.10f}, // orange
    glm::vec3{1.0f, 1.0f, 0.20f},  // yellow
    glm::vec3{0.20f, 1.0f, 0.30f}, // green
    glm::vec3{0.20f, 0.7f, 1.0f},  // cyan/blue
    glm::vec3{1.0f, 0.25f, 1.0f},  // magenta
};

inline constexpr int kFirstLightSlot = 1;

struct Sphere {
    glm::vec3 position{0.0f};
    int lightSlot = 0;
    int colorIndex = 0;
};

struct SphereMeshGpu {
    GLuint vao = 0U;
    GLuint vbo = 0U;
    GLuint ebo = 0U;
    GLsizei indexCount = 0;

    SphereMeshGpu() = default;
    SphereMeshGpu(const SphereMeshGpu&) = delete;
    SphereMeshGpu& operator=(const SphereMeshGpu&) = delete;
    SphereMeshGpu(SphereMeshGpu&&) = delete;
    SphereMeshGpu& operator=(SphereMeshGpu&&) = delete;

    ~SphereMeshGpu() {
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

struct State {
    std::vector<Sphere> spheres{};

    asset::ShaderProgram emissiveProgram{};
    GLint locMvp = -1;
    GLint locModel = -1;
    GLint locColor = -1;
    GLint locAlpha = -1;

    std::shared_ptr<SphereMeshGpu> mesh{};

    bool leftClickWasPressed = false;
};

inline void generateUnitSphere(std::vector<rendering::GpuVertex>& outVertices,
                               std::vector<std::uint32_t>& outIndices) {
    outVertices.clear();
    outIndices.clear();
    outVertices.reserve(static_cast<std::size_t>((kSphereRings + 1) * (kSphereSegments + 1)));
    outIndices.reserve(static_cast<std::size_t>(kSphereRings * kSphereSegments * 6));

    constexpr float kPi = 3.14159265358979323846f;

    for (int i = 0; i <= kSphereRings; ++i) {
        const float phi = kPi * static_cast<float>(i) / static_cast<float>(kSphereRings);
        const float sinPhi = std::sin(phi);
        const float cosPhi = std::cos(phi);
        for (int j = 0; j <= kSphereSegments; ++j) {
            const float theta =
                2.0f * kPi * static_cast<float>(j) / static_cast<float>(kSphereSegments);
            const float x = sinPhi * std::cos(theta);
            const float y = cosPhi;
            const float z = sinPhi * std::sin(theta);
            outVertices.push_back(rendering::GpuVertex{
                .px = x,
                .py = y,
                .pz = z,
                .nx = x,
                .ny = y,
                .nz = z,
                .u = static_cast<float>(j) / static_cast<float>(kSphereSegments),
                .v = static_cast<float>(i) / static_cast<float>(kSphereRings),
            });
        }
    }

    for (int i = 0; i < kSphereRings; ++i) {
        for (int j = 0; j < kSphereSegments; ++j) {
            const std::uint32_t a =
                static_cast<std::uint32_t>(i * (kSphereSegments + 1) + j);
            const std::uint32_t b = a + static_cast<std::uint32_t>(kSphereSegments + 1);
            outIndices.push_back(a);
            outIndices.push_back(b);
            outIndices.push_back(a + 1);
            outIndices.push_back(b);
            outIndices.push_back(b + 1);
            outIndices.push_back(a + 1);
        }
    }
}

inline void uploadSphereMesh(SphereMeshGpu& gpu) {
    std::vector<rendering::GpuVertex> vertices;
    std::vector<std::uint32_t> indices;
    generateUnitSphere(vertices, indices);

    glGenVertexArrays(1, &gpu.vao);
    glGenBuffers(1, &gpu.vbo);
    glGenBuffers(1, &gpu.ebo);

    glBindVertexArray(gpu.vao);

    glBindBuffer(GL_ARRAY_BUFFER, gpu.vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(vertices.size() * sizeof(rendering::GpuVertex)),
                 vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gpu.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(indices.size() * sizeof(std::uint32_t)), indices.data(),
                 GL_STATIC_DRAW);

    // Layout matches rendering::GpuVertex / default.vert: position, normal, UV.
    constexpr GLsizei kStride = sizeof(rendering::GpuVertex);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, kStride,
                          reinterpret_cast<const void*>(offsetof(rendering::GpuVertex, px)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, kStride,
                          reinterpret_cast<const void*>(offsetof(rendering::GpuVertex, nx)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, kStride,
                          reinterpret_cast<const void*>(offsetof(rendering::GpuVertex, u)));

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    gpu.indexCount = static_cast<GLsizei>(indices.size());
}

inline std::array<glm::vec3, kSphereCount> spherePositions() {
    return {
        glm::vec3{50.0f, -150.0f, 100.0f},      // near the slide entry, elevated
        glm::vec3{-200.0f, -260.0f, 250.0f},
        glm::vec3{-300.0f, -250.0f, -150.0f}, 
        glm::vec3{100.0f, -350.0f, -450.0f}, 
        glm::vec3{400.0f, -380.0f, -300.0f},
    };
}

inline void writeLightSlot(const Registry& registry, int slot, const glm::vec3& position,
                           const glm::vec3& color, bool on) {
    for (const asset::ShaderProgram* program : registry.getObjects<asset::ShaderProgram>()) {
        if (program == nullptr || program->resource == nullptr || program->id == 0U) {
            continue;
        }
        if (slot < 0 || slot >= asset::ShaderProgram::MAX_LIGHT_COUNT) {
            continue;
        }
        const auto& slotLocs = program->resource->LightResources[slot];
        if (slotLocs.locActive < 0) {
            continue;
        }

        glUseProgram(program->id);
        glUniform1i(slotLocs.locActive, on ? GL_TRUE : GL_FALSE);
        glUniform3fv(slotLocs.locPos, 1, glm::value_ptr(position));
        glUniform3fv(slotLocs.locColor, 1, glm::value_ptr(color));
        if (slotLocs.locLinAtt >= 0) {
            glUniform1f(slotLocs.locLinAtt, 0.001f);
        }
        if (slotLocs.locQuadAtt >= 0) {
            glUniform1f(slotLocs.locQuadAtt, 0.0001f);
        }
        if (slotLocs.locAmbient >= 0) {
            glUniform1f(slotLocs.locAmbient, 0.10f);
        }
        if (slotLocs.locDiffuse >= 0) {
            glUniform1f(slotLocs.locDiffuse, 0.50f);
        }
        if (slotLocs.locSpecular >= 0) {
            glUniform1f(slotLocs.locSpecular, 0.40f);
        }
    }
    glUseProgram(0);
}

inline void setupSystem(Registry& registry) {
    if (registry.getObject<State>(kStateName) != nullptr) {
        return;
    }

    State state{};
    state.emissiveProgram = asset::AssetLoader::loadShaderProgram(std::string(kVertexShaderPath),
                                                                  std::string(kEmissiveFragPath));
    if (state.emissiveProgram.id == 0U) {
        LOG_WARN("Interactive lights emissive shader failed to load; feature disabled");
        return;
    }
    state.locMvp = state.emissiveProgram.resource->locMvp;
    state.locModel = state.emissiveProgram.resource->locModel;
    state.locColor = state.emissiveProgram.resource->locColor;
    state.locAlpha = glGetUniformLocation(state.emissiveProgram.id, "u_alpha");

    state.mesh = std::make_shared<SphereMeshGpu>();
    uploadSphereMesh(*state.mesh);

    const auto positions = spherePositions();
    state.spheres.reserve(positions.size());
    for (std::size_t i = 0; i < positions.size(); ++i) {
        Sphere sphere{};
        sphere.position = positions[i];
        sphere.lightSlot = kFirstLightSlot + static_cast<int>(i);
        sphere.colorIndex = 0;  // start at white
        state.spheres.push_back(sphere);
    }

    for (const Sphere& sphere : state.spheres) {
        writeLightSlot(registry, sphere.lightSlot, sphere.position, kPalette[sphere.colorIndex],
                       true);
    }

    registry.registerObject(kStateName, std::move(state));
    LOG_INFO(
        "Interactive light spheres ready ({} spheres, left-click to cycle color)", kSphereCount);
}

inline std::optional<float> intersectRaySphere(const glm::vec3& origin, const glm::vec3& dir,
                                               const glm::vec3& sphereCenter,
                                               float sphereRadius) {
    const glm::vec3 L = origin - sphereCenter;
    const float b = 2.0f * glm::dot(L, dir);
    const float c = glm::dot(L, L) - sphereRadius * sphereRadius;
    const float discriminant = b * b - 4.0f * c;
    if (discriminant < 0.0f) {
        return std::nullopt;
    }
    const float sq = std::sqrt(discriminant);
    const float t1 = (-b - sq) * 0.5f;
    if (t1 > 0.0f) {
        return t1;
    }
    const float t2 = (-b + sq) * 0.5f;
    if (t2 > 0.0f) {
        return t2;
    }
    return std::nullopt;
}

inline std::optional<int> pickSphere(const State& state, const camera::Camera& cam) {
    int bestIndex = -1;
    float bestT = std::numeric_limits<float>::infinity();
    const glm::vec3 dir = glm::normalize(cam.front);

    for (std::size_t i = 0; i < state.spheres.size(); ++i) {
        const Sphere& sphere = state.spheres[i];
        if (const auto t = intersectRaySphere(cam.position, dir, sphere.position, kSphereRadius);
            t.has_value() && *t < bestT && *t <= cam.farZ) {
            bestT = *t;
            bestIndex = static_cast<int>(i);
        }
    }
    if (bestIndex < 0) {
        return std::nullopt;
    }
    return bestIndex;
}

inline void inputSystem(Registry& registry) {
    auto* state = registry.getObject<State>(kStateName);
    const auto* windowState = registry.getObject<window::WindowState>(window::kMainWindowStateName);
    const auto* cameraState = registry.getObject<camera::CameraState>(camera::kCameraStateName);
    if (state == nullptr || windowState == nullptr || windowState->handle == nullptr ||
        cameraState == nullptr) {
        return;
    }

    const bool leftClickPressed =
        glfwGetMouseButton(windowState->handle, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    const bool justClicked = leftClickPressed && !state->leftClickWasPressed;
    state->leftClickWasPressed = leftClickPressed;

    if (!justClicked || !cameraState->cursorCaptured) {
        return;
    }

    const camera::Camera* cam = camera::activeCamera(registry);
    if (cam == nullptr) {
        return;
    }

    const auto hitOpt = pickSphere(*state, *cam);
    if (!hitOpt.has_value()) {
        return;
    }

    Sphere& sphere = state->spheres[static_cast<std::size_t>(*hitOpt)];
    sphere.colorIndex = (sphere.colorIndex + 1) % static_cast<int>(kPalette.size());
    const glm::vec3 newColor = kPalette[sphere.colorIndex];
    writeLightSlot(registry, sphere.lightSlot, sphere.position, newColor, true);
    LOG_INFO("Cycled sphere {} (slot {}) to color ({:.2f}, {:.2f}, {:.2f})", *hitOpt,
             sphere.lightSlot, newColor.r, newColor.g, newColor.b);
}

inline void renderSystem(Registry& registry) {
    const auto* state = registry.getObject<State>(kStateName);
    if (state == nullptr || state->emissiveProgram.id == 0U || state->mesh == nullptr ||
        state->mesh->vao == 0U) {
        return;
    }
    const camera::Camera* cam = camera::activeCamera(registry);
    if (cam == nullptr) {
        return;
    }

    const GLboolean blendWasEnabled = glIsEnabled(GL_BLEND);
    GLboolean depthMaskWas = GL_TRUE;
    glGetBooleanv(GL_DEPTH_WRITEMASK, &depthMaskWas);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    glUseProgram(state->emissiveProgram.id);
    glBindVertexArray(state->mesh->vao);

    if (state->locAlpha >= 0) {
        glUniform1f(state->locAlpha, kSphereAlpha);
    }

    std::vector<Sphere> drawOrder = state->spheres;
    std::ranges::sort(drawOrder, [&cam](const Sphere& a, const Sphere& b) {
        const float da = glm::length(a.position - cam->position);
        const float db = glm::length(b.position - cam->position);
        return da > db;
    });

    for (const Sphere& sphere : drawOrder) {
        const glm::mat4 model = glm::translate(glm::mat4(1.0f), sphere.position) *
                                glm::scale(glm::mat4(1.0f), glm::vec3(kSphereRadius));
        const glm::mat4 mvp = cam->viewProjMatrix * model;
        if (state->locMvp >= 0) {
            glUniformMatrix4fv(state->locMvp, 1, GL_FALSE, glm::value_ptr(mvp));
        }
        if (state->locModel >= 0) {
            glUniformMatrix4fv(state->locModel, 1, GL_FALSE, glm::value_ptr(model));
        }
        if (state->locColor >= 0) {
            const glm::vec3 emissive = kPalette[sphere.colorIndex] * kEmissiveBoost;
            glUniform3fv(state->locColor, 1, glm::value_ptr(emissive));
        }
        glDrawElements(GL_TRIANGLES, state->mesh->indexCount, GL_UNSIGNED_INT, nullptr);
    }

    glBindVertexArray(0);
    glUseProgram(0);

    glDepthMask(depthMaskWas);
    if (blendWasEnabled == GL_FALSE) {
        glDisable(GL_BLEND);
    }
}

}  // namespace interactive_lights

#endif  // CG_OPENGL_PROJECT_INTERACTIVE_LIGHTS_HPP
