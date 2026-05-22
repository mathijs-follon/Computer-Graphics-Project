#ifndef CG_OPENGL_PROJECT_LIGHTS_HPP
#define CG_OPENGL_PROJECT_LIGHTS_HPP

#include "asset/shader.hpp"
#include "world/registry.hpp"
#include <glm/ext/vector_float3.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>

namespace lights {

struct Transform {
    glm::vec3 position{0.0f, -20.0f, -20.0f};
    glm::vec3 scale{1.0f, 1.0f, 1.0f};
    float uniformTargetSize = 1000.0f;
};

struct Light {
    Transform transform;
    glm::vec3 color = {1.0f, 1.0f, 1.0f};

    float linAtt = 0.001f;
    float quadAtt = 0.0001f;

    float ambientFac = 0.10;
    float diffuseFac = 0.50;
    float specularFac = 0.40;
};

// spawn the models & set initial lighting
inline void setupSystem(Registry& registry) {
    static constexpr Light Sun = Light{{{500.0f, 0.0f, 0.0f}, {100.0f, 100.0f, 100.0f}},
                                       {1.0f, 0.8f, 0.8f},
                                       0.00000001f,
                                       0.0000000001f,
                                       0.2,
                                       0.5,
                                       0.3};

    std::vector<Light> lights{Sun};

    // move lighting data into mesh shaders that support it
    for (const asset::ShaderProgram* meshShaderPrgm : registry.getObjects<asset::ShaderProgram>()) {
        GLint lightActiveLocation = glGetUniformLocation(meshShaderPrgm->id, "u_lights[0].on");
        if (lightActiveLocation >= 0) {
            glUseProgram(meshShaderPrgm->id);
            // move light data for each light
            for (size_t i = 0; i < asset::ShaderProgram::MAX_LIGHT_COUNT; i++) {
                asset::ShaderProgram::ProgramResource::LightResource* lightResources =
                    &meshShaderPrgm->resource->LightResources[i];

                if (i < lights.size()) {
                    glUniform1i(lightResources->locActive, GL_TRUE);
                    glUniform3fv(lightResources->locPos, 1,
                                 glm::value_ptr(lights[i].transform.position));
                    glUniform3fv(lightResources->locColor, 1, glm::value_ptr(lights[i].color));
                    glUniform1f(lightResources->locLinAtt, lights[i].linAtt);
                    glUniform1f(lightResources->locQuadAtt, lights[i].quadAtt);

                    glUniform1f(lightResources->locAmbient, lights[i].ambientFac);
                    glUniform1f(lightResources->locDiffuse, lights[i].diffuseFac);
                    glUniform1f(lightResources->locSpecular, lights[i].specularFac);

                } else {
                    glUniform1i(lightResources->locActive, GL_FALSE);
                };
            }

            glUseProgram(0);
        }
    }
}

}  // namespace lights

#endif  // CG_OPENGL_PROJECT_LIGHTS_HPP