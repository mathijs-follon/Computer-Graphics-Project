#ifndef CG_OPENGL_PROJECT_MODEL_HPP
#define CG_OPENGL_PROJECT_MODEL_HPP

#include <optional>
#include <string_view>

namespace asset {

struct Model {
};

[[nodiscard]] inline std::optional<Model> loadModelFromPath(std::string_view path) {
    return std::nullopt;
}

}  // namespace asset

#endif  // CG_OPENGL_PROJECT_MODEL_HPP
