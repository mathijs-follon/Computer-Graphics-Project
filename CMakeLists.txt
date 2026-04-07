cmake_minimum_required(VERSION 4.3)
project(CG_Project)

set(CMAKE_CXX_STANDARD 23)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(lib/glfw)


add_executable(CG_Project
    src/main.cpp
)

target_include_directories(CG_Project PRIVATE
    src/
    lib/glad/generated/include/
    lib/glfw/include/
    lib/glm/
    lib/imgui/
    lib/json/include/
    lib/magic_enum/include/
    lib/miniaudio/
    lib/spdlog/include/
    lib/stb/
    lib/tinygltf/
)

target_link_libraries(CG_Project PRIVATE glfw)

add_custom_command(
        TARGET CG_Project
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/assets"
        "$<TARGET_FILE_DIR:CG_Project>/assets"
        COMMENT "Copying assets to $<TARGET_FILE_DIR:CG_Project>"
        VERBATIM)

