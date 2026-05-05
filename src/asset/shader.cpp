#include "asset/shader.hpp"

#include <fstream>
#include <sstream>

namespace asset {

std::optional<std::string> loadShaderSourceFromPath(std::string_view path) {
    if (path.empty()) {
        return std::nullopt;
    }

    std::ifstream file(std::string(path), std::ios::binary);
    if (!file.is_open()) {
        return std::nullopt;
    }

    std::ostringstream contents;
    // Eerst volledige source inlezen; dat maakt compile-fouten reproduceerbaar.
    contents << file.rdbuf();
    return contents.str();
}

std::optional<Shader> loadShaderFromPath(ShaderType type, std::string_view path) {
    const auto source = loadShaderSourceFromPath(path);
    if (!source.has_value()) {
        return std::nullopt;
    }

    const GLenum glType = (type == ShaderType::Vertex) ? GL_VERTEX_SHADER : GL_FRAGMENT_SHADER;
    const GLuint shaderId = glCreateShader(glType);
    if (shaderId == 0U) {
        return std::nullopt;
    }

    const char* sourcePtr = source->c_str();
    glShaderSource(shaderId, 1, &sourcePtr, nullptr);
    glCompileShader(shaderId);

    // Stop direct bij compile-fouten zodat een kapotte shader niet verder gaat.
    GLint status = GL_FALSE;
    glGetShaderiv(shaderId, GL_COMPILE_STATUS, &status);
    if (status != GL_TRUE) {
        glDeleteShader(shaderId);
        return std::nullopt;
    }

    Shader shader{};
    shader.id = shaderId;
    shader.type = type;
    shader.sourcePath = std::string(path);
    return shader;
}

std::optional<ShaderProgram> loadShaderProgramFromPaths(std::string_view vertexShaderPath,
                                                        std::string_view fragmentShaderPath) {
    const auto vertexShader = loadShaderFromPath(ShaderType::Vertex, vertexShaderPath);
    if (!vertexShader.has_value()) {
        return std::nullopt;
    }

    const auto fragmentShader = loadShaderFromPath(ShaderType::Fragment, fragmentShaderPath);
    if (!fragmentShader.has_value()) {
        glDeleteShader(vertexShader->id);
        return std::nullopt;
    }

    const GLuint programId = glCreateProgram();
    if (programId == 0U) {
        glDeleteShader(vertexShader->id);
        glDeleteShader(fragmentShader->id);
        return std::nullopt;
    }

    glAttachShader(programId, vertexShader->id);
    glAttachShader(programId, fragmentShader->id);
    glLinkProgram(programId);

    // Link-status controleren om mismatch tussen vertex/fragment vroeg te zien.
    GLint status = GL_FALSE;
    glGetProgramiv(programId, GL_LINK_STATUS, &status);
    if (status != GL_TRUE) {
        glDeleteProgram(programId);
        glDeleteShader(vertexShader->id);
        glDeleteShader(fragmentShader->id);
        return std::nullopt;
    }

    glDetachShader(programId, vertexShader->id);
    glDetachShader(programId, fragmentShader->id);
    glDeleteShader(vertexShader->id);
    glDeleteShader(fragmentShader->id);

    // Uniform locaties cachen voor minder lookups tijdens renderen.
    ShaderProgram program{};
    program.id = programId;
    program.resource = std::make_shared<ShaderProgram::ProgramResource>(programId);
    program.resource->locMvp = glGetUniformLocation(programId, "u_mvp");
    program.resource->locModel = glGetUniformLocation(programId, "u_model");
    program.resource->locView = glGetUniformLocation(programId, "u_view");
    program.resource->locProjection = glGetUniformLocation(programId, "u_projection");
    program.resource->locColor = glGetUniformLocation(programId, "u_color");
    program.resource->locAlbedo = glGetUniformLocation(
        programId, "u_albedo");  // the base color / the reflectivity of a surface
    return program;
}

}  // namespace asset
