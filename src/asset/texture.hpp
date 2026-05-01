#ifndef CG_OPENGL_PROJECT_TEXTURE_HPP
#define CG_OPENGL_PROJECT_TEXTURE_HPP

#include <glad/gl.h>

#include <optional>
#include <string_view>

namespace asset {

struct Texture {
    GLuint textureHdl = 0U;
};

[[nodiscard]] std::optional<Texture> loadTextureFromPath(std::string_view path);

}  // namespace asset

#endif  // CG_OPENGL_PROJECT_TEXTURE_HPP
