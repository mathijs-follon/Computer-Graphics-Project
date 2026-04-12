#ifndef CG_OPENGL_PROJECT_GEOMETRY_HPP
#define CG_OPENGL_PROJECT_GEOMETRY_HPP
#include <algorithm>
#include <array>
#include <glm/glm.hpp>
#include <glm/geometric.hpp>
#include <glm/vec3.hpp>

struct AABB {
    glm::vec3 min{0.0f};
    glm::vec3 max{0.0f};

    [[nodiscard]] AABB transform(const glm::mat4& matrix) const {
        const glm::vec3 corners[8] = {
            {min.x, min.y, min.z}, {max.x, min.y, min.z}, {min.x, max.y, min.z},
            {max.x, max.y, min.z}, {min.x, min.y, max.z}, {max.x, min.y, max.z},
            {min.x, max.y, max.z}, {max.x, max.y, max.z},
        };

        glm::vec3 out_min(std::numeric_limits<float>::max());
        glm::vec3 out_max(std::numeric_limits<float>::lowest());

        for (const auto& corner : corners) {
            const auto w = glm::vec3(matrix * glm::vec4(corner, 1.0f));
            out_min = glm::min(out_min, w);
            out_max = glm::max(out_max, w);
        }
        return AABB{out_min, out_max};
    }
};

struct Plane {
    glm::vec3 normal{0.0f, 1.0f, 0.0f};
    float offset{0.0f};

    Plane() = default;

    Plane(const glm::vec3& point, const glm::vec3& norm)
        : normal(glm::normalize(norm)),
          offset(-glm::dot(normal, point)) {}

    [[nodiscard]] float signedDistance(const glm::vec3& point) const {
        return glm::dot(normal, point) + offset;
    }
};

struct Frustum {
    std::array<Plane, 6> planes{};

    static Frustum fromViewProjection(const glm::mat4& viewProjection) {
        Frustum frustum{};

        const glm::vec4& colX = viewProjection[0];
        const glm::vec4& colY = viewProjection[1];
        const glm::vec4& colZ = viewProjection[2];
        const glm::vec4& colW = viewProjection[3];

        const std::array combinations = {
            colW + colX, // Left
            colW - colX, // Right
            colW + colY, // Bottom
            colW - colY, // Top
            colW + colZ, // Near
            colW - colZ  // Far
        };

        for (std::size_t i = 0; i < 6; ++i) {
            const glm::vec4& planeEquation = combinations[i];

            auto normal = glm::vec3(planeEquation);
            const float length = glm::length(normal);

            const float inverseLength = (length > 1e-6f) ? (1.0f / length) : 1.0f;

            frustum.planes[i].normal = normal * inverseLength;
            frustum.planes[i].offset = planeEquation.w * inverseLength;
        }

        return frustum;
    }

    [[nodiscard]] bool intersectsWith(const AABB& aabb) const {
        return std::ranges::all_of(planes, [&](const Plane& plane) {

            glm::vec3 furthestPoint;
            furthestPoint.x = (plane.normal.x >= 0.0f) ? aabb.max.x : aabb.min.x;
            furthestPoint.y = (plane.normal.y >= 0.0f) ? aabb.max.y : aabb.min.y;
            furthestPoint.z = (plane.normal.z >= 0.0f) ? aabb.max.z : aabb.min.z;

            return plane.signedDistance(furthestPoint) >= 0.0f;
        });
    }
};

#endif  // CG_OPENGL_PROJECT_GEOMETRY_HPP
