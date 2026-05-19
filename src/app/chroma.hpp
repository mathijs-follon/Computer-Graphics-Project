#ifndef CG_OPENGL_PROJECT_CHROMA_HPP
#define CG_OPENGL_PROJECT_CHROMA_HPP

// Chroma-keying overlay: a screen-aligned quad drawn after the main scene.
// Press 'L' to cycle three modes:
//   0 = hidden (no overlay)
//   1 = raw image overlay (frame texture shown over the scene, green still visible)
//   2 = chroma-keyed overlay (green discarded in YCbCr space, scene shows through)

#include "app/app.hpp"
#include "asset/asset.hpp"
#include "graphics/window.hpp"
#include "log/log.hpp"
#include "world/registry.hpp"

#include <array>
#include <cstdint>
#include <memory>

#include <glad/gl.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <glm/vec3.hpp>

namespace chroma {

inline constexpr auto kChromaStateName = "chroma.state";
inline constexpr std::string_view kVertexShaderPath = "assets/shaders/chroma.vert";
inline constexpr std::string_view kFragmentShaderPath = "assets/shaders/chroma.frag";
inline constexpr std::string_view kTexturePath = "assets/textures/chroma_key.jpg";

enum class OverlayMode : std::uint8_t {
    Hidden = 0,
    RawOverlay = 1,
    ChromaKeyed = 2,
};

struct GpuResources {
    GLuint vao = 0U;
    GLuint vbo = 0U;
    GLuint ebo = 0U;

    GpuResources() = default;
    GpuResources(const GpuResources&) = delete;
    GpuResources& operator=(const GpuResources&) = delete;
    GpuResources(GpuResources&&) = delete;
    GpuResources& operator=(GpuResources&&) = delete;

    ~GpuResources() {
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

struct ChromaState {
    OverlayMode mode = OverlayMode::Hidden;
    bool toggleWasPressed = false;

    asset::ShaderProgram program{};
    asset::Texture texture{};
    std::shared_ptr<GpuResources> gpu{};

    GLint locAlbedo = -1;
    GLint locEnableKey = -1;
    GLint locKeyColor = -1;
    GLint locKeyThreshold = -1;
    GLint locKeySoftness = -1;

    // Chroma key tuning: green key, distance threshold in normalized [0,1] CbCr plane.
    glm::vec3 keyColor{0.0f, 1.0f, 0.0f};
    float keyThreshold = 0.18f;
    float keySoftness = 0.08f;
};

inline OverlayMode nextMode(OverlayMode mode) {
    switch (mode) {
        case OverlayMode::Hidden:
            return OverlayMode::RawOverlay;
        case OverlayMode::RawOverlay:
            return OverlayMode::ChromaKeyed;
        case OverlayMode::ChromaKeyed:
        default:
            return OverlayMode::Hidden;
    }
}

inline const char* modeName(OverlayMode mode) {
    switch (mode) {
        case OverlayMode::Hidden:
            return "hidden";
        case OverlayMode::RawOverlay:
            return "raw overlay";
        case OverlayMode::ChromaKeyed:
            return "chroma-keyed";
    }
    return "unknown";
}

// Fullscreen quad in NDC: 4 verts (x, y, u, v). Origin at center, covers [-1, 1].
inline void buildScreenQuad(GpuResources& gpu) {
    // UV v is flipped (1 - v) because stb_image is loaded with vertical flip enabled,
    // and we want the image to appear right-side-up on screen.
    constexpr std::array<float, 16> kVertices = {
        -1.0f, -1.0f, 0.0f, 0.0f,
         1.0f, -1.0f, 1.0f, 0.0f,
         1.0f,  1.0f, 1.0f, 1.0f,
        -1.0f,  1.0f, 0.0f, 1.0f,
    };
    constexpr std::array<std::uint32_t, 6> kIndices = {0, 1, 2, 0, 2, 3};

    glGenVertexArrays(1, &gpu.vao);
    glGenBuffers(1, &gpu.vbo);
    glGenBuffers(1, &gpu.ebo);

    glBindVertexArray(gpu.vao);

    glBindBuffer(GL_ARRAY_BUFFER, gpu.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(kVertices), kVertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gpu.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(kIndices), kIndices.data(), GL_STATIC_DRAW);

    constexpr GLsizei kStride = 4 * sizeof(float);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, kStride, reinterpret_cast<void*>(0));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, kStride,
                          reinterpret_cast<void*>(2 * sizeof(float)));

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

inline void setupSystem(Registry& registry) {
    if (registry.getObject<ChromaState>(kChromaStateName) != nullptr) {
        return;
    }

    ChromaState state{};
    state.program =
        asset::AssetLoader::loadShaderProgram(std::string(kVertexShaderPath),
                                              std::string(kFragmentShaderPath));
    if (state.program.id == 0U) {
        LOG_WARN("Chroma overlay shader failed to load; overlay disabled");
        return;
    }

    state.texture = asset::AssetLoader::loadTexture(std::string(kTexturePath));
    if (state.texture.textureHdl == 0U) {
        LOG_WARN("Chroma overlay texture failed to load; overlay disabled");
        return;
    }

    state.locAlbedo = glGetUniformLocation(state.program.id, "u_albedo");
    state.locEnableKey = glGetUniformLocation(state.program.id, "u_enableKey");
    state.locKeyColor = glGetUniformLocation(state.program.id, "u_keyColor");
    state.locKeyThreshold = glGetUniformLocation(state.program.id, "u_keyThreshold");
    state.locKeySoftness = glGetUniformLocation(state.program.id, "u_keySoftness");

    state.gpu = std::make_shared<GpuResources>();
    buildScreenQuad(*state.gpu);

    registry.registerObject(kChromaStateName, std::move(state));
    LOG_INFO("Chroma overlay ready (press L to cycle: hidden -> overlay -> chroma-keyed)");
}

inline void inputSystem(Registry& registry) {
    auto* state = registry.getObject<ChromaState>(kChromaStateName);
    const auto* windowState =
        registry.getObject<window::WindowState>(window::kMainWindowStateName);
    if (state == nullptr || windowState == nullptr || windowState->handle == nullptr) {
        return;
    }

    const bool pressed = glfwGetKey(windowState->handle, GLFW_KEY_L) == GLFW_PRESS;
    if (pressed && !state->toggleWasPressed) {
        state->mode = nextMode(state->mode);
        LOG_INFO("Chroma overlay: {}", modeName(state->mode));
    }
    state->toggleWasPressed = pressed;
}

inline void renderSystem(Registry& registry) {
    const auto* state = registry.getObject<ChromaState>(kChromaStateName);
    if (state == nullptr || state->mode == OverlayMode::Hidden) {
        return;
    }
    if (state->program.id == 0U || state->gpu == nullptr || state->gpu->vao == 0U ||
        state->texture.textureHdl == 0U) {
        return;
    }

    // Draw on top of everything: disable depth test/write so the quad always wins,
    // and use alpha blending so chroma-keyed soft edges blend with the scene.
    GLboolean depthMask = GL_TRUE;
    glGetBooleanv(GL_DEPTH_WRITEMASK, &depthMask);
    const GLboolean depthTestWasEnabled = glIsEnabled(GL_DEPTH_TEST);
    const GLboolean blendWasEnabled = glIsEnabled(GL_BLEND);

    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glUseProgram(state->program.id);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, state->texture.textureHdl);
    if (state->locAlbedo >= 0) {
        glUniform1i(state->locAlbedo, 0);
    }
    if (state->locEnableKey >= 0) {
        glUniform1i(state->locEnableKey, state->mode == OverlayMode::ChromaKeyed ? 1 : 0);
    }
    if (state->locKeyColor >= 0) {
        glUniform3f(state->locKeyColor, state->keyColor.r, state->keyColor.g, state->keyColor.b);
    }
    if (state->locKeyThreshold >= 0) {
        glUniform1f(state->locKeyThreshold, state->keyThreshold);
    }
    if (state->locKeySoftness >= 0) {
        glUniform1f(state->locKeySoftness, state->keySoftness);
    }

    glBindVertexArray(state->gpu->vao);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);

    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(0);

    if (depthTestWasEnabled == GL_TRUE) {
        glEnable(GL_DEPTH_TEST);
    }
    glDepthMask(depthMask);
    if (blendWasEnabled == GL_FALSE) {
        glDisable(GL_BLEND);
    }
}

}  // namespace chroma

#endif  // CG_OPENGL_PROJECT_CHROMA_HPP
