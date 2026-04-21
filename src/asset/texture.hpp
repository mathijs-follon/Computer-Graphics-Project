#ifndef CG_OPENGL_PROJECT_TEXTURE_HPP
#define CG_OPENGL_PROJECT_TEXTURE_HPP

#include <glad/gl.h>
#include "log/log.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <optional>
#include <string_view>

namespace asset {

struct Texture {
    GLuint textureHdl;
};

[[nodiscard]] inline std::optional<Texture> loadTextureFromPath(std::string_view path) {
    stbi_set_flip_vertically_on_load(true);  // flip textures straight

    // load image
    int w, h, channelCnt;
    unsigned char* textureData = stbi_load(path.data(), &w, &h, &channelCnt, 0);

    if (textureData == NULL) {
        LOG_ERROR("Failed to load image: {} because: {}", path, stbi_failure_reason());
        return std::nullopt;
    }

    // generate texture object
    GLuint textureHdl;
    glGenTextures(1, &textureHdl);
    glBindTexture(GL_TEXTURE_2D, textureHdl);

    // setup texture
    int inputFormat = GL_RGB;

    if (channelCnt < 3) {
        LOG_ERROR("Unsupported channel count in image");
        stbi_image_free(textureData);
        return std::nullopt;
    } else if (channelCnt == 4)
        inputFormat = GL_RGBA;

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, inputFormat, GL_UNSIGNED_BYTE, textureData);
    glGenerateMipmap(GL_TEXTURE_2D);

    // setup texture filtering
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // free loaded image (not texture) and unbind texture
    stbi_image_free(textureData);
    glBindTexture(GL_TEXTURE_2D, 0);

    return Texture{textureHdl};
}

}  // namespace asset

#endif  // CG_OPENGL_PROJECT_TEXTURE_HPP
