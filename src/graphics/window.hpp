#ifndef CG_OPENGL_PROJECT_WINDOW_HPP
#define CG_OPENGL_PROJECT_WINDOW_HPP

#include "app/app.hpp"

#include <glad/gl.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "log/log.hpp"

namespace window {
inline constexpr auto kMainWindowStateName = "window.state";

struct WindowState {
    GLFWwindow* handle{nullptr};
};

inline void setupSystem(Registry& registry) {
    if (glfwInit() != GLFW_TRUE) {
        LOG_ERROR("Failed to initialize GLFW");
        registry.emitEvent(ApplicationExitRequest{});
        return;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* handle = glfwCreateWindow(1600, 900, "Glijbaan Scene", nullptr, nullptr);
    if (handle == nullptr) {
        LOG_ERROR("Failed to create GLFW window");
        glfwTerminate();
        registry.emitEvent(ApplicationExitRequest{});
        return;
    }

    glfwMakeContextCurrent(handle);
    glfwSwapInterval(1);

    const int gl_version = gladLoadGL(glfwGetProcAddress);
    if (gl_version == 0) {
        LOG_ERROR("Failed to load OpenGL via GLAD");
        glfwDestroyWindow(handle);
        glfwTerminate();
        registry.emitEvent(ApplicationExitRequest{});
        return;
    }

    registry.registerObject(kMainWindowStateName, WindowState{handle});

    registry.registerObject("app.time", App::Time{});

    if (auto* time = registry.getObject<App::Time>("app.time"); !time) {
        LOG_ERROR("app.time Time struct was deleted.");
    } else {
        time->currentTime = glfwGetTime();
    }

    LOG_INFO("Window ready (OpenGL {}.{})", GLAD_VERSION_MAJOR(gl_version),
             GLAD_VERSION_MINOR(gl_version));
}

inline void pollPlatformEventsSystem(Registry& registry) {
    const auto* windowState = registry.getObject<WindowState>(kMainWindowStateName);
    if (windowState == nullptr || windowState->handle == nullptr) {
        return;
    }

    glfwPollEvents();

    if (glfwWindowShouldClose(windowState->handle) == GLFW_TRUE) {
        registry.emitEvent(ApplicationExitRequest{});
    }

    auto* time = registry.getObject<App::Time>("app.time");
    if (!time) {
        LOG_ERROR("app.time Time struct was deleted.");
        return;
    }

    time->lastFrameTime = time->currentTime;
    time->currentTime = glfwGetTime();
    time->deltaTime = static_cast<float>(time->currentTime - time->lastFrameTime);
}

inline void clearWindowSystem(Registry&) {
    glClearColor(0.15f, 0.18f, 0.22f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

inline void presentSystem(Registry& registry) {
    const auto* windowState = registry.getObject<WindowState>(kMainWindowStateName);
    if (windowState == nullptr || windowState->handle == nullptr) {
        return;
    }

    glfwSwapBuffers(windowState->handle);
}

inline void shutdownSystem(Registry& registry) {
    if (auto* windowState = registry.getObject<WindowState>(kMainWindowStateName);
        windowState != nullptr && windowState->handle != nullptr) {
        glfwDestroyWindow(windowState->handle);
        windowState->handle = nullptr;
    }
    glfwTerminate();
}
}  // namespace window

#endif  // CG_OPENGL_PROJECT_WINDOW_HPP