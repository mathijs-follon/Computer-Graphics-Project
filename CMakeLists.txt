cmake_minimum_required(VERSION 3.30)
project(CG_Project)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(glfw3 CONFIG REQUIRED)
if(TARGET glfw)
    set(GLFW_TARGET glfw)
elseif(TARGET glfw::glfw)
    set(GLFW_TARGET glfw::glfw)
else()
    message(FATAL_ERROR "glfw3 package found, but no known GLFW target is exported.")
endif()

find_package(glad CONFIG QUIET)
if(TARGET glad::glad)
    set(GLAD_TARGET glad::glad)
elseif(TARGET glad)
    set(GLAD_TARGET glad)
else()
    message(WARNING "Missing glad package. Install a package exporting glad::glad or glad.")
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

find_package(spdlog CONFIG QUIET)
if(TARGET spdlog::spdlog_header_only)
    set(SPDLOG_TARGET spdlog::spdlog_header_only)
elseif(TARGET spdlog::spdlog)
    set(SPDLOG_TARGET spdlog::spdlog)
else()
    message(WARNING "Missing spdlog package. Install a package exporting spdlog::spdlog_header_only or spdlog::spdlog.")
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

add_executable(CG_Project
    src/main.cpp
)

target_include_directories(CG_Project PRIVATE
    src/
)

target_link_libraries(CG_Project PRIVATE
    ${GLFW_TARGET}
    ${GLAD_TARGET}
    ${GLM_TARGET}
    ${IMGUI_TARGET}
    ${JSON_TARGET}
    ${MAGIC_ENUM_TARGET}
    ${MINIAUDIO_TARGET}
    ${SPDLOG_TARGET}
    ${STB_TARGET}
    ${TINYGLTF_TARGET}
)

add_custom_command(
        TARGET CG_Project
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/assets"
        "$<TARGET_FILE_DIR:CG_Project>/assets"
        COMMENT "Copying assets to $<TARGET_FILE_DIR:CG_Project>"
        VERBATIM)

