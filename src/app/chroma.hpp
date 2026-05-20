#ifndef CG_OPENGL_PROJECT_CHROMA_HPP
#define CG_OPENGL_PROJECT_CHROMA_HPP

// Chroma-keying overlay: a screen-aligned quad drawn after the main scene.
// Press 'L' to cycle three modes:
//   0 = hidden (no overlay)
//   1 = raw image overlay (frame texture shown over the scene, green still visible)
//   2 = chroma-keyed overlay (green discarded in YCbCr space, scene shows through)

#include "app/app.hpp"
#include "asset/asset.hpp"
#include "graphics/screen_quad.hpp"
#include "graphics/window.hpp"
#include "log/log.hpp"
#include "world/registry.hpp"

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

struct ChromaState {
    OverlayMode mode = OverlayMode::Hidden;
    bool toggleWasPressed = false;

    asset::ShaderProgram program{};
    asset::Texture texture{};
    std::shared_ptr<graphics::ScreenQuad> quad{};

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

inline void setupSystem(Registry& registry) {
    if (registry.getObject<ChromaState>(kChromaStateName) != nullptr) {
        return;
    }

    ChromaState state{};
    state.program = asset::AssetLoader::loadShaderProgram(std::string(kVertexShaderPath),
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

    state.quad = std::make_shared<graphics::ScreenQuad>();
    graphics::buildScreenQuad(*state.quad);

    registry.registerObject(kChromaStateName, std::move(state));
    LOG_INFO("Chroma overlay ready (press L to cycle: hidden -> overlay -> chroma-keyed)");
}

inline void inputSystem(Registry& registry) {
    auto* state = registry.getObject<ChromaState>(kChromaStateName);
    const auto* windowState = registry.getObject<window::WindowState>(window::kMainWindowStateName);
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
    if (state->program.id == 0U || state->quad == nullptr || state->quad->vao == 0U ||
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

    graphics::drawScreenQuad(*state->quad);

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
