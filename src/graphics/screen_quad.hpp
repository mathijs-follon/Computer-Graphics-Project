#ifndef CG_OPENGL_PROJECT_SCREEN_QUAD_HPP
#define CG_OPENGL_PROJECT_SCREEN_QUAD_HPP


#include <array>
#include <cstdint>

#include <glad/gl.h>

namespace graphics {

struct ScreenQuad {
    GLuint vao = 0U;
    GLuint vbo = 0U;
    GLuint ebo = 0U;

    ScreenQuad() = default;
    ScreenQuad(const ScreenQuad&) = delete;
    ScreenQuad& operator=(const ScreenQuad&) = delete;
    ScreenQuad(ScreenQuad&&) = delete;
    ScreenQuad& operator=(ScreenQuad&&) = delete;

    ~ScreenQuad() {
        if (ebo != 0U) {
            glDeleteBuffers(1, &ebo);
        }
        if (vbo != 0U) {
            glDeleteBuffers(1, &vbo);
        }
        if (vao != 0U) {
            glDeleteVertexArrays(1, &vao);
        }
    }
};

inline void buildScreenQuad(ScreenQuad& quad) {
    constexpr std::array<float, 16> kVertices = {
        -1.0f, -1.0f, 0.0f, 0.0f,
         1.0f, -1.0f, 1.0f, 0.0f,
         1.0f,  1.0f, 1.0f, 1.0f,
        -1.0f,  1.0f, 0.0f, 1.0f,
    };
    constexpr std::array<std::uint32_t, 6> kIndices = {0U, 1U, 2U, 0U, 2U, 3U};

    glGenVertexArrays(1, &quad.vao);
    glGenBuffers(1, &quad.vbo);
    glGenBuffers(1, &quad.ebo);

    glBindVertexArray(quad.vao);

    glBindBuffer(GL_ARRAY_BUFFER, quad.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(kVertices), kVertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quad.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(kIndices), kIndices.data(), GL_STATIC_DRAW);

    constexpr GLsizei kStride = 4 * sizeof(float);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, kStride, reinterpret_cast<void*>(0));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, kStride,
                          reinterpret_cast<void*>(2 * sizeof(float)));

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

inline void drawScreenQuad(const ScreenQuad& quad) {
    glBindVertexArray(quad.vao);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

}  // namespace graphics

#endif  // CG_OPENGL_PROJECT_SCREEN_QUAD_HPP
