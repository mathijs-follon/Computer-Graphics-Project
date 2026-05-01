#include "asset/texture.hpp"

#include "log/log.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace asset {

std::optional<Texture> loadTextureFromPath(std::string_view path) {
    stbi_set_flip_vertically_on_load(true);

    int w = 0;
    int h = 0;
    int channelCnt = 0;
    unsigned char* textureData = stbi_load(path.data(), &w, &h, &channelCnt, 0);

    if (textureData == nullptr) {
        LOG_ERROR("Failed to load image: {} because: {}", path, stbi_failure_reason());
        return std::nullopt;
    }

    GLuint textureHdl = 0U;
    glGenTextures(1, &textureHdl);
    glBindTexture(GL_TEXTURE_2D, textureHdl);

    GLint internalFormat = GL_RGB8;
    GLenum inputFormat = GL_RGB;
    if (channelCnt == 1) {
        internalFormat = GL_R8;
        inputFormat = GL_RED;
    } else if (channelCnt == 2) {
        internalFormat = GL_RG8;
        inputFormat = GL_RG;
    } else if (channelCnt == 3) {
        internalFormat = GL_RGB8;
        inputFormat = GL_RGB;
    } else if (channelCnt == 4) {
        internalFormat = GL_RGBA8;
        inputFormat = GL_RGBA;
    } else {
        LOG_ERROR("Unsupported channel count in image: {}", channelCnt);
        stbi_image_free(textureData);
        glBindTexture(GL_TEXTURE_2D, 0);
        glDeleteTextures(1, &textureHdl);
        return std::nullopt;
    }

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, w, h, 0, inputFormat, GL_UNSIGNED_BYTE, textureData);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(textureData);
    glBindTexture(GL_TEXTURE_2D, 0);

    return Texture{textureHdl};
}

}  // namespace asset
