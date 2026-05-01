#ifndef CG_OPENGL_PROJECT_ASSET_PATHS_HPP
#define CG_OPENGL_PROJECT_ASSET_PATHS_HPP

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace asset {

[[nodiscard]] inline std::filesystem::path resolveAssetPath(const std::filesystem::path& relative) {
    if (relative.empty()) {
        return relative;
    }
    if (std::filesystem::exists(relative)) {
        return relative;
    }
    const std::filesystem::path fromParent = std::filesystem::path("..") / relative;
    if (std::filesystem::exists(fromParent)) {
        return fromParent;
    }
    return relative;
}

[[nodiscard]] inline std::vector<std::string> fallbackCandidates(std::string_view path) {
    std::vector<std::string> candidates;
    if (path.empty()) {
        return candidates;
    }

    const std::filesystem::path resolved = resolveAssetPath(std::filesystem::path(path));
    candidates.push_back(resolved.string());
    if (resolved != std::filesystem::path(path)) {
        candidates.push_back(std::string(path));
    }
    return candidates;
}

}  // namespace asset

#endif  // CG_OPENGL_PROJECT_ASSET_PATHS_HPP
