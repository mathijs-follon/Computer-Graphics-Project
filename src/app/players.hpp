#ifndef CG_OPENGL_PROJECT_PLAYERS_HPP
#define CG_OPENGL_PROJECT_PLAYERS_HPP

#include <glm/glm.hpp>
#include <string>

struct ActivePlayer {
    std::string name;
};

struct Player {
    glm::vec3 position{0.0f, 0.0f, 0.0f};
    glm::vec3 forward{0.0f, 0.0f, -1.0f};
};

struct FreeRoamEntity {
    glm::vec3 position{0.0f, 740.0f, 1080.0f};
    float yaw{-90.0f};
    float pitch{-30.0f};
    float moveSpeed{320.0f};
    float mouseSensitivity{0.1f};
};

struct SlideRiderEntity {
    glm::vec3 position{0.0f};
    glm::vec3 forward{0.0f, 0.0f, 1.0f};
    glm::vec3 trackNormal{0.0f, 1.0f, 0.0f};
    float seatOffsetAlongNormal = 5.0f;
    float eyeHeight = 20.0f;
    float eyeForward = 30.0f;
    float fovYDeg = 120.0f;
    float modelYawCorrectionDeg = 0.0f;
    float modelPitchCorrectionDeg = 0.0f;
};


#endif  // CG_OPENGL_PROJECT_PLAYERS_HPP
