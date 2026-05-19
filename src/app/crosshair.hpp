#ifndef CG_OPENGL_PROJECT_CROSSHAIR_HPP
#define CG_OPENGL_PROJECT_CROSSHAIR_HPP

// Crosshair overlay: a small centered quad rendered in NDC with a "+" shape
// drawn in the fragment shader. Drawn after bloom post-processing so it
// always lands on the default framebuffer, on top of everything else.

#include "asset/asset.hpp"
#include "graphics/screen_quad.hpp"
#include "graphics/window.hpp"
#include "log/log.hpp"
#include "world/registry.hpp"

#include <memory>

#include <glad/gl.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <glm/vec3.hpp>

namespace crosshair {

inline constexpr auto kCrosshairStateName = "crosshair.state";
inline constexpr std::string_view kVertexShaderPath = "assets/shaders/crosshair.vert";
inline constexpr std::string_view kFragmentShaderPath = "assets/shaders/crosshair.frag";

inline constexpr float kHalfSizePx = 14.0f;
inline constexpr float kThickness = 0.12f;
inline constexpr float kGap = 0.20f;

struct CrosshairState {
    bool visible = true;

    asset::ShaderProgram program{};
    std::shared_ptr<graphics::ScreenQuad> quad{};

    GLint locHalfSizeNdc = -1;
    GLint locColor = -1;
    GLint locAlpha = -1;
    GLint locThickness = -1;
    GLint locGap = -1;

    glm::vec3 color{1.0f, 1.0f, 1.0f};
    float alpha = 0.9f;
};

inline void setupSystem(Registry& registry) {
    if (registry.getObject<CrosshairState>(kCrosshairStateName) != nullptr) {
        return;
    }

    CrosshairState state{};
    state.program = asset::AssetLoader::loadShaderProgram(std::string(kVertexShaderPath),
                                                          std::string(kFragmentShaderPath));
    if (state.program.id == 0U) {
        LOG_WARN("Crosshair shader failed to load; crosshair disabled");
        return;
    }

    state.locHalfSizeNdc = glGetUniformLocation(state.program.id, "u_halfSizeNdc");
    state.locColor = glGetUniformLocation(state.program.id, "u_color");
    state.locAlpha = glGetUniformLocation(state.program.id, "u_alpha");
    state.locThickness = glGetUniformLocation(state.program.id, "u_thickness");
    state.locGap = glGetUniformLocation(state.program.id, "u_gap");

    state.quad = std::make_shared<graphics::ScreenQuad>();
    graphics::buildScreenQuad(*state.quad);

    registry.registerObject(kCrosshairStateName, std::move(state));
    LOG_INFO("Crosshair overlay ready");
}

inline void renderSystem(Registry& registry) {
    const auto* state = registry.getObject<CrosshairState>(kCrosshairStateName);
    const auto* windowState = registry.getObject<window::WindowState>(window::kMainWindowStateName);
    if (state == nullptr || !state->visible || state->program.id == 0U ||
        state->quad == nullptr || state->quad->vao == 0U || windowState == nullptr ||
        windowState->handle == nullptr) {
        return;
    }

    int fbWidth = 0;
    int fbHeight = 0;
    glfwGetFramebufferSize(windowState->handle, &fbWidth, &fbHeight);
    if (fbWidth <= 0 || fbHeight <= 0) {
        return;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    GLboolean depthMask = GL_TRUE;
    glGetBooleanv(GL_DEPTH_WRITEMASK, &depthMask);
    const GLboolean depthTestWasEnabled = glIsEnabled(GL_DEPTH_TEST);
    const GLboolean blendWasEnabled = glIsEnabled(GL_BLEND);

    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glUseProgram(state->program.id);

    const float halfX = kHalfSizePx / static_cast<float>(fbWidth);
    const float halfY = kHalfSizePx / static_cast<float>(fbHeight);
    if (state->locHalfSizeNdc >= 0) {
        glUniform2f(state->locHalfSizeNdc, halfX, halfY);
    }
    if (state->locColor >= 0) {
        glUniform3f(state->locColor, state->color.r, state->color.g, state->color.b);
    }
    if (state->locAlpha >= 0) {
        glUniform1f(state->locAlpha, state->alpha);
    }
    if (state->locThickness >= 0) {
        glUniform1f(state->locThickness, kThickness);
    }
    if (state->locGap >= 0) {
        glUniform1f(state->locGap, kGap);
    }

    graphics::drawScreenQuad(*state->quad);

    glUseProgram(0);

    if (depthTestWasEnabled == GL_TRUE) {
        glEnable(GL_DEPTH_TEST);
    }
    glDepthMask(depthMask);
    if (blendWasEnabled == GL_FALSE) {
        glDisable(GL_BLEND);
    }
}

}  // namespace crosshair

#endif  // CG_OPENGL_PROJECT_CROSSHAIR_HPP
