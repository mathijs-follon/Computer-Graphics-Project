#ifndef CG_OPENGL_PROJECT_PP_HPP
#define CG_OPENGL_PROJECT_PP_HPP

// Post-processing overlay: a screen-aligned quad drawn after the main scene.
// Press 'P' to cycle three modes:
//   0 = hidden (no overlay)
//   1 = Gaussian blur
//   2 = Laplacian (edge detection)

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

namespace post_processing {

inline constexpr auto kPostProcessingStateName = "postprocessing.state";
inline constexpr std::string_view kVertexShaderPath = "assets/shaders/chroma.vert";
inline constexpr std::string_view kGaussianFragmentShaderPath = "assets/shaders/gaussian.frag";
inline constexpr std::string_view kLaplacianFragmentShaderPath = "assets/shaders/laplacian.frag";

enum class OverlayMode : std::uint8_t {
    Hidden = 0,
    Gaussian = 1,
    Laplacian = 2,
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

struct PostProcessingFilterState {
    OverlayMode mode = OverlayMode::Hidden;
    bool toggleWasPressed = false;

    asset::ShaderProgram blurProgram{};
    asset::ShaderProgram edgeProgram{};
    std::shared_ptr<GpuResources> gpu{};

    GLuint FBO = 0U;
    GLuint FBOTexture = 0U;
};

inline OverlayMode nextMode(OverlayMode mode) {
    switch (mode) {
        case OverlayMode::Hidden:
            return OverlayMode::Gaussian;
        case OverlayMode::Gaussian:
            return OverlayMode::Laplacian;
        case OverlayMode::Laplacian:
        default:
            return OverlayMode::Hidden;
    }
}

inline const char* modeName(OverlayMode mode) {
    switch (mode) {
        case OverlayMode::Hidden:
            return "hidden";
        case OverlayMode::Gaussian:
            return "blurred";
        case OverlayMode::Laplacian:
            return "edge detection";
    }
    return "unknown";
}

// Fullscreen quad in NDC: 4 verts (x, y, u, v). Origin at center, covers [-1, 1].
inline void buildScreenQuad(GpuResources& gpu) {
    // UV v is flipped (1 - v) because stb_image is loaded with vertical flip enabled,
    // and we want the image to appear right-side-up on screen.
    constexpr std::array<float, 16> kVertices = {
        -1.0f, -1.0f, 0.0f, 0.0f, 1.0f,  -1.0f, 1.0f, 0.0f,
        1.0f,  1.0f,  1.0f, 1.0f, -1.0f, 1.0f,  0.0f, 1.0f,
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
    // ignore if setup already
    if (registry.getObject<PostProcessingFilterState>(kPostProcessingStateName) != nullptr) {
        return;
    }

    // setup state
    PostProcessingFilterState state{};
    state.blurProgram = asset::AssetLoader::loadShaderProgram(
        std::string(kVertexShaderPath), std::string(kGaussianFragmentShaderPath));
    if (state.blurProgram.id == 0U) {
        LOG_WARN("Gaussian overlay shader failed to load; overlay disabled");
        return;
    }
    state.edgeProgram = asset::AssetLoader::loadShaderProgram(
        std::string(kVertexShaderPath), std::string(kLaplacianFragmentShaderPath));
    if (state.edgeProgram.id == 0U) {
        LOG_WARN("Laplacian overlay shader failed to load; overlay disabled");
        return;
    }

    // build screen quad
    state.gpu = std::make_shared<GpuResources>();
    buildScreenQuad(*state.gpu);

    // setup FBO & output texture
    glGenFramebuffers(1, &state.FBO);
    glBindFramebuffer(GL_FRAMEBUFFER, state.FBO);

    glGenTextures(1, &state.FBOTexture);
    glBindTexture(GL_TEXTURE_2D, state.FBOTexture);

    // fetch screen dimensions

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, fbo_width, fbo_height, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                 nullptr);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    // Gebruik een kernel die resulteert in het blurren van het beeld (bijvoorbeeld aan de hand van
    // een Gaussian Blur) en een kernel die de randen highlight (bijvoorbeeld een Laplaciaan of
    // Sobel kernel). Render hiervoor je scene uit naar een framebuffer object (FBO). Gebruik
    // vervolgens de kleurenbuffer van deze FBO als textuur en de opgestelde shader om een rechthoek
    // die uitgelijnd is met de camera, en het scherm volledig vult (een screen aligned quad), uit
    // te tekenen

    registry.registerObject(kPostProcessingStateName, std::move(state));
    LOG_INFO(
        "Post processing overlay ready (press P to cycle: hidden -> blurred -> edge detection)");
}

inline void inputSystem(Registry& registry) {
    auto* state = registry.getObject<PostProcessingFilterState>(kPostProcessingStateName);
    const auto* windowState = registry.getObject<window::WindowState>(window::kMainWindowStateName);
    if (state == nullptr || windowState == nullptr || windowState->handle == nullptr) {
        return;
    }

    const bool pressed = glfwGetKey(windowState->handle, GLFW_KEY_P) == GLFW_PRESS;
    if (pressed && !state->toggleWasPressed) {
        state->mode = nextMode(state->mode);
        LOG_INFO("Post processing overlay: {}", modeName(state->mode));
    }
    state->toggleWasPressed = pressed;
}

inline void renderSystem(Registry& registry) {
    const auto* state = registry.getObject<PostProcessingFilterState>(kPostProcessingStateName);
    if (state == nullptr || state->mode == OverlayMode::Hidden) {
        return;
    }
    if (state->blurProgram.id == 0U || state->edgeProgram.id == 0U || state->gpu == nullptr ||
        state->gpu->vao == 0U) {
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

    switch (state->mode) {
        case OverlayMode::Gaussian:
            glUseProgram(state->blurProgram.id);
            break;
        case OverlayMode::Laplacian:
            glUseProgram(state->edgeProgram.id);
            break;
        case OverlayMode::Hidden:
            return;
    }
    glActiveTexture(GL_TEXTURE0);
    //    glBindTexture(GL_TEXTURE_2D, state->texture.textureHdl);

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

}  // namespace post_processing

#endif  // CG_OPENGL_PROJECT_PP_HPP
