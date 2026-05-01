#ifndef CG_OPENGL_PROJECT_ASSET_HPP
#define CG_OPENGL_PROJECT_ASSET_HPP

#include "asset_paths.hpp"
#include "model.hpp"
#include "shader.hpp"
#include "texture.hpp"

#include <optional>
#include <string>
#include <string_view>

#include "log/log.hpp"

namespace asset {

inline constexpr std::string_view kFallbackTexturePath = "assets/textures/fallback.png";

class AssetLoader {
public:
    static Texture loadTexture(const std::string& path) {
        for (const std::string& candidate : fallbackCandidates(path)) {
            if (const auto texture = loadTextureFromPath(candidate); texture.has_value()) {
                return texture.value();
            }
        }
        LOG_ERROR("Failed to load texture asset: {}", path);

        for (const std::string& fallbackPath : fallbackCandidates(kFallbackTexturePath)) {
            if (const auto fallback = loadTextureFromPath(fallbackPath); fallback.has_value()) {
                LOG_WARN("Using fallback texture {}", fallbackPath);
                return fallback.value();
            }
        }
        return Texture{};
    }

    static Model loadModel(const std::string& path) {
        for (const std::string& candidate : fallbackCandidates(path)) {
            if (const auto model = loadModelFromPath(candidate); model.has_value()) {
                return model.value();
            }
        }
        LOG_ERROR("Failed to load model asset: {}", path);
        return Model{};
    }

    static ShaderProgram loadShaderProgram(const std::string& vertexPath, const std::string& fragmentPath) {
        const auto vertexCandidates = fallbackCandidates(vertexPath);
        const auto fragmentCandidates = fallbackCandidates(fragmentPath);
        if (vertexCandidates.empty() || fragmentCandidates.empty()) {
            LOG_ERROR("Failed to load shader program from: {}, {}", vertexPath, fragmentPath);
            return ShaderProgram{};
        }

        if (const auto program = loadShaderProgramFromPaths(vertexCandidates.front(), fragmentCandidates.front());
            !program.has_value()) {
            LOG_ERROR("Failed to load shader program from: {}, {}", vertexPath, fragmentPath);
            return ShaderProgram{};
        } else {
            return program.value();
        }
    }
};

}  // namespace asset

#endif  // CG_OPENGL_PROJECT_ASSET_HPP
