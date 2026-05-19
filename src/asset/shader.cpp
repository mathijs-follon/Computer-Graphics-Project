#include "asset/shader.hpp"
#include "log/log.hpp"

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
        GLint logLength = 0;
        glGetShaderiv(shaderId, GL_INFO_LOG_LENGTH, &logLength);
        std::string log(logLength, '\0');

        glGetShaderInfoLog(shaderId, logLength, nullptr, log.data());
        LOG_ERROR(log);

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
        LOG_CRITICAL("NO VERTEX");
        return std::nullopt;
    }

    const auto fragmentShader = loadShaderFromPath(ShaderType::Fragment, fragmentShaderPath);
    if (!fragmentShader.has_value()) {
        glDeleteShader(vertexShader->id);
        LOG_CRITICAL("NO FRAG");
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

    program.resource->locViewPos = glGetUniformLocation(programId, "u_viewPos");

    program.resource->locMaterialAmbient = glGetUniformLocation(programId, "u_material.ambient");
    program.resource->locMaterialDiffuse = glGetUniformLocation(programId, "u_material.diffuse");
    program.resource->locMaterialShininess =
        glGetUniformLocation(programId, "u_material.shininess");
    program.resource->locMaterialSpecular = glGetUniformLocation(programId, "u_material.specular");

    // cache all the light data locaties
    for (int i = 0; i < ShaderProgram::MAX_LIGHT_COUNT; i++) {
        program.resource->LightResources[i].locActive =
            glGetUniformLocation(programId, std::format("u_lights[{}].on", i).c_str());
        program.resource->LightResources[i].locPos =
            glGetUniformLocation(programId, std::format("u_lights[{}].pos", i).c_str());
        program.resource->LightResources[i].locColor =
            glGetUniformLocation(programId, std::format("u_lights[{}].color", i).c_str());
        program.resource->LightResources[i].locLinAtt =
            glGetUniformLocation(programId, std::format("u_lights[{}].linAtt", i).c_str());
        program.resource->LightResources[i].locQuadAtt =
            glGetUniformLocation(programId, std::format("u_lights[{}].quadAtt", i).c_str());
        program.resource->LightResources[i].locAmbient =
            glGetUniformLocation(programId, std::format("u_lights[{}].ambientFac", i).c_str());
        program.resource->LightResources[i].locDiffuse =
            glGetUniformLocation(programId, std::format("u_lights[{}].diffuseFac", i).c_str());
        program.resource->LightResources[i].locSpecular =
            glGetUniformLocation(programId, std::format("u_lights[{}].specularFac", i).c_str());
    }

    return program;
}

}  // namespace asset
