cmake_minimum_required(VERSION 3.30)
project(CG_OpenGL_Project)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
enable_testing()

include(FetchContent)
find_package(OpenGL REQUIRED)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_EXAMPLE OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_BENCH OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_SHARED OFF CACHE BOOL "" FORCE)

if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/glfw/CMakeLists.txt")
    FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG 3.4
        SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/glfw"
    )
    FetchContent_MakeAvailable(glfw)
else()
    add_subdirectory(third_party/glfw)
endif()

if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/spdlog/CMakeLists.txt")
    FetchContent_Declare(
        spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog.git
        GIT_TAG v1.15.3
        SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/spdlog"
    )
    FetchContent_MakeAvailable(spdlog)
else()
    add_subdirectory(third_party/spdlog)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/tinygltf/CMakeLists.txt")
    add_subdirectory(third_party/tinygltf)
elseif(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/tinygltf/tiny_gltf.h")
    FetchContent_Declare(
        tinygltf
        GIT_REPOSITORY https://github.com/syoyo/tinygltf.git
        GIT_TAG v2.9.3
        SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/tinygltf"
    )
    FetchContent_MakeAvailable(tinygltf)
endif()

if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/glad/src/gl.c")
    message(FATAL_ERROR "GLAD source not found at third_party/glad/src/gl.c")
endif()
add_library(glad_gl STATIC third_party/glad/src/gl.c)
target_include_directories(glad_gl PUBLIC third_party/glad/include)

if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/stb/stb_image.h")
    FetchContent_Declare(
        stb
        GIT_REPOSITORY https://github.com/nothings/stb.git
        GIT_TAG master
        SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/stb"
    )
    FetchContent_MakeAvailable(stb)
endif()
add_library(stb_image INTERFACE)
target_include_directories(stb_image INTERFACE third_party/stb)

if(NOT TARGET tinygltf)
    add_library(tinygltf INTERFACE)
    target_include_directories(tinygltf INTERFACE third_party/tinygltf)
endif()

if(MSVC)
    set(PROJECT_WARNING_FLAGS /W4)
else()
    set(PROJECT_WARNING_FLAGS -Wall -Wextra)
endif()

add_executable(CG_OpenGL_Project
        src/main.cpp
        src/log/log.cpp
        src/log/log.hpp
        src/world/registry.hpp
        src/app/pipeline.hpp
        src/app/app.hpp
        src/app/system.hpp
        src/world/events.hpp
        src/graphics/window.hpp
        src/graphics/camera.hpp
        src/graphics/geometry.hpp
        src/app/players.hpp
        src/asset/asset.hpp
        src/asset/texture.hpp
        src/asset/shader.hpp
        src/asset/model.hpp
        src/asset/material.hpp
)

# Remove DEBUG to disable logs to LOG_CONSOLE
target_compile_definitions(CG_OpenGL_Project PRIVATE DEBUG)

target_include_directories(CG_OpenGL_Project PRIVATE
    src/
)

target_compile_options(CG_OpenGL_Project PRIVATE ${PROJECT_WARNING_FLAGS})

target_link_libraries(CG_OpenGL_Project PRIVATE
    OpenGL::GL
    glad_gl
    glfw
    spdlog::spdlog
    tinygltf
    stb_image
)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/assets")
    add_custom_command(
            TARGET CG_OpenGL_Project
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/assets"
            "$<TARGET_FILE_DIR:CG_OpenGL_Project>/assets"
            COMMENT "Copying assets to $<TARGET_FILE_DIR:CG_OpenGL_Project>"
            VERBATIM)
endif()

add_executable(template_test
    tests/template_test.cpp
)

target_compile_options(template_test PRIVATE ${PROJECT_WARNING_FLAGS})

add_test(
    NAME template_test
    COMMAND template_test
)

