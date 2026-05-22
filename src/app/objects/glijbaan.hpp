#ifndef CG_OPENGL_PROJECT_GLIJBAAN_HPP
#define CG_OPENGL_PROJECT_GLIJBAAN_HPP

#include "app/players.hpp"
#include "asset/render_object_spawner.hpp"
#include "graphics/camera.hpp"
#include "graphics/geometry.hpp"
#include "graphics/rendering.hpp"
#include "app/app.hpp"
#include "graphics/window.hpp"
#include "log/log.hpp"
#include "world/registry.hpp"

#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <algorithm>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/geometric.hpp>
#include <glm/vec3.hpp>

// C wissel naar slide / dinosaur POV
// G auto-rit aan/uit
// R terug naar start
// W/S handmatig (alleen in slide-modus)
// V debug-curve toggle

namespace glijbaan {

inline constexpr auto kGlijbaanStateName = "glijbaan.state";
inline constexpr std::string_view kDebugCurvePrefix = "scene.glijbaan.debug";
inline constexpr std::string_view kPipeMeshPrefix = "scene.glijbaan.pipe";
inline constexpr std::string_view kRiderMeshPrefix = "scene.glijbaan.rider";
inline constexpr const char* kDinosaurModelPath = "assets/models/dinosaur/scene.gltf";
inline constexpr float kRiderModelTargetSize = 55.0f;
inline constexpr float kHalfPipeRadius = 35.0f;
inline constexpr float kHalfPipeWallThickness = 5.0f;
inline constexpr float kGlijbaanTextureTileWorldUnits = 40.0f;
inline constexpr std::size_t kHalfPipeRingSegments = 20;
inline constexpr std::size_t kHalfPipeCenterlineSamplesPerSegment = 64;

inline constexpr std::array<glm::vec3, 9> kGlijbaanKnots = {
    // Sampled with coord debug
    glm::vec3{-48.99495f, -214.75156f, 134.66557f},
    glm::vec3{-27.471014f, -252.15022f, 305.4894f},
    glm::vec3{-189.63342f, -337.37402f, 337.59814f},
    glm::vec3{-319.11328f, -444.42938f, 195.30103f},
    glm::vec3{-365.77716f, -343.05505f, 8.638097f},
    glm::vec3{-246.88297f, -295.09128f, -384.28702f},
    glm::vec3{-16.14079f, -471.27438f, -527.46f},
    glm::vec3{292.04883f, -452.2132f, -578.3326f},
    glm::vec3{474.55807f, -540.41486f, -383.35184f},
};

struct GlijbaanPath {
    std::vector<CubicBezierCurve> segments{};
    std::vector<double> segmentStartDistance{};
    double totalLength = 0.0;

    void clear() {
        segments.clear();
        segmentStartDistance.clear();
        totalLength = 0.0;
    }
};

struct RiderMeshBinding {
    std::string meshName;
    glm::mat4 baseModelMatrix{1.0f};
};

struct GlijbaanState {
    GlijbaanPath path{};
    std::vector<glm::vec3> glijbaanCubicBezierSamples{};
    std::vector<rendering::GpuVertex> halfPipeVertices{};
    std::vector<uint32_t32_t> halfPipeIndices{};
    std::vector<RiderMeshBinding> riderMeshes{};

    double rideDistance = 0.0;
    float rideSpeed = 140.0f;
    bool autoRide = true;
    bool showCurveDebug = true;
    bool toggleWasPressed = false;
    bool resetWasPressed = false;
};

inline glm::vec3 knotAt(int index) {
    constexpr int knotCount = static_cast<int>(kGlijbaanKnots.size());
    if (index < 0) {
        return 2.0f * kGlijbaanKnots[0] - kGlijbaanKnots[1];
    }
    if (index >= knotCount) {
        return 2.0f * kGlijbaanKnots[knotCount - 1] - kGlijbaanKnots[knotCount - 2];
    }
    return kGlijbaanKnots[static_cast<std::size_t>(index)];
}

inline void catmullRomToBezier(const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2,
                               const glm::vec3& p3, glm::vec3& b0, glm::vec3& b1, glm::vec3& b2,
                               glm::vec3& b3) {
    b0 = p1;
    b1 = p1 + (p2 - p0) / 6.0f;
    b2 = p2 - (p3 - p1) / 6.0f;
    b3 = p2;
}

inline void appendBezierSamples(std::vector<glm::vec3>& out, const glm::vec3& p0,
                                const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3,
                                std::size_t sampleCount) {
    const CubicBezierCurve segment{p0, p1, p2, p3, static_cast<unsigned int>(sampleCount)};
    if (segment.samplePoints.empty()) {
        return;
    }
    if (out.empty()) {
        out.insert(out.end(), segment.samplePoints.begin(), segment.samplePoints.end());
        return;
    }
    out.insert(out.end(), segment.samplePoints.begin() + 1, segment.samplePoints.end());
}

inline std::vector<glm::vec3> buildCenterLineSamples(std::size_t samplesPerSegment = 64) {
    std::vector<glm::vec3> samples;
    const std::size_t segmentCount = kGlijbaanKnots.size() - 1;
    samples.reserve(samplesPerSegment * segmentCount + segmentCount);

    for (std::size_t i = 0; i < segmentCount; ++i) {
        const int index = static_cast<int>(i);
        glm::vec3 b0{};
        glm::vec3 b1{};
        glm::vec3 b2{};
        glm::vec3 b3{};
        catmullRomToBezier(knotAt(index - 1), knotAt(index), knotAt(index + 1), knotAt(index + 2),
                           b0, b1, b2, b3);
        appendBezierSamples(samples, b0, b1, b2, b3, samplesPerSegment);
    }
    return samples;
}

inline constexpr std::size_t kArcLengthSamplesPerSegment = 128;

inline double wrapArcLength(double distance, double totalLength) {
    if (totalLength <= 0.0) {
        return 0.0;
    }
    distance = std::fmod(distance, totalLength);
    if (distance < 0.0) {
        distance += totalLength;
    }
    return distance;
}

inline GlijbaanPath buildGlijbaanPath(std::size_t samplesPerSegment = kArcLengthSamplesPerSegment) {
    GlijbaanPath path{};
    path.segmentStartDistance.push_back(0.0);

    const std::size_t segmentCount = kGlijbaanKnots.size() - 1;
    path.segments.reserve(segmentCount);

    for (std::size_t i = 0; i < segmentCount; ++i) {
        const int index = static_cast<int>(i);
        glm::vec3 b0{};
        glm::vec3 b1{};
        glm::vec3 b2{};
        glm::vec3 b3{};
        catmullRomToBezier(knotAt(index - 1), knotAt(index), knotAt(index + 1), knotAt(index + 2),
                           b0, b1, b2, b3);
        path.segments.emplace_back(b0, b1, b2, b3, static_cast<unsigned int>(samplesPerSegment));

        double segmentLength = 0.0;
        if (!path.segments.back().arcLengthLUT.empty()) {
            segmentLength = path.segments.back().arcLengthLUT.rbegin()->first;
        }
        path.totalLength += segmentLength;
        path.segmentStartDistance.push_back(path.totalLength);
    }

    return path;
}

struct PathSample {
    std::size_t segmentIndex = 0;
    double localT = 0.0;
};

// Opgave: motion uses arc length d; per-segment LUT maps local d to curve parameter t.
inline std::optional<PathSample> sampleAtDistance(const GlijbaanPath& path, double distance) {
    if (path.segments.empty() || path.totalLength <= 0.0) {
        return std::nullopt;
    }

    distance = wrapArcLength(distance, path.totalLength);

    const auto segmentIt = std::upper_bound(path.segmentStartDistance.begin(),
                                            path.segmentStartDistance.end(), distance);
    std::size_t segmentIndex = 0;
    if (segmentIt != path.segmentStartDistance.begin()) {
        segmentIndex = static_cast<std::size_t>(
            std::distance(path.segmentStartDistance.begin(), segmentIt) - 1);
    }
    if (segmentIndex >= path.segments.size()) {
        segmentIndex = path.segments.size() - 1;
    }

    const double localDistance = distance - path.segmentStartDistance[segmentIndex];
    const std::optional<double> localT =
        path.segments[segmentIndex].tvalueForDistance(localDistance);
    if (!localT.has_value()) {
        return std::nullopt;
    }

    return PathSample{.segmentIndex = segmentIndex, .localT = *localT};
}

inline glm::vec3 positionOnPath(const GlijbaanPath& path, double distance) {
    const std::optional<PathSample> sample = sampleAtDistance(path, distance);
    if (!sample.has_value()) {
        return glm::vec3{0.0f};
    }
    return path.segments[sample->segmentIndex].pointAt(sample->localT);
}

inline glm::vec3 tangentOnPath(const GlijbaanPath& path, double distance) {
    const std::optional<PathSample> sample = sampleAtDistance(path, distance);
    if (!sample.has_value()) {
        return glm::vec3{0.0f, 0.0f, 1.0f};
    }

    const glm::vec3 derivative = path.segments[sample->segmentIndex].tangentAt(sample->localT);
    const float length = glm::length(derivative);
    if (length < 1e-6f) {
        return glm::vec3{0.0f, 0.0f, 1.0f};
    }
    return derivative / length;
}

inline float maxExtent(const std::vector<glm::vec3>& points) {
    glm::vec3 boundsMin(std::numeric_limits<float>::max());
    glm::vec3 boundsMax(std::numeric_limits<float>::lowest());
    for (const glm::vec3& point : points) {
        boundsMin = glm::min(boundsMin, point);
        boundsMax = glm::max(boundsMax, point);
    }
    const glm::vec3 extent = boundsMax - boundsMin;
    return std::max({extent.x, extent.y, extent.z, 1.0e-4f});
}

inline float maxExtent(const std::vector<rendering::GpuVertex>& vertices) {
    glm::vec3 boundsMin(std::numeric_limits<float>::max());
    glm::vec3 boundsMax(std::numeric_limits<float>::lowest());
    for (const rendering::GpuVertex& vertex : vertices) {
        const glm::vec3 position{vertex.px, vertex.py, vertex.pz};
        boundsMin = glm::min(boundsMin, position);
        boundsMax = glm::max(boundsMax, position);
    }
    const glm::vec3 extent = boundsMax - boundsMin;
    return std::max({extent.x, extent.y, extent.z, 1.0e-4f});
}

// Track frame: tangent along the curve, slideUp +-= world +Y, slideRight horizontal in the
// cross-section.
inline void computeFrameAt(const glm::vec3& tangent, glm::vec3& normalOut, glm::vec3& binormalOut) {
    constexpr glm::vec3 worldUp{0.0f, 1.0f, 0.0f};
    glm::vec3 tangentSafe = tangent;
    if (glm::length(tangentSafe) < 1e-6f) {
        tangentSafe = glm::vec3{0.0f, 0.0f, 1.0f};
    } else {
        tangentSafe = glm::normalize(tangentSafe);
    }

    binormalOut = glm::cross(tangentSafe, worldUp);
    if (glm::length(binormalOut) < 1e-4f) {
        binormalOut = glm::normalize(glm::cross(tangentSafe, glm::vec3{1.0f, 0.0f, 0.0f}));
    } else {
        binormalOut = glm::normalize(binormalOut);
    }
    normalOut = glm::normalize(glm::cross(binormalOut, tangentSafe));

    if (glm::dot(normalOut, worldUp) < 0.0f) {
        normalOut = -normalOut;
        binormalOut = -binormalOut;
    }
}

inline void pushGpuVertex(std::vector<rendering::GpuVertex>& verticesOut, const glm::vec3& position,
                          const glm::vec3& normal, float u, float v) {
    const glm::vec3 normalSafe =
        glm::length(normal) > 1e-6f ? glm::normalize(normal) : glm::vec3{0.0f, 1.0f, 0.0f};
    verticesOut.push_back(rendering::GpuVertex{
        .px = position.x,
        .py = position.y,
        .pz = position.z,
        .nx = normalSafe.x,
        .ny = normalSafe.y,
        .nz = normalSafe.z,
        .u = u,
        .v = v,
    });
}

// Solid half-pipe shell: inner riding surface, outer wall, side bands; trough opens toward
// +slideUp.
inline void buildHalfPipeMesh(const std::vector<glm::vec3>& centerLine, float radius,
                              float wallThickness, std::size_t ringSegments,
                              std::vector<rendering::GpuVertex>& verticesOut,
                              std::vector<uint32_t>& indicesOut) {
    if (centerLine.size() < 2 || ringSegments < 3 || radius <= 0.0f || wallThickness <= 0.0f) {
        return;
    }

    verticesOut.clear();
    indicesOut.clear();

    constexpr std::size_t kLayerInner = 0;
    constexpr std::size_t kLayerOuter = 1;
    constexpr std::size_t kLayerCount = 2;

    const std::size_t stationCount = centerLine.size();
    const std::size_t ringVertexCount = ringSegments;
    const std::size_t vertsPerStation = kLayerCount * ringVertexCount;
    verticesOut.reserve(stationCount * vertsPerStation);
    indicesOut.reserve((stationCount - 1) * ringSegments * (ringSegments - 1) * 6);

    std::vector<glm::vec3> tangents(stationCount);
    for (std::size_t station = 0; station < stationCount; ++station) {
        if (station == 0) {
            tangents[station] = centerLine[1] - centerLine[0];
        } else if (station + 1 == stationCount) {
            tangents[station] = centerLine[station] - centerLine[station - 1];
        } else {
            tangents[station] = centerLine[station + 1] - centerLine[station - 1];
        }
    }

    const auto vertexIndex = [vertsPerStation, ringVertexCount](
                                 std::size_t station, std::size_t layer, std::size_t ring) {
        return static_cast<uint32_t>(station * vertsPerStation + layer * ringVertexCount +
                                          ring);
    };

    const auto pushTriangle = [&](uint32_t a, uint32_t b, uint32_t c) {
        indicesOut.push_back(a);
        indicesOut.push_back(b);
        indicesOut.push_back(c);
    };

    const auto pushQuad = [&](uint32_t a, uint32_t b, uint32_t c, uint32_t d) {
        pushTriangle(a, b, c);
        pushTriangle(b, d, c);
    };

    std::vector<float> stationArcLength(stationCount, 0.0f);
    for (std::size_t station = 1; station < stationCount; ++station) {
        stationArcLength[station] = stationArcLength[station - 1] +
                                    glm::length(centerLine[station] - centerLine[station - 1]);
    }

    const float crossSectionArc = glm::pi<float>() * radius;
    const float tileUnits = std::max(kGlijbaanTextureTileWorldUnits, 1.0e-3f);

    for (std::size_t station = 0; station < stationCount; ++station) {
        glm::vec3 slideUp{};
        glm::vec3 slideRight{};
        computeFrameAt(tangents[station], slideUp, slideRight);

        const glm::vec3& position = centerLine[station];
        const float uCoord = stationArcLength[station] / tileUnits;

        std::vector<glm::vec3> innerPositions(ringSegments);
        std::vector<glm::vec3> outerPositions(ringSegments);
        std::vector<glm::vec3> wallOutwards(ringSegments);

        for (std::size_t ring = 0; ring < ringSegments; ++ring) {
            const float theta = -glm::pi<float>() * 0.5f + glm::pi<float>() *
                                                               static_cast<float>(ring) /
                                                               static_cast<float>(ringSegments - 1);
            const glm::vec3 offset =
                radius * (std::sin(theta) * slideRight - std::cos(theta) * slideUp);

            glm::vec3 wallOutward = offset;
            if (glm::length(wallOutward) < 1e-6f) {
                wallOutward = -slideUp;
            } else {
                wallOutward = glm::normalize(wallOutward);
            }

            innerPositions[ring] = position + offset;
            outerPositions[ring] = innerPositions[ring] + wallOutward * wallThickness;
            wallOutwards[ring] = wallOutward;
        }

        for (std::size_t ring = 0; ring < ringSegments; ++ring) {
            const float v01 = (static_cast<float>(ring) + 0.5f) / static_cast<float>(ringSegments);
            const float vCoord = v01 * crossSectionArc / tileUnits;
            pushGpuVertex(verticesOut, innerPositions[ring], -wallOutwards[ring], uCoord, vCoord);
        }
        for (std::size_t ring = 0; ring < ringSegments; ++ring) {
            const float v01 = (static_cast<float>(ring) + 0.5f) / static_cast<float>(ringSegments);
            const float vCoord = v01 * crossSectionArc / tileUnits;
            pushGpuVertex(verticesOut, outerPositions[ring], wallOutwards[ring], uCoord, vCoord);
        }
    }

    for (std::size_t station = 0; station + 1 < stationCount; ++station) {
        for (std::size_t ring = 0; ring + 1 < ringSegments; ++ring) {
            const uint32_t in0 = vertexIndex(station, kLayerInner, ring);
            const uint32_t in1 = vertexIndex(station, kLayerInner, ring + 1);
            const uint32_t in2 = vertexIndex(station + 1, kLayerInner, ring);
            const uint32_t in3 = vertexIndex(station + 1, kLayerInner, ring + 1);
            pushQuad(in0, in1, in2, in3);

            const uint32_t out0 = vertexIndex(station, kLayerOuter, ring);
            const uint32_t out1 = vertexIndex(station, kLayerOuter, ring + 1);
            const uint32_t out2 = vertexIndex(station + 1, kLayerOuter, ring);
            const uint32_t out3 = vertexIndex(station + 1, kLayerOuter, ring + 1);
            pushQuad(out0, out2, out1, out3);
        }

        for (std::size_t ring = 0; ring < ringSegments; ++ring) {
            const uint32_t innerNear = vertexIndex(station, kLayerInner, ring);
            const uint32_t outerNear = vertexIndex(station, kLayerOuter, ring);
            const uint32_t innerFar = vertexIndex(station + 1, kLayerInner, ring);
            const uint32_t outerFar = vertexIndex(station + 1, kLayerOuter, ring);
            pushQuad(innerNear, innerFar, outerNear, outerFar);
        }
    }
}

// Align model +Y to track normal and model +Z to slide direction (tangent).
inline glm::mat4 orientationFromTrackFrame(const glm::vec3& tangent, const glm::vec3& trackNormal) {
    glm::vec3 forward = tangent;
    if (glm::length(forward) < 1e-6f) {
        forward = glm::vec3{0.0f, 0.0f, 1.0f};
    } else {
        forward = glm::normalize(forward);
    }

    glm::vec3 up = trackNormal;
    if (glm::length(up) < 1e-6f) {
        up = glm::vec3{0.0f, 1.0f, 0.0f};
    } else {
        up = glm::normalize(up);
    }

    glm::vec3 right = glm::cross(up, forward);
    if (glm::length(right) < 1e-4f) {
        right = glm::normalize(glm::cross(up, glm::vec3{1.0f, 0.0f, 0.0f}));
    } else {
        right = glm::normalize(right);
    }

    forward = glm::normalize(glm::cross(up, right));

    return glm::mat4(glm::vec4(right, 0.0f), glm::vec4(up, 0.0f), glm::vec4(forward, 0.0f),
                     glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
}

inline glm::mat4 buildRiderWorldMatrix(const SlideRiderEntity& rider) {
    const glm::mat4 orientation = orientationFromTrackFrame(rider.forward, rider.trackNormal);
    const glm::mat4 yawCorrection = glm::rotate(
        glm::mat4{1.0f}, glm::radians(rider.modelYawCorrectionDeg), glm::vec3{0.0f, 1.0f, 0.0f});
    const glm::mat4 pitchCorrection = glm::rotate(
        glm::mat4{1.0f}, glm::radians(rider.modelPitchCorrectionDeg), glm::vec3{1.0f, 0.0f, 0.0f});
    const glm::mat4 translation = glm::translate(glm::mat4{1.0f}, rider.position);
    return translation * orientation * pitchCorrection * yawCorrection;
}

inline glm::vec3 seatPositionOnPath(const GlijbaanPath& path, double distance, float seatOffset) {
    const glm::vec3 center = positionOnPath(path, distance);
    const glm::vec3 tangent = tangentOnPath(path, distance);
    glm::vec3 normal{};
    glm::vec3 binormal{};
    computeFrameAt(tangent, normal, binormal);
    return center - normal * seatOffset;
}

inline void syncRiderMeshes(Registry& registry, const GlijbaanState& state,
                            const SlideRiderEntity& rider) {
    const glm::mat4 worldMatrix = buildRiderWorldMatrix(rider);
    for (const RiderMeshBinding& binding : state.riderMeshes) {
        if (rendering::RenderMeshInstance* mesh =
                registry.getObject<rendering::RenderMeshInstance>(binding.meshName);
            mesh != nullptr) {
            mesh->modelMatrix = worldMatrix * binding.baseModelMatrix;
            mesh->visible = true;
            mesh->useFrustumCull = false;
            const glm::vec3 worldMin =
                glm::vec3(worldMatrix * glm::vec4(mesh->modelBounds.min, 1.0f));
            const glm::vec3 worldMax =
                glm::vec3(worldMatrix * glm::vec4(mesh->modelBounds.max, 1.0f));
            mesh->modelBounds = AABB{glm::min(worldMin, worldMax), glm::max(worldMin, worldMax)};
        }
    }
}

inline void captureRiderMeshBindings(Registry& registry, GlijbaanState& state) {
    state.riderMeshes.clear();
    const std::string prefix{kRiderMeshPrefix};
    for (auto [name, mesh] : registry.getEntries<rendering::RenderMeshInstance>()) {
        if (mesh == nullptr || !name.starts_with(prefix)) {
            continue;
        }
        state.riderMeshes.push_back(
            RiderMeshBinding{.meshName = std::string{name}, .baseModelMatrix = mesh->modelMatrix});
    }
}

inline void spawnRiderModel(Registry& registry, GlijbaanState& state) {
    asset::RenderObjectSpawnRequest request{};
    request.namePrefix = std::string(kRiderMeshPrefix);
    request.modelPath = kDinosaurModelPath;
    request.vertexShaderPath = "assets/shaders/default.vert";
    request.fragmentShaderPath = "assets/shaders/default.frag";
    request.useModelMaterialTexture = true;
    request.worldPosition = glm::vec3{0.0f};
    request.uniformTargetSize = kRiderModelTargetSize;
    request.centerModel = true;
    request.layer = rendering::RenderLayer::Opaque;
    request.enableFrustumCull = false;

    const asset::RenderObjectSpawnResult result =
        asset::spawnModelAsRenderMeshes(registry, request);
    if (!result.error.empty()) {
        LOG_WARN("Glijbaan rider spawn failed: {}", result.error);
        return;
    }

    captureRiderMeshBindings(registry, state);
    for (const RiderMeshBinding& binding : state.riderMeshes) {
        if (rendering::RenderMeshInstance* mesh =
                registry.getObject<rendering::RenderMeshInstance>(binding.meshName);
            mesh != nullptr) {
            mesh->useFrustumCull = false;
        }
    }
    LOG_INFO("Spawned glijbaan rider '{}' ({} meshes)", request.namePrefix, result.meshCount);
}

inline void initializeRiderEntity(Registry& registry, const GlijbaanPath& path) {
    auto* rider = registry.getObject<SlideRiderEntity>(camera::kSlideRiderEntityName);
    if (rider == nullptr || path.totalLength <= 0.0) {
        return;
    }

    rider->forward = tangentOnPath(path, 0.0);
    glm::vec3 binormal{};
    computeFrameAt(rider->forward, rider->trackNormal, binormal);
    rider->position = seatPositionOnPath(path, 0.0, rider->seatOffsetAlongNormal);
}

inline void setMeshPrefixVisible(Registry& registry, std::string_view prefix, bool visible) {
    const std::string prefixStr{prefix};
    for (auto [name, mesh] : registry.getEntries<rendering::RenderMeshInstance>()) {
        if (mesh != nullptr && name.starts_with(prefixStr)) {
            mesh->visible = visible;
        }
    }
}

inline void setupSystem(Registry& registry) {
    GlijbaanState state{};
    state.path = buildGlijbaanPath();
    state.glijbaanCubicBezierSamples = buildCenterLineSamples(kHalfPipeCenterlineSamplesPerSegment);
    buildHalfPipeMesh(state.glijbaanCubicBezierSamples, kHalfPipeRadius, kHalfPipeWallThickness,
                      kHalfPipeRingSegments, state.halfPipeVertices, state.halfPipeIndices);

    if (state.glijbaanCubicBezierSamples.empty()) {
        LOG_WARN("Glijbaan path has no samples");
        return;
    }

    const float worldExtent = maxExtent(state.glijbaanCubicBezierSamples);

    {
        asset::RenderObjectSpawnRequest request{};
        request.namePrefix = std::string(kDebugCurvePrefix);
        request.vertexShaderPath = "assets/shaders/raw_vert.vert";
        request.fragmentShaderPath = "assets/shaders/raw_vert.frag";
        request.worldPosition = glm::vec3{0.0f};
        request.uniformTargetSize = worldExtent;
        request.centerModel = false;
        request.layer = rendering::RenderLayer::DebugOverlay;
        request.enableFrustumCull = false;
        request.spawnFromRawVertices = true;
        request.rawVertices = &state.glijbaanCubicBezierSamples;

        const asset::RenderObjectSpawnResult result =
            asset::spawnModelAsRenderMeshes(registry, request);
        if (!result.error.empty()) {
            LOG_WARN("Glijbaan debug curve spawn failed: {}", result.error);
        }
    }

    if (!state.halfPipeVertices.empty() && !state.halfPipeIndices.empty()) {
        const float pipeExtent = maxExtent(state.halfPipeVertices);

        asset::RenderObjectSpawnRequest request{};
        request.namePrefix = std::string(kPipeMeshPrefix);
        request.vertexShaderPath = "assets/shaders/default.vert";
        request.fragmentShaderPath = "assets/shaders/default.frag";
        request.overrideTexturePath = "assets/textures/glijbaan.png";
        request.useModelMaterialTexture = false;
        request.worldPosition = glm::vec3{0.0f};
        request.uniformTargetSize = pipeExtent;
        request.centerModel = false;
        request.layer = rendering::RenderLayer::Opaque;
        request.enableFrustumCull = false;
        request.spawnFromRawMesh = true;
        request.rawMeshVertices = &state.halfPipeVertices;
        request.rawMeshIndices = &state.halfPipeIndices;

        const asset::RenderObjectSpawnResult result =
            asset::spawnModelAsRenderMeshes(registry, request);
        if (!result.error.empty()) {
            LOG_WARN("Glijbaan half-pipe spawn failed: {}", result.error);
        } else {
            LOG_INFO("Spawned glijbaan half-pipe ({} vertices, {} indices)",
                     state.halfPipeVertices.size(), state.halfPipeIndices.size());
        }
    }

    setMeshPrefixVisible(registry, kDebugCurvePrefix, state.showCurveDebug);

    spawnRiderModel(registry, state);
    initializeRiderEntity(registry, state.path);
    if (auto* rider = registry.getObject<SlideRiderEntity>(camera::kSlideRiderEntityName);
        rider != nullptr) {
        syncRiderMeshes(registry, state, *rider);
    }

    const double pathLength = state.path.totalLength;
    const std::size_t segmentCount = state.path.segments.size();
    registry.registerObject(kGlijbaanStateName, std::move(state));

    LOG_INFO("Glijbaan path length: {:.1f} units, {} cubic segments", pathLength, segmentCount);
}

inline void inputSystem(Registry& registry) {
    auto* state = registry.getObject<GlijbaanState>(kGlijbaanStateName);
    const auto* windowState = registry.getObject<window::WindowState>(window::kMainWindowStateName);
    if (state == nullptr || windowState == nullptr || windowState->handle == nullptr) {
        return;
    }

    GLFWwindow* window = windowState->handle;

    const bool togglePressed = glfwGetKey(window, GLFW_KEY_V) == GLFW_PRESS;
    if (togglePressed && !state->toggleWasPressed) {
        state->showCurveDebug = !state->showCurveDebug;
        setMeshPrefixVisible(registry, kDebugCurvePrefix, state->showCurveDebug);
        LOG_INFO("Glijbaan curve debug {}", state->showCurveDebug ? "visible" : "hidden");
    }
    state->toggleWasPressed = togglePressed;

    const bool resetPressed = glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS;
    if (resetPressed && !state->resetWasPressed) {
        state->rideDistance = 0.0;
        LOG_INFO("Glijbaan ride reset to start");
    }
    state->resetWasPressed = resetPressed;

    const bool playPausePressed = glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS;
    static bool playPauseWasPressed = false;
    if (playPausePressed && !playPauseWasPressed) {
        state->autoRide = !state->autoRide;
        LOG_INFO("Glijbaan auto-ride {}", state->autoRide ? "on" : "paused");
    }
    playPauseWasPressed = playPausePressed;

    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
        state->rideSpeed = std::min(state->rideSpeed + 40.0f * 0.016f, 420.0f);
    }
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
        state->rideSpeed = std::max(state->rideSpeed - 40.0f * 0.016f, 20.0f);
    }
}

inline void rideSystem(Registry& registry) {
    auto* state = registry.getObject<GlijbaanState>(kGlijbaanStateName);
    auto* rider = registry.getObject<SlideRiderEntity>(camera::kSlideRiderEntityName);
    const auto* time = registry.getObject<App::Time>("app.time");
    const auto* windowState = registry.getObject<window::WindowState>(window::kMainWindowStateName);
    if (state == nullptr || rider == nullptr || time == nullptr || windowState == nullptr ||
        windowState->handle == nullptr || state->path.totalLength <= 0.0) {
        return;
    }

    GLFWwindow* window = windowState->handle;

    const auto* cameraState = registry.getObject<camera::CameraState>(camera::kCameraStateName);
    const bool slideCameraActive =
        cameraState != nullptr && cameraState->activeId == camera::CameraId::SlideFollow;

    const double arcDelta = static_cast<double>(state->rideSpeed) * time->deltaTime;
    if (state->autoRide) {
        state->rideDistance += arcDelta;
    } else if (slideCameraActive) {
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
            state->rideDistance += arcDelta;
        }
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
            state->rideDistance -= arcDelta;
        }
    }

    state->rideDistance = wrapArcLength(state->rideDistance, state->path.totalLength);

    rider->forward = tangentOnPath(state->path, state->rideDistance);
    glm::vec3 binormal{};
    computeFrameAt(rider->forward, rider->trackNormal, binormal);
    rider->position =
        seatPositionOnPath(state->path, state->rideDistance, rider->seatOffsetAlongNormal);
    syncRiderMeshes(registry, *state, *rider);
}

}  // namespace glijbaan

#endif  // CG_OPENGL_PROJECT_GLIJBAAN_HPP
