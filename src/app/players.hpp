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
    glm::vec3 position{140.0f, 435.0f, -207.0f};
    glm::vec3 forward{0.0f, -0.15f, -1.0f};
};


#endif  // CG_OPENGL_PROJECT_PLAYERS_HPP
