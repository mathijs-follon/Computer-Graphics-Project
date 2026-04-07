cmake_minimum_required(VERSION 4.2)
project(CG_OpenGL_Project)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
enable_testing()

add_library(glad_gl STATIC
    third_party/glad/src/gl.c
)
target_include_directories(glad_gl PUBLIC
    third_party/glad/include
)

find_package(glfw3 CONFIG REQUIRED)
if(TARGET glfw)
    set(GLFW_TARGET glfw)
elseif(TARGET glfw::glfw)
    set(GLFW_TARGET glfw::glfw)
else()
    message(FATAL_ERROR "glfw3 package found, but no known GLFW target is exported.")
endif()

find_package(glm CONFIG QUIET)
if(TARGET glm::glm)
    set(GLM_TARGET glm::glm)
else()
    message(WARNING "Missing glm package. Install a package exporting glm::glm.")
endif()

find_package(imgui CONFIG QUIET)
if(TARGET imgui::imgui)
    set(IMGUI_TARGET imgui::imgui)
elseif(TARGET ImGui::ImGui)
    set(IMGUI_TARGET ImGui::ImGui)
else()
    message(WARNING "Missing imgui package. Install a package exporting imgui::imgui or ImGui::ImGui.")
endif()

find_package(nlohmann_json CONFIG QUIET)
if(TARGET nlohmann_json::nlohmann_json)
    set(JSON_TARGET nlohmann_json::nlohmann_json)
else()
    message(WARNING "Missing nlohmann_json package. Install a package exporting nlohmann_json::nlohmann_json.")
endif()

find_package(magic_enum CONFIG QUIET)
if(TARGET magic_enum::magic_enum)
    set(MAGIC_ENUM_TARGET magic_enum::magic_enum)
else()
    message(WARNING "Missing magic_enum package. Install a package exporting magic_enum::magic_enum.")
endif()

find_package(miniaudio CONFIG QUIET)
if(TARGET miniaudio::miniaudio)
    set(MINIAUDIO_TARGET miniaudio::miniaudio)
elseif(TARGET miniaudio)
    set(MINIAUDIO_TARGET miniaudio)
else()
    message(WARNING "Missing miniaudio package. Install a package exporting miniaudio::miniaudio or miniaudio.")
endif()

find_package(spdlog CONFIG REQUIRED)
if(TARGET spdlog::spdlog_header_only)
    set(SPDLOG_TARGET spdlog::spdlog_header_only)
elseif(TARGET spdlog::spdlog)
    set(SPDLOG_TARGET spdlog::spdlog)
else()
    message(FATAL_ERROR "spdlog package found, but no known spdlog target is exported.")
endif()

find_package(stb CONFIG QUIET)
if(TARGET stb::stb)
    set(STB_TARGET stb::stb)
else()
    message(WARNING "Missing stb package. Install a package exporting stb::stb.")
endif()

find_package(tinygltf CONFIG QUIET)
if(TARGET tinygltf::tinygltf)
    set(TINYGLTF_TARGET tinygltf::tinygltf)
elseif(TARGET tinygltf)
    set(TINYGLTF_TARGET tinygltf)
else()
    message(WARNING "Missing tinygltf package. Install a package exporting tinygltf::tinygltf or tinygltf.")
endif()

if(MSVC)
    set(PROJECT_WARNING_FLAGS /W4)
else()
    set(PROJECT_WARNING_FLAGS -Wall -Wextra)
endif()

add_executable(CG_OpenGL_Project
        src/main.cpp
        src/log/log.hpp
        src/log/log.cpp
)

target_include_directories(CG_OpenGL_Project PRIVATE
    src/
)

target_compile_options(CG_OpenGL_Project PRIVATE ${PROJECT_WARNING_FLAGS})

target_link_libraries(CG_OpenGL_Project PRIVATE
    glad_gl
    ${GLFW_TARGET}
    ${GLM_TARGET}
    ${IMGUI_TARGET}
    ${JSON_TARGET}
    ${MAGIC_ENUM_TARGET}
    ${MINIAUDIO_TARGET}
    ${SPDLOG_TARGET}
    ${STB_TARGET}
    ${TINYGLTF_TARGET}
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

