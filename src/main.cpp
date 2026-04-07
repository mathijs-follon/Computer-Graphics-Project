#include "log/log.hpp"

#include <glad/gl.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <cstdlib>

namespace {

void glfw_error_callback(int code, const char* description) {
    LOG_ERROR("GLFW error {}: {}", code, description != nullptr ? description : "(null)");
}

}  // namespace

int main() {
    logger::default_setup();

    glfwSetErrorCallback(glfw_error_callback);

    if (glfwInit() != GLFW_TRUE) {
        LOG_ERROR("Failed to initialize GLFW");
        return EXIT_FAILURE;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(800, 600, "Hello GLFW", nullptr, nullptr);
    if (window == nullptr) {
        LOG_ERROR("Failed to create GLFW window");
        glfwTerminate();
        return EXIT_FAILURE;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    const int gl_version = gladLoadGL(glfwGetProcAddress);
    if (gl_version == 0) {
        LOG_ERROR("Failed to load OpenGL via GLAD");
        glfwDestroyWindow(window);
        glfwTerminate();
        return EXIT_FAILURE;
    }

    LOG_INFO("Hello GLFW — window open (OpenGL {}.{})",
             GLAD_VERSION_MAJOR(gl_version),
             GLAD_VERSION_MINOR(gl_version));

    glClearColor(0.15f, 0.18f, 0.22f, 1.0f);

    while (glfwWindowShouldClose(window) == GLFW_FALSE) {
        glClear(GL_COLOR_BUFFER_BIT);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return EXIT_SUCCESS;
}
