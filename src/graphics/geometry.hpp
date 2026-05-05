#ifndef CG_OPENGL_PROJECT_GEOMETRY_HPP
#define CG_OPENGL_PROJECT_GEOMETRY_HPP
#include <algorithm>
#include <array>
#include <glm/ext/matrix_float3x3.hpp>
#include <glm/ext/matrix_float3x4.hpp>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/glm.hpp>
#include <glm/geometric.hpp>
#include <glm/vec3.hpp>
#include <map>
#include <optional>
#include <sys/types.h>
#include <vector>

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
        : normal(glm::normalize(norm)), offset(-glm::dot(normal, point)) {}

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
            colW + colX,  // Left
            colW - colX,  // Right
            colW + colY,  // Bottom
            colW - colY,  // Top
            colW + colZ,  // Near
            colW - colZ   // Far
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

struct FrenetFrame {
    glm::vec3 tangent;
    glm::vec3 normal;
    glm::vec3 binormal;
};

struct CubicBezierCurve {
    static constexpr glm::mat4 basis{-1, 3, -3, -1, 3, -6, 3, 0, -3, 3, 0, 0, 1, 0, 0, 0};
    const glm::mat4x3 controlPoints{};
    glm::mat4x3 coefficients{0.0};
    std::vector<glm::vec3> samplePoints{};
    std::map<double, double> arcLengthLUT{};

    CubicBezierCurve(const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2,
                     const glm::vec3& p3, uint sampleCount = 32)
        : controlPoints{p0, p1, p2, p3} {
        sampleFD(sampleCount);
    };

    void sampleFD(uint sampleCount) {
        if (sampleCount == 0)
            return;

        double dt = 1.0 / sampleCount;

        // first time sampling, calculate FD coefficients
        if (samplePoints.empty())
            coefficients = controlPoints * basis;

        arcLengthLUT.clear();
        samplePoints.clear();

        // !TODO: do this nice with matrices
        double x = coefficients[3][0];
        double dx = coefficients[0][0] * dt * dt * dt + coefficients[1][0] * dt * dt +
                    coefficients[2][0] * dt;
        double dx2 = 6 * coefficients[0][0] * dt * dt * dt + 2 * coefficients[1][0] * dt * dt;
        double dx3 = 6 * coefficients[0][0] * dt * dt * dt;

        double y = coefficients[3][1];
        double dy = coefficients[0][1] * dt * dt * dt + coefficients[1][1] * dt * dt +
                    coefficients[2][1] * dt;
        double dy2 = 6 * coefficients[0][1] * dt * dt * dt + 2 * coefficients[1][1] * dt * dt;
        double dy3 = 6 * coefficients[0][1] * dt * dt * dt;

        double z = coefficients[3][2];
        double dz = coefficients[0][2] * dt * dt * dt + coefficients[1][2] * dt * dt +
                    coefficients[2][2] * dt;
        double dz2 = 6 * coefficients[0][2] * dt * dt * dt + 2 * coefficients[1][2] * dt * dt;
        double dz3 = 6 * coefficients[0][2] * dt * dt * dt;

        samplePoints.push_back(glm::vec3(x, y, z));

        double arclength{};
        for (uint i = 0; i < sampleCount; i++) {
            x += dx;
            dx += dx2;
            dx2 += dx3;

            y += dy;
            dy += dy2;
            dy2 += dy3;

            z += dz;
            dz += dz2;
            dz2 += dz3;

            glm::vec3 newSample(x, y, z);

            if (!samplePoints.empty()) {
                arclength += glm::distance(samplePoints.back(), newSample);
                arcLengthLUT.insert({arclength, dt * sampleCount});
            }

            samplePoints.push_back(newSample);
        }
    }

    [[nodiscard]] std::optional<double> tvalueForDistance(double d) {
        auto upperBound = arcLengthLUT.upper_bound(d);

        // given d is beyond d values in LUT
        if (upperBound == arcLengthLUT.end())
            return std::nullopt;

        double upperDistance = upperBound->first;
        double upperT = upperBound->second;

        if (--upperBound == arcLengthLUT.begin())
            return std::nullopt;

        double lowerDistance = (upperBound)->first;
        double lowerT = upperBound->second;

        return lowerT + ((d - lowerDistance) / (upperDistance - lowerDistance)) * (upperT - lowerT);
    }

    [[nodiscard]] glm::vec3 pointAt(double t) const {
        glm::vec4 tVec = glm::vec4(t * t * t, t * t, t, 1);
        return controlPoints * basis * tVec;
    }

    [[nodiscard]] FrenetFrame frenetFrameAt(double t) const {
        glm::vec4 tderVec = glm::vec4(3 * t * t, 2 * t, 1, 0);
        glm::vec4 tder2Vec = glm::vec4(6 * t, 2, 0, 0);
        glm::vec3 tangent = glm::normalize(controlPoints * basis * tderVec);
        glm::vec3 normal = glm::normalize(controlPoints * basis * tder2Vec);

        return FrenetFrame{tangent, normal, glm::cross(tangent, normal)};
    }
};
#endif  // CG_OPENGL_PROJECT_GEOMETRY_HPP
