#ifndef CG_OPENGL_PROJECT_FBO_HPP
#define CG_OPENGL_PROJECT_FBO_HPP

#include <glad/gl.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "log/log.hpp"
namespace graphics {

struct SceneFbo {
    GLuint fbo = 0U;
    GLuint color = 0U;
    GLuint depth = 0U;
    int width = 0;
    int height = 0;

    SceneFbo() = default;
    SceneFbo(const SceneFbo&) = delete;
    SceneFbo& operator=(const SceneFbo&) = delete;
    SceneFbo(SceneFbo&&) = delete;
    SceneFbo& operator=(SceneFbo&&) = delete;

    ~SceneFbo() {
        if (fbo != 0U) {
            glDeleteFramebuffers(1, &fbo);
        }
        if (color != 0U) {
            glDeleteTextures(1, &color);
        }
        if (depth != 0U) {
            glDeleteRenderbuffers(1, &depth);
        }
    }
};

inline void getFramebufferSize(GLFWwindow* window, int& width, int& height) {
    width = 0;
    height = 0;
    glfwGetFramebufferSize(window, &width, &height);
}

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

inline bool buildSceneFbo(SceneFbo& fbo, int fbWidth, int fbHeight) {
    fbo.width = fbWidth;
    fbo.height = fbHeight;

    fbo.color = createColorTexture(fbWidth, fbHeight);
    glGenRenderbuffers(1, &fbo.depth);
    glBindRenderbuffer(GL_RENDERBUFFER, fbo.depth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, fbWidth, fbHeight);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    glGenFramebuffers(1, &fbo.fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo.fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fbo.color, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, fbo.depth);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        LOG_ERROR("Scene FBO incomplete");
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return false;
    }
    return true;
}  // namespace graphics

}  // namespace graphics

#endif  // CG_OPENGL_PROJECT_FBO_HPP