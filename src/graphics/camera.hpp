#ifndef CG_OPENGL_PROJECT_CAMERA_HPP
#define CG_OPENGL_PROJECT_CAMERA_HPP

#include "geometry.hpp"
#include "window.hpp"
#include "app/app.hpp"
#include "app/players.hpp"
#include "world/registry.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace camera {
inline constexpr auto kCameraStateName = "camera.state";
inline constexpr auto kFreeRoamEntityName = "scene.free_roam_entity";
inline constexpr auto kSlideRiderEntityName = "scene.slide_rider_entity";

struct Camera {
    glm::vec3 position{0.0f, 0.0f, 3.0f};
    glm::vec3 front{0.0f, 0.0f, 1.0f};
    glm::vec3 up{0.0f, 1.0f, 0.0f};

    float yaw{90.0f};
    float pitch{0.0f};

    float fovYDeg{60.0f};
    float aspect{16.0f / 9.0f};
    float nearZ{0.001f};
    float farZ{8000.0f};


    glm::mat4 viewMatrix{1.0f};
    glm::mat4 projMatrix{1.0f};
    glm::mat4 viewProjMatrix{1.0f};
    Frustum frustum{};
};

enum class CameraId : std::size_t {
    FreeRoam = 0,
    SlideFollow = 1,
};


struct CameraState {
    std::array<Camera, 2> cameras{};
    CameraId activeId{CameraId::FreeRoam};

    bool cursorCaptured{true};
    bool escapeWasPressed{false};
    bool leftMouseButtonPressed{false};
    bool switchWasPressed{false};

    double lastMouseX{0.0};
    double lastMouseY{0.0};
    bool firstMouse{true};
};

inline glm::vec3 directionFromYawPitchDegrees(float yawDeg, float pitchDeg) {
    const float yawRad = glm::radians(yawDeg);
    const float pitchRad = glm::radians(pitchDeg);
    return glm::normalize(glm::vec3(std::cos(pitchRad) * std::cos(yawRad), std::sin(pitchRad),
                                    std::cos(pitchRad) * std::sin(yawRad)));
}

inline void forwardToYawPitchDegrees(const glm::vec3& forward, float& yawOut, float& pitchOut) {
    glm::vec3 forwardCopy = forward;
    const float len = glm::length(forwardCopy);
    if (len < 1e-6f) {
        return;
    }
    forwardCopy /= len;
    pitchOut = glm::degrees(std::asin(std::clamp(forwardCopy.y, -1.0f, 1.0f)));
    yawOut = glm::degrees(std::atan2(forwardCopy.z, forwardCopy.x));
}

inline const Camera* activeCamera(const Registry& registry) {
    auto* state = registry.getObject<CameraState>(kCameraStateName);
    if (state == nullptr) {
        return nullptr;
    }
    return &state->cameras[static_cast<std::size_t>(state->activeId)];
}


inline void setupSystem(Registry& registry) {
    const auto* windowState = registry.getObject<window::WindowState>(window::kMainWindowStateName);
    if (windowState == nullptr || windowState->handle == nullptr) {
        return;
    }

    glfwSetInputMode(windowState->handle, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    if (glfwRawMouseMotionSupported() == GLFW_TRUE) {
        glfwSetInputMode(windowState->handle, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    }

    FreeRoamEntity freeRoam{};
    freeRoam.position = {0.0f, 740.0f, 1080.0f};
    freeRoam.yaw = -90.0f;
    freeRoam.pitch = -30.0f;
    registry.registerObject(kFreeRoamEntityName, freeRoam);

    SlideRiderEntity rider{};
    rider.position = {140.0f, 435.0f, -207.0f};
    rider.forward = {0.0f, -0.15f, -1.0f};
    registry.registerObject(kSlideRiderEntityName, rider);

    CameraState state{};

    Camera freeRoamCam{};
    freeRoamCam.fovYDeg = 50.0f;
    freeRoamCam.farZ = 8000.0f;

    Camera slideCam{};
    slideCam.fovYDeg = 65.0f;
    slideCam.farZ = 8000.0f;

    state.cameras[static_cast<std::size_t>(CameraId::FreeRoam)] = freeRoamCam;
    state.cameras[static_cast<std::size_t>(CameraId::SlideFollow)] = slideCam;

    state.cursorCaptured = true;
    registry.registerObject(kCameraStateName, state);
}

inline void inputSystem(Registry& registry) {
    auto* cameraState = registry.getObject<CameraState>(kCameraStateName);
    auto* freeRoam = registry.getObject<FreeRoamEntity>(kFreeRoamEntityName);
    const auto* time = registry.getObject<App::Time>("app.time");
    const auto* windowState = registry.getObject<window::WindowState>(window::kMainWindowStateName);
    if (cameraState == nullptr || freeRoam == nullptr || windowState == nullptr ||
        windowState->handle == nullptr || time == nullptr) {
        return;
    }

    GLFWwindow* window = windowState->handle;

    const bool escapePressed = glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS;
    if (escapePressed && ! cameraState->escapeWasPressed && cameraState->cursorCaptured) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        if (glfwRawMouseMotionSupported() == GLFW_TRUE) {
            glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_FALSE);
        }
        cameraState->cursorCaptured = false;
        cameraState->firstMouse = true;
    }
    cameraState->escapeWasPressed = escapePressed;

    const bool leftMouseButtonPressed = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    if (!cameraState->cursorCaptured) {
        if (leftMouseButtonPressed && ! cameraState->leftMouseButtonPressed) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            if (glfwRawMouseMotionSupported() == GLFW_TRUE) {
                glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
            }
            cameraState->cursorCaptured = true;
            cameraState->firstMouse = true;
        }
        cameraState->leftMouseButtonPressed = leftMouseButtonPressed;
        return;
    }
    cameraState->leftMouseButtonPressed = leftMouseButtonPressed;

    double mouseX{0.0}, mouseY{0.0};
    glfwGetCursorPos(window, &mouseX, &mouseY);

    if (cameraState->firstMouse) {
        cameraState->lastMouseX = mouseX;
        cameraState->lastMouseY = mouseY;
        cameraState->firstMouse = false;
    }

    const auto dx = static_cast<float>(mouseX - cameraState->lastMouseX);
    const auto dy = static_cast<float>(cameraState->lastMouseY - mouseY);
    cameraState->lastMouseX = mouseX;
    cameraState->lastMouseY = mouseY;

    freeRoam->yaw += dx * freeRoam->mouseSensitivity;
    freeRoam->pitch = std::clamp(freeRoam->pitch + dy * freeRoam->mouseSensitivity, -89.0f, 89.0f);


    const glm::vec3 lookDir = directionFromYawPitchDegrees(freeRoam->yaw, freeRoam->pitch);
    constexpr glm::vec3 worldUp{0.0f, 1.0f, 0.0f};
    const glm::vec3 flatFront = glm::normalize(glm::vec3(lookDir.x, 0.0f, lookDir.z));
    const glm::vec3 right = glm::normalize(glm::cross(flatFront, worldUp));

    float speed = freeRoam->moveSpeed;
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
        speed *= 2.5f;
    }
    const float velocity = speed * time->deltaTime;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        freeRoam->position += flatFront * velocity;
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        freeRoam->position -= flatFront * velocity;
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        freeRoam->position -= right * velocity;
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        freeRoam->position += right * velocity;
    }
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
        freeRoam->position += worldUp * velocity;
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
        freeRoam->position -= worldUp * velocity;
    }
}


inline void switchSystem(Registry& registry) {
    auto* cameraState = registry.getObject<CameraState>(kCameraStateName);
    const auto* windowState = registry.getObject<window::WindowState>(window::kMainWindowStateName);
    if (cameraState == nullptr || windowState == nullptr || windowState->handle == nullptr) {
        return;
    }

    const bool switchPressed = glfwGetKey(windowState->handle, GLFW_KEY_C) == GLFW_PRESS;
    if (switchPressed && !cameraState->switchWasPressed) {
        cameraState->activeId =
            cameraState->activeId == CameraId::FreeRoam ? CameraId::SlideFollow : CameraId::FreeRoam;
        cameraState->firstMouse = true;
        LOG_INFO("Switched active camera to {}",
                 cameraState->activeId == CameraId::FreeRoam ? "Free roam" : "Slide POV");
    }
    cameraState->switchWasPressed = switchPressed;
}

inline void syncCamerasFromEntitiesSystem(Registry& registry) {
    auto* cameraState = registry.getObject<CameraState>(kCameraStateName);
    if (cameraState == nullptr) {
        return;
    }

    if (const auto* freeRoam = registry.getObject<FreeRoamEntity>(kFreeRoamEntityName)) {
        auto& camera = cameraState->cameras[static_cast<std::size_t>(CameraId::FreeRoam)];
        camera.position = freeRoam->position;
        camera.yaw = freeRoam->yaw;
        camera.pitch = freeRoam->pitch;
    }

    if (const auto* rider = registry.getObject<SlideRiderEntity>(kSlideRiderEntityName)) {
        auto& camera = cameraState->cameras[static_cast<std::size_t>(CameraId::SlideFollow)];
        camera.position = rider->position;
        forwardToYawPitchDegrees(rider->forward, camera.yaw, camera.pitch);
    }
}


inline void updateMatricesSystem(Registry& registry) {
    auto* cameraState = registry.getObject<CameraState>(kCameraStateName);
    const auto* windowState = registry.getObject<window::WindowState>(window::kMainWindowStateName);
    if (cameraState == nullptr || windowState == nullptr || windowState->handle == nullptr) {
        return;
    }

    int width{0}, height{0};
    glfwGetFramebufferSize(windowState->handle, &width, &height);

    const float aspect = height > 0 ? static_cast<float>(width) / static_cast<float>(height) : 16.0f / 9.0f;
    constexpr glm::vec3 worldUp{0.0f, 1.0f, 0.0f};

    for (auto& camera : cameraState->cameras) {
        const float yawRad = glm::radians(camera.yaw);
        const float pitchRad = glm::radians(camera.pitch);
        camera.front = glm::normalize(glm::vec3(std::cos(pitchRad) * std::cos(yawRad), std::sin(pitchRad), std::cos(pitchRad) * std::sin(yawRad)));
        camera.up = worldUp;
        camera.aspect = aspect;
        camera.viewMatrix = glm::lookAt(camera.position, camera.position + camera.front, camera.up);
        camera.projMatrix = glm::perspective(glm::radians(camera.fovYDeg), camera.aspect, camera.nearZ, camera.farZ);
        camera.viewProjMatrix = camera.projMatrix * camera.viewMatrix;
        camera.frustum = Frustum::fromViewProjection(camera.viewProjMatrix);
    }
}

inline void debugPrintCameraSystem(Registry& registry) {
    ONLY_IF_DEBUG

    const Camera* camera = activeCamera(registry);
    if (!camera)
        return;

    static bool first = true;

    if (!first) {
        std::cout << "\033[" << 10 << "A";
    }

    first = false;

    std::cout << "Camera Debug:\n";

    std::cout << "Position: " << camera->position.x << ", " << camera->position.y << ", "
              << camera->position.z << "\n";

    std::cout << "Front:    " << camera->front.x << ", " << camera->front.y << ", " << camera->front.z
              << "\n";

    std::cout << "Yaw/Pitch: " << camera->yaw << " / " << camera->pitch << "\n";

    std::cout << "FOV: " << camera->fovYDeg << "\n";
    std::cout << "Aspect: " << camera->aspect << "\n";
    std::cout << "Near/Far: " << camera->nearZ << " / " << camera->farZ << "\n";

    std::cout << "View[0]: " << camera->viewMatrix[0][0] << ", " << camera->viewMatrix[0][1] << ", "
              << camera->viewMatrix[0][2] << ", " << camera->viewMatrix[0][3] << "\n";

    std::cout << "Proj[0]: " << camera->projMatrix[0][0] << ", " << camera->projMatrix[0][1] << ", "
              << camera->projMatrix[0][2] << ", " << camera->projMatrix[0][3] << "\n";

    std::cout << "====================\n";

    std::cout.flush();
}

}

#endif  // CG_OPENGL_PROJECT_CAMERA_HPP