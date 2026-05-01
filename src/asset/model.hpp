#ifndef CG_OPENGL_PROJECT_MODEL_HPP
#define CG_OPENGL_PROJECT_MODEL_HPP

#include "material.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace asset {

struct Vertex {
    std::array<float, 3> position{};
    std::array<float, 3> normal{};
    std::array<float, 2> texCoord{};
    std::array<float, 3> tangent{};
    std::array<float, 3> bitangent{};
};

struct Mesh {
    std::string name;
    std::vector<Vertex> vertices;
    std::vector<std::uint32_t> indices;
    std::size_t materialIndex = 0;
};

struct Model {
    std::string sourcePath;
    std::vector<Mesh> meshes;
    std::vector<Material> materials;
};

[[nodiscard]] std::optional<Model> loadModelFromPath(std::string_view path);

}  // namespace asset

#endif  // CG_OPENGL_PROJECT_MODEL_HPP
