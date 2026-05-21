#ifndef CG_OPENGL_PROJECT_PP_HPP
#define CG_OPENGL_PROJECT_PP_HPP

// convolution Post-processing overlay: a screen-aligned quad drawn after the main scene.
// Press 'P' to cycle three modes:
//   0 = hidden (no overlay)
//   1 = Gaussian blur
//   2 = Laplacian (edge detection)

#include "asset/asset.hpp"
#include "asset/shader.hpp"
#include "graphics/fbo.hpp"
#include "graphics/screen_quad.hpp"
#include "graphics/window.hpp"
#include "log/log.hpp"
#include "world/registry.hpp"

#include <cstdint>
#include <memory>

#include <glad/gl.h>
#include <sys/types.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <glm/vec3.hpp>

namespace convolution {

inline constexpr auto kPostProcessingStateName = "postprocessing.state";
inline constexpr auto kKernelSize = 5;
inline constexpr auto kBlurIterations = 3;
inline constexpr std::string_view kVertexShaderPath = "assets/shaders/screen_quad.vert";
inline constexpr std::string_view kGaussianFragmentShaderPath = "assets/shaders/bloom_blur.frag";
inline constexpr std::string_view kBasicShaderPath = "assets/shaders/screen_quad.frag";
inline constexpr std::string_view kLaplacianFragmentShaderPath =
    "assets/shaders/laplacian_edge.frag";

enum class PostProcessingFilterMode : std::uint8_t {
    Hidden = 0,
    Gaussian = 1,
    Laplacian = 2,
};

struct ConvolutionFbos {
    graphics::SceneFbo sceneFbo;

    GLuint pingFbo = 0U;
    GLuint pingColor = 0U;
    GLuint pongFbo = 0U;
    GLuint pongColor = 0U;
    GLuint convolutionFbo = 0U;
    GLuint convolutionColor = 0U;
    int convolutionWidth = 0;
    int convolutionHeight = 0;

    ConvolutionFbos() = default;
    ConvolutionFbos(const ConvolutionFbos&) = delete;
    ConvolutionFbos& operator=(const ConvolutionFbos&) = delete;
    ConvolutionFbos(ConvolutionFbos&&) = delete;
    ConvolutionFbos& operator=(ConvolutionFbos&&) = delete;

    ~ConvolutionFbos() {
        if (convolutionFbo != 0U) {
            glDeleteFramebuffers(1, &convolutionFbo);
        }
        if (convolutionColor != 0U) {
            glDeleteTextures(1, &convolutionColor);
        }
    }
};

struct PostProcessingFilterState {
    PostProcessingFilterMode mode = PostProcessingFilterMode::Hidden;
    bool toggleWasPressed = false;

    GLint locBlurImage = -1;
    GLint locEdgeImage = -1;
    GLint locBasicImage = -1;
    GLint locBlurTexelSize = -1;
    GLint locEdgeTexelSize = -1;
    GLint locBlurHorizontal = -1;

    asset::ShaderProgram blurProgram{};
    asset::ShaderProgram edgeProgram{};
    asset::ShaderProgram basicProgram{};
    std::shared_ptr<graphics::ScreenQuad> quad{};
    std::shared_ptr<ConvolutionFbos> fbos{};
};

inline PostProcessingFilterMode nextMode(PostProcessingFilterMode mode) {
    switch (mode) {
        case PostProcessingFilterMode::Hidden:
            return PostProcessingFilterMode::Gaussian;
        case PostProcessingFilterMode::Gaussian:
            return PostProcessingFilterMode::Laplacian;
        case PostProcessingFilterMode::Laplacian:
        default:
            return PostProcessingFilterMode::Hidden;
    }
}

inline const char* modeName(PostProcessingFilterMode mode) {
    switch (mode) {
        case PostProcessingFilterMode::Hidden:
            return "hidden";
        case PostProcessingFilterMode::Gaussian:
            return "blurred";
        case PostProcessingFilterMode::Laplacian:
            return "edge detection";
    }
    return "unknown";
}

inline bool buildConvolutionFbo(ConvolutionFbos& fbos, int fbWidth, int fbHeight) {
    fbos.convolutionWidth = fbWidth;
    fbos.convolutionHeight = fbHeight;

    bool SceneFboBuildStatus = graphics::buildSceneFbo(fbos.sceneFbo, fbWidth, fbHeight);
    if (!SceneFboBuildStatus) {
        return false;
    }

    fbos.convolutionColor =
        graphics::createColorTexture(fbos.convolutionWidth, fbos.convolutionHeight);
    glGenFramebuffers(1, &fbos.convolutionFbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbos.convolutionFbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           fbos.convolutionColor, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        LOG_ERROR("Convolution FBO incomplete");
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return false;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return true;
}

inline bool ensureFbosForSize(PostProcessingFilterState& state, int fbWidth, int fbHeight) {
    if (fbWidth <= 0 || fbHeight <= 0) {
        return false;
    }
    if (state.fbos != nullptr && state.fbos->sceneFbo.width == fbWidth &&
        state.fbos->sceneFbo.height == fbHeight) {
        return true;
    }
    auto fresh = std::make_shared<ConvolutionFbos>();
    if (!buildConvolutionFbo(*fresh, fbWidth, fbHeight)) {
        return false;
    }
    state.fbos = std::move(fresh);
    return true;
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
    state.basicProgram = asset::AssetLoader::loadShaderProgram(std::string(kVertexShaderPath),
                                                               std::string(kBasicShaderPath));
    if (state.basicProgram.id == 0U) {
        LOG_WARN("Basic fragment shader failed to load; overlay disabled");
        return;
    }

    state.locBlurImage = glGetUniformLocation(state.blurProgram.id, "u_image");
    state.locEdgeImage = glGetUniformLocation(state.edgeProgram.id, "u_image");
    state.locBasicImage = glGetUniformLocation(state.basicProgram.id, "u_image");
    state.locBlurTexelSize = glGetUniformLocation(state.blurProgram.id, "u_texelSize");
    state.locEdgeTexelSize = glGetUniformLocation(state.edgeProgram.id, "u_texelSize");
    state.locBlurHorizontal = glGetUniformLocation(state.blurProgram.id, "u_horizontal");

    // build screen quad
    state.quad = std::make_shared<graphics::ScreenQuad>();
    graphics::buildScreenQuad(*state.quad);

    registry.registerObject(kPostProcessingStateName, std::move(state));
    LOG_INFO("Post processing overlay ready (press P to cycle: hidden -> blurred -> edge "
             "detection)");
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

inline void beginScenePassSystem(Registry& registry) {
    auto* state = registry.getObject<PostProcessingFilterState>(kPostProcessingStateName);
    const auto* windowState = registry.getObject<window::WindowState>(window::kMainWindowStateName);
    if (state == nullptr || state->mode == PostProcessingFilterMode::Hidden ||
        windowState == nullptr || windowState->handle == nullptr) {
        return;
    }

    int fbWidth = 0;
    int fbHeight = 0;
    graphics::getFramebufferSize(windowState->handle, fbWidth, fbHeight);
    if (!ensureFbosForSize(*state, fbWidth, fbHeight)) {
        return;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, state->fbos->sceneFbo.fbo);
    glViewport(0, 0, fbWidth, fbHeight);
    // Match window::clearWindowSystem so the bloom-on view doesn't show a
    // different background than bloom-off.
    glClearColor(0.15f, 0.18f, 0.22f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

inline void postProcessSystem(Registry& registry) {
    auto* state = registry.getObject<PostProcessingFilterState>(kPostProcessingStateName);

    const auto* windowState = registry.getObject<window::WindowState>(window::kMainWindowStateName);
    if (state == nullptr || state->mode == PostProcessingFilterMode::Hidden ||
        state->fbos == nullptr || state->quad == nullptr || windowState == nullptr ||
        windowState->handle == nullptr) {
        return;
    }
    if (state->blurProgram.id == 0U || state->edgeProgram.id == 0U) {
        return;
    }

    int fbWidth = 0;
    int fbHeight = 0;
    graphics::getFramebufferSize(windowState->handle, fbWidth, fbHeight);
    if (fbWidth <= 0 || fbHeight <= 0) {
        return;
    }

    const ConvolutionFbos& fbos = *state->fbos;

    const GLboolean depthTestWasEnabled = glIsEnabled(GL_DEPTH_TEST);
    GLboolean depthMaskWas = GL_TRUE;
    glGetBooleanv(GL_DEPTH_WRITEMASK, &depthMaskWas);
    const GLboolean blendWasEnabled = glIsEnabled(GL_BLEND);

    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glDisable(GL_BLEND);

    glBindFramebuffer(GL_FRAMEBUFFER, fbos.convolutionFbo);
    glViewport(0, 0, fbos.convolutionWidth, fbos.convolutionHeight);

    const float texelX = 1.0f / static_cast<float>(fbos.convolutionWidth);
    const float texelY = 1.0f / static_cast<float>(fbos.convolutionHeight);
    switch (state->mode) {
        case PostProcessingFilterMode::Gaussian:
            glUseProgram(state->blurProgram.id);
            if (state->locBlurImage >= 0) {
                glUniform1i(state->locBlurImage, 0);
            }
            if (state->locBlurTexelSize >= 0) {
                glUniform2f(state->locBlurTexelSize, texelX, texelY);
            }
            for (int i = 0; i < kBlurIterations; ++i) {
                glBindFramebuffer(GL_FRAMEBUFFER, fbos.pongFbo);
                glViewport(0, 0, fbos.convolutionWidth, fbos.convolutionHeight);
                glBindTexture(GL_TEXTURE_2D, fbos.pingColor);
                if (state->locBlurHorizontal >= 0) {
                    glUniform1i(state->locBlurHorizontal, 1);
                }
                graphics::drawScreenQuad(*state->quad);

                glBindFramebuffer(GL_FRAMEBUFFER, fbos.pingFbo);
                glBindTexture(GL_TEXTURE_2D, fbos.pongColor);
                if (state->locBlurHorizontal >= 0) {
                    glUniform1i(state->locBlurHorizontal, 0);
                }

                graphics::drawScreenQuad(*state->quad);
            }

            break;
        case PostProcessingFilterMode::Laplacian:
            glUseProgram(state->edgeProgram.id);
            if (state->locEdgeImage >= 0) {
                glUniform1i(state->locEdgeImage, 0);
            }
            if (state->locEdgeTexelSize >= 0) {
                glUniform2f(state->locEdgeTexelSize, texelX, texelY);
            }
            break;
        case PostProcessingFilterMode::Hidden:
            return;
    }

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, fbos.sceneFbo.color);

    graphics::drawScreenQuad(*state->quad);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, fbWidth, fbHeight);

    glUseProgram(state->basicProgram.id);
    if (state->locBasicImage >= 0) {
        glUniform1i(state->locBasicImage, 0);
    }

    if (state->mode == PostProcessingFilterMode::Laplacian) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, fbos.convolutionColor);
        graphics::drawScreenQuad(*state->quad);
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(0);

    if (depthTestWasEnabled == GL_TRUE) {
        glEnable(GL_DEPTH_TEST);
    }
    glDepthMask(depthMaskWas);
    if (blendWasEnabled == GL_TRUE) {
        glEnable(GL_BLEND);
    }
}
}  // namespace convolution

#endif  // CG_OPENGL_PROJECT_PP_HPP
