#ifndef CG_OPENGL_PROJECT_TEXTURE_HPP
#define CG_OPENGL_PROJECT_TEXTURE_HPP

#include <glad/gl.h>

#include <optional>
#include <string_view>

namespace asset {

struct Texture {
};

[[nodiscard]] inline std::optional<Texture> loadTextureFromPath(std::string_view path) {
    return std::nullopt;
}

}  // namespace asset

#endif  // CG_OPENGL_PROJECT_TEXTURE_HPP
