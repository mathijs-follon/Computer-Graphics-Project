#ifndef CG_OPENGL_PROJECT_BLOOM_HPP
#define CG_OPENGL_PROJECT_BLOOM_HPP

// Bloom post-processing. Press 'B' to toggle.

#include "app/app.hpp"
#include "asset/asset.hpp"
#include "graphics/screen_quad.hpp"
#include "graphics/window.hpp"
#include "log/log.hpp"
#include "world/registry.hpp"

#include <memory>

#include <glad/gl.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

namespace bloom {

inline constexpr auto kBloomStateName = "bloom.state";
inline constexpr std::string_view kVertexShaderPath = "assets/shaders/screen_quad.vert";
inline constexpr std::string_view kBrightFragPath = "assets/shaders/bloom_bright.frag";
inline constexpr std::string_view kBlurFragPath = "assets/shaders/bloom_blur.frag";
inline constexpr std::string_view kCompositeFragPath = "assets/shaders/bloom_composite.frag";

inline constexpr float kBloomBufferScale = 0.5f;
inline constexpr int kBlurIterations = 5;

struct FboSet {
    GLuint sceneFbo = 0U;
    GLuint sceneColor = 0U;
    GLuint sceneDepth = 0U;
    int sceneWidth = 0;
    int sceneHeight = 0;

    GLuint pingFbo = 0U;
    GLuint pingColor = 0U;
    GLuint pongFbo = 0U;
    GLuint pongColor = 0U;
    int bloomWidth = 0;
    int bloomHeight = 0;

    FboSet() = default;
    FboSet(const FboSet&) = delete;
    FboSet& operator=(const FboSet&) = delete;
    FboSet(FboSet&&) = delete;
    FboSet& operator=(FboSet&&) = delete;

    ~FboSet() {
        if (sceneFbo != 0U) {
            glDeleteFramebuffers(1, &sceneFbo);
        }
        if (pingFbo != 0U) {
            glDeleteFramebuffers(1, &pingFbo);
        }
        if (pongFbo != 0U) {
            glDeleteFramebuffers(1, &pongFbo);
        }
        if (sceneColor != 0U) {
            glDeleteTextures(1, &sceneColor);
        }
        if (pingColor != 0U) {
            glDeleteTextures(1, &pingColor);
        }
        if (pongColor != 0U) {
            glDeleteTextures(1, &pongColor);
        }
        if (sceneDepth != 0U) {
            glDeleteRenderbuffers(1, &sceneDepth);
        }
    }
};

struct BloomState {
    bool enabled = false;
    bool toggleWasPressed = false;

    float threshold = 0.5f;
    float intensity = 1.2f;

    asset::ShaderProgram brightProgram{};
    asset::ShaderProgram blurProgram{};
    asset::ShaderProgram compositeProgram{};

    GLint locBrightScene = -1;
    GLint locBrightThreshold = -1;

    GLint locBlurImage = -1;
    GLint locBlurTexelSize = -1;
    GLint locBlurHorizontal = -1;

    GLint locCompositeScene = -1;
    GLint locCompositeBloom = -1;
    GLint locCompositeIntensity = -1;

    std::shared_ptr<graphics::ScreenQuad> quad{};
    std::shared_ptr<FboSet> fbos{};
};

inline GLuint createColorTexture(int width, int height) {
    GLuint texture = 0U;
    glGenTextures(1, &texture);
    if (texture == 0U) {
        return 0U;
    }
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);
    return texture;
}

inline bool buildFboSet(FboSet& fbos, int fbWidth, int fbHeight) {
    fbos.sceneWidth = fbWidth;
    fbos.sceneHeight = fbHeight;
    fbos.bloomWidth = std::max(1, static_cast<int>(static_cast<float>(fbWidth) * kBloomBufferScale));
    fbos.bloomHeight =
        std::max(1, static_cast<int>(static_cast<float>(fbHeight) * kBloomBufferScale));

    fbos.sceneColor = createColorTexture(fbWidth, fbHeight);
    glGenRenderbuffers(1, &fbos.sceneDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, fbos.sceneDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, fbWidth, fbHeight);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    glGenFramebuffers(1, &fbos.sceneFbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbos.sceneFbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fbos.sceneColor, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, fbos.sceneDepth);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        LOG_ERROR("Bloom scene FBO incomplete");
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return false;
    }

    fbos.pingColor = createColorTexture(fbos.bloomWidth, fbos.bloomHeight);
    glGenFramebuffers(1, &fbos.pingFbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbos.pingFbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fbos.pingColor, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        LOG_ERROR("Bloom ping FBO incomplete");
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return false;
    }

    fbos.pongColor = createColorTexture(fbos.bloomWidth, fbos.bloomHeight);
    glGenFramebuffers(1, &fbos.pongFbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbos.pongFbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fbos.pongColor, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        LOG_ERROR("Bloom pong FBO incomplete");
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return false;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return true;
}

inline bool ensureFbosForSize(BloomState& state, int fbWidth, int fbHeight) {
    if (fbWidth <= 0 || fbHeight <= 0) {
        return false;
    }
    if (state.fbos != nullptr && state.fbos->sceneWidth == fbWidth &&
        state.fbos->sceneHeight == fbHeight) {
        return true;
    }
    auto fresh = std::make_shared<FboSet>();
    if (!buildFboSet(*fresh, fbWidth, fbHeight)) {
        return false;
    }
    state.fbos = std::move(fresh);
    return true;
}

inline void getFramebufferSize(GLFWwindow* window, int& width, int& height) {
    width = 0;
    height = 0;
    glfwGetFramebufferSize(window, &width, &height);
}

inline void setupSystem(Registry& registry) {
    if (registry.getObject<BloomState>(kBloomStateName) != nullptr) {
        return;
    }

    BloomState state{};
    state.brightProgram = asset::AssetLoader::loadShaderProgram(std::string(kVertexShaderPath),
                                                                std::string(kBrightFragPath));
    state.blurProgram = asset::AssetLoader::loadShaderProgram(std::string(kVertexShaderPath),
                                                              std::string(kBlurFragPath));
    state.compositeProgram = asset::AssetLoader::loadShaderProgram(
        std::string(kVertexShaderPath), std::string(kCompositeFragPath));
    if (state.brightProgram.id == 0U || state.blurProgram.id == 0U ||
        state.compositeProgram.id == 0U) {
        LOG_WARN("Bloom shader programs failed to load; bloom disabled");
        return;
    }

    state.locBrightScene = glGetUniformLocation(state.brightProgram.id, "u_scene");
    state.locBrightThreshold = glGetUniformLocation(state.brightProgram.id, "u_threshold");

    state.locBlurImage = glGetUniformLocation(state.blurProgram.id, "u_image");
    state.locBlurTexelSize = glGetUniformLocation(state.blurProgram.id, "u_texelSize");
    state.locBlurHorizontal = glGetUniformLocation(state.blurProgram.id, "u_horizontal");

    state.locCompositeScene = glGetUniformLocation(state.compositeProgram.id, "u_scene");
    state.locCompositeBloom = glGetUniformLocation(state.compositeProgram.id, "u_bloom");
    state.locCompositeIntensity = glGetUniformLocation(state.compositeProgram.id, "u_intensity");

    state.quad = std::make_shared<graphics::ScreenQuad>();
    graphics::buildScreenQuad(*state.quad);

    registry.registerObject(kBloomStateName, std::move(state));
    LOG_INFO("Bloom ready (press B to toggle, currently disabled)");
}

inline void inputSystem(Registry& registry) {
    auto* state = registry.getObject<BloomState>(kBloomStateName);
    const auto* windowState = registry.getObject<window::WindowState>(window::kMainWindowStateName);
    if (state == nullptr || windowState == nullptr || windowState->handle == nullptr) {
        return;
    }

    const bool pressed = glfwGetKey(windowState->handle, GLFW_KEY_B) == GLFW_PRESS;
    if (pressed && !state->toggleWasPressed) {
        state->enabled = !state->enabled;
        LOG_INFO("Bloom: {}", state->enabled ? "enabled" : "disabled");
    }
    state->toggleWasPressed = pressed;
}

inline void beginScenePassSystem(Registry& registry) {
    auto* state = registry.getObject<BloomState>(kBloomStateName);
    const auto* windowState = registry.getObject<window::WindowState>(window::kMainWindowStateName);
    if (state == nullptr || !state->enabled || windowState == nullptr ||
        windowState->handle == nullptr) {
        return;
    }

    int fbWidth = 0;
    int fbHeight = 0;
    getFramebufferSize(windowState->handle, fbWidth, fbHeight);
    if (!ensureFbosForSize(*state, fbWidth, fbHeight)) {
        return;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, state->fbos->sceneFbo);
    glViewport(0, 0, fbWidth, fbHeight);
    // Match window::clearWindowSystem so the bloom-on view doesn't show a
    // different background than bloom-off.
    glClearColor(0.15f, 0.18f, 0.22f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

inline void postProcessSystem(Registry& registry) {
    auto* state = registry.getObject<BloomState>(kBloomStateName);
    const auto* windowState = registry.getObject<window::WindowState>(window::kMainWindowStateName);
    if (state == nullptr || !state->enabled || state->fbos == nullptr || state->quad == nullptr ||
        windowState == nullptr || windowState->handle == nullptr) {
        return;
    }
    if (state->brightProgram.id == 0U || state->blurProgram.id == 0U ||
        state->compositeProgram.id == 0U) {
        return;
    }

    int fbWidth = 0;
    int fbHeight = 0;
    getFramebufferSize(windowState->handle, fbWidth, fbHeight);
    if (fbWidth <= 0 || fbHeight <= 0) {
        return;
    }

    const GLboolean depthTestWasEnabled = glIsEnabled(GL_DEPTH_TEST);
    GLboolean depthMaskWas = GL_TRUE;
    glGetBooleanv(GL_DEPTH_WRITEMASK, &depthMaskWas);
    const GLboolean blendWasEnabled = glIsEnabled(GL_BLEND);

    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glDisable(GL_BLEND);

    const FboSet& fbos = *state->fbos;

    glBindFramebuffer(GL_FRAMEBUFFER, fbos.pingFbo);
    glViewport(0, 0, fbos.bloomWidth, fbos.bloomHeight);
    glUseProgram(state->brightProgram.id);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, fbos.sceneColor);
    if (state->locBrightScene >= 0) {
        glUniform1i(state->locBrightScene, 0);
    }
    if (state->locBrightThreshold >= 0) {
        glUniform1f(state->locBrightThreshold, state->threshold);
    }
    graphics::drawScreenQuad(*state->quad);

    const float texelX = 1.0f / static_cast<float>(fbos.bloomWidth);
    const float texelY = 1.0f / static_cast<float>(fbos.bloomHeight);
    glUseProgram(state->blurProgram.id);
    if (state->locBlurImage >= 0) {
        glUniform1i(state->locBlurImage, 0);
    }
    if (state->locBlurTexelSize >= 0) {
        glUniform2f(state->locBlurTexelSize, texelX, texelY);
    }
    for (int i = 0; i < kBlurIterations; ++i) {
        glBindFramebuffer(GL_FRAMEBUFFER, fbos.pongFbo);
        glViewport(0, 0, fbos.bloomWidth, fbos.bloomHeight);
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

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, fbWidth, fbHeight);
    glUseProgram(state->compositeProgram.id);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, fbos.sceneColor);
    if (state->locCompositeScene >= 0) {
        glUniform1i(state->locCompositeScene, 0);
    }
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, fbos.pingColor);
    if (state->locCompositeBloom >= 0) {
        glUniform1i(state->locCompositeBloom, 1);
    }
    if (state->locCompositeIntensity >= 0) {
        glUniform1f(state->locCompositeIntensity, state->intensity);
    }
    graphics::drawScreenQuad(*state->quad);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE0);
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

}  // namespace bloom

#endif  // CG_OPENGL_PROJECT_BLOOM_HPP
