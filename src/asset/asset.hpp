#ifndef CG_OPENGL_PROJECT_ASSET_HPP
#define CG_OPENGL_PROJECT_ASSET_HPP

#include "model.hpp"
#include "texture.hpp"

#include <optional>
#include <string>
#include <string_view>

#include "log/log.hpp"

namespace asset {

class AssetLoader {
public:
    static Texture loadTexture(const std::string& path) {
        if (const auto texture = loadTextureFromPath(path); texture.has_value()) {
            LOG_ERROR("Failed to load texture asset: {}", path);

            // TODO return static fallback simple texture here

            return Texture{};
        } else {
            return texture.value();
        }
    }

    static Model loadModel(const std::string& path) {
        if (const auto model = loadModelFromPath(path); model.has_value()) {
            LOG_ERROR("Failed to load model asset: {}", path);

            // TODO return static fallback simple model here

            return Model{};
        } else {
            return model.value();
        }
    }
};

}  // namespace asset

#endif  // CG_OPENGL_PROJECT_ASSET_HPP
