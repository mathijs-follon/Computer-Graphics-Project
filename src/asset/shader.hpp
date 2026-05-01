#ifndef CG_OPENGL_PROJECT_SHADER_HPP
#define CG_OPENGL_PROJECT_SHADER_HPP

#include <glad/gl.h>

#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace asset {

enum class ShaderType {
    Vertex,
    Fragment,
};

struct Shader {
    GLuint id = 0;
    ShaderType type = ShaderType::Vertex;
    std::string sourcePath;
};

struct ShaderProgram {
    GLuint id = 0;
    struct ProgramResource {
        GLuint id = 0U;
        GLint locMvp = -1;
        GLint locModel = -1;
        GLint locView = -1;
        GLint locProjection = -1;
        GLint locColor = -1;
        GLint locAlbedo = -1;

        ProgramResource() = default;
        explicit ProgramResource(GLuint programId)
            : id(programId) {}

        ~ProgramResource() {
            if (id != 0U) {
                glDeleteProgram(id);
            }
        }
    };

    std::shared_ptr<ProgramResource> resource{};
};

[[nodiscard]] std::optional<std::string> loadShaderSourceFromPath(std::string_view path);
[[nodiscard]] std::optional<Shader> loadShaderFromPath(ShaderType type, std::string_view path);
[[nodiscard]] std::optional<ShaderProgram> loadShaderProgramFromPaths(std::string_view vertexShaderPath,
                                                                      std::string_view fragmentShaderPath);

}  // namespace asset

#endif  // CG_OPENGL_PROJECT_SHADER_HPP
