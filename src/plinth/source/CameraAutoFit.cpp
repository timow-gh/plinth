#include "plinth/CameraAutoFit.hpp"
#include <algorithm>
#include <cmath>
#include <limits>

namespace renderer {
namespace {

constexpr double epsilon = 1.0e-9;
constexpr double halfScale = 0.5;
constexpr double nearPlaneMultiplier = 2.0;
constexpr double significantRecenterImprovement = 1.5;
constexpr double degreesToRadians = 3.14159265358979323846 / 180.0;
// Caps the far/near ratio so depth-buffer precision does not collapse when the
// scene is far from the camera. A standard [0,1] depth buffer keeps usable
// precision up to roughly this ratio; beyond it z-fighting appears. Reversed-Z
// would lift this limit, but the clamp is projection-format agnostic.
constexpr double maxDepthRatio = 1.0e4;

struct CameraFrame {
    linal::double3 forward{0.0};
    linal::double3 right{0.0};
    linal::double3 up{0.0};
};

struct CameraSpaceBounds {
    bool hasGeometry{false};
    double minX{std::numeric_limits<double>::max()};
    double maxX{std::numeric_limits<double>::lowest()};
    double minY{std::numeric_limits<double>::max()};
    double maxY{std::numeric_limits<double>::lowest()};
    double minZ{std::numeric_limits<double>::max()};
    double maxZ{std::numeric_limits<double>::lowest()};
    linal::double3 worldMin{std::numeric_limits<double>::max()};
    linal::double3 worldMax{std::numeric_limits<double>::lowest()};
};

[[nodiscard]]
CameraFrame make_camera_frame(const CameraAutoFitInput& input) {
    CameraFrame frame;
    frame.forward = linal::normalize(input.target - input.position);
    frame.right = linal::normalize(linal::cross(frame.forward, input.vertical));
    frame.up = linal::normalize(linal::cross(frame.right, frame.forward));
    return frame;
}

void add_vertex_to_bounds(CameraSpaceBounds& bounds,
                          const CameraFrame& frame,
                          const linal::double3& cameraPosition,
                          const linal::double3& point) {
    const linal::double3 toPoint = point - cameraPosition;
    const double cameraX = linal::dot(toPoint, frame.right);
    const double cameraY = linal::dot(toPoint, frame.up);
    const double cameraZ = linal::dot(toPoint, frame.forward);

    bounds.hasGeometry = true;
    bounds.minX = std::min(bounds.minX, cameraX);
    bounds.maxX = std::max(bounds.maxX, cameraX);
    bounds.minY = std::min(bounds.minY, cameraY);
    bounds.maxY = std::max(bounds.maxY, cameraY);
    bounds.minZ = std::min(bounds.minZ, cameraZ);
    bounds.maxZ = std::max(bounds.maxZ, cameraZ);

    for (linal::double3::size_type i = 0; i < 3; ++i) {
        bounds.worldMin[i] = std::min(bounds.worldMin[i], point[i]);
        bounds.worldMax[i] = std::max(bounds.worldMax[i], point[i]);
    }
}

void add_vertices_to_bounds(CameraSpaceBounds& bounds,
                            const CameraFrame& frame,
                            const linal::double3& cameraPosition,
                            std::span<const float> vertices) {
    for (std::size_t i = 0; i + 2 < vertices.size(); i += 3) {
        add_vertex_to_bounds(bounds,
                             frame,
                             cameraPosition,
                             linal::double3{static_cast<double>(vertices[i]),
                                            static_cast<double>(vertices[i + 1]),
                                            static_cast<double>(vertices[i + 2])});
    }
}

[[nodiscard]]
CameraSpaceBounds calculate_scene_bounds(std::span<const std::span<const float>> vertexPositionBuffers,
                                         const CameraFrame& frame,
                                         const linal::double3& cameraPosition) {
    CameraSpaceBounds bounds;
    for (const std::span<const float> vertices: vertexPositionBuffers) {
        add_vertices_to_bounds(bounds, frame, cameraPosition, vertices);
    }
    return bounds;
}

[[nodiscard]]
double get_world_radius(const CameraSpaceBounds& bounds) {
    return linal::length(bounds.worldMax - bounds.worldMin) * halfScale;
}

[[nodiscard]]
double get_perspective_required_delta(const CameraSpaceBounds& bounds,
                                      double tanHalfVerticalFov,
                                      double aspectRatio,
                                      double padding,
                                      double nearPlane) {
    const double tanHalfHorizontalFov = tanHalfVerticalFov * aspectRatio;
    const double maxAbsX = std::max(std::abs(bounds.minX), std::abs(bounds.maxX));
    const double maxAbsY = std::max(std::abs(bounds.minY), std::abs(bounds.maxY));

    double requiredDelta = nearPlane - bounds.minZ + nearPlane;
    if (maxAbsX > epsilon) {
        requiredDelta = std::max(requiredDelta, (maxAbsX * padding / tanHalfHorizontalFov) - bounds.minZ);
    }
    if (maxAbsY > epsilon) {
        requiredDelta = std::max(requiredDelta, (maxAbsY * padding / tanHalfVerticalFov) - bounds.minZ);
    }
    return std::max(0.0, requiredDelta);
}

[[nodiscard]]
double get_perspective_occupancy(const CameraSpaceBounds& bounds, double tanHalfVerticalFov, double aspectRatio) {
    const double minPositiveZ = std::max(bounds.minZ, epsilon);
    const double tanHalfHorizontalFov = tanHalfVerticalFov * aspectRatio;
    const double xOccupancy =
        std::max(std::abs(bounds.minX), std::abs(bounds.maxX)) / (minPositiveZ * tanHalfHorizontalFov);
    const double yOccupancy =
        std::max(std::abs(bounds.minY), std::abs(bounds.maxY)) / (minPositiveZ * tanHalfVerticalFov);
    return std::max(xOccupancy, yOccupancy);
}

[[nodiscard]]
double get_orthographic_occupancy(const CameraSpaceBounds& bounds,
                                  double orthographicWidth,
                                  double orthographicHeight,
                                  double aspectRatio) {
    const double halfWidth = std::max(orthographicWidth * aspectRatio * halfScale, epsilon);
    const double halfHeight = std::max(orthographicHeight * halfScale, epsilon);
    const double xOccupancy = std::max(std::abs(bounds.minX), std::abs(bounds.maxX)) / halfWidth;
    const double yOccupancy = std::max(std::abs(bounds.minY), std::abs(bounds.maxY)) / halfHeight;
    return std::max(xOccupancy, yOccupancy);
}

[[nodiscard]]
double get_required_orthographic_height(const CameraSpaceBounds& bounds, double aspectRatio, double padding) {
    const double maxAbsX = std::max(std::abs(bounds.minX), std::abs(bounds.maxX));
    const double maxAbsY = std::max(std::abs(bounds.minY), std::abs(bounds.maxY));
    return std::max(nearPlaneMultiplier * maxAbsY * padding, nearPlaneMultiplier * maxAbsX * padding / aspectRatio);
}

void pan_bounds_to_scene_center(CameraSpaceBounds& bounds) {
    const double centerX = (bounds.minX + bounds.maxX) * halfScale;
    const double centerY = (bounds.minY + bounds.maxY) * halfScale;
    bounds.minX -= centerX;
    bounds.maxX -= centerX;
    bounds.minY -= centerY;
    bounds.maxY -= centerY;
}

[[nodiscard]]
bool should_pan_to_scene_center(const CameraSpaceBounds& bounds,
                                const CameraAutoFitInput& input,
                                double requiredDeltaWithoutPan,
                                double requiredDeltaWithPan) {
    if (requiredDeltaWithoutPan <= epsilon) {
        return false;
    }

    const double centerX = (bounds.minX + bounds.maxX) * halfScale;
    const double centerY = (bounds.minY + bounds.maxY) * halfScale;

    if (input.projectionType == CameraProjectionType::ORTHOGRAPHIC) {
        const double halfWidth = input.orthographicWidth * input.aspectRatio * halfScale;
        const double halfHeight = input.orthographicHeight * halfScale;
        return std::abs(centerX) > halfWidth || std::abs(centerY) > halfHeight ||
               requiredDeltaWithoutPan > requiredDeltaWithPan * significantRecenterImprovement;
    }

    return requiredDeltaWithoutPan > requiredDeltaWithPan * significantRecenterImprovement;
}

void pan_result_to_bounds_center(CameraAutoFitResult& result,
                                 CameraSpaceBounds& bounds,
                                 const CameraSpaceBounds& centeredBounds,
                                 const CameraFrame& frame) {
    const double centerX = (bounds.minX + bounds.maxX) * halfScale;
    const double centerY = (bounds.minY + bounds.maxY) * halfScale;
    const linal::double3 pan = frame.right * centerX + frame.up * centerY;
    result.position += pan;
    result.target += pan;
    bounds = centeredBounds;
    result.panned = true;
}

// Fraction of the scene's front depth used as an adaptive near floor. Scaling
// the floor with how far the scene sits from the camera keeps depth-buffer
// precision reasonable for distant scenes without forcing a tiny fixed near
// plane. It stays well below frontDepth * (1 - halfScale^2) so it never pushes
// the near plane past the nearest geometry.
constexpr double sceneScaleNearFloorFraction = 1.0e-4;

// Adaptive near floor derived purely from the scene's camera-space geometry, so
// it carries no per-frame history (unlike feeding back the camera's current
// near plane, which ratchets the near plane upward and clips close geometry
// after a zoom-out/zoom-in cycle). \p absoluteMinimum is the smallest near
// plane allowed regardless of scene scale.
[[nodiscard]]
double scene_scale_near_floor(const CameraSpaceBounds& bounds, double sceneRadius, double absoluteMinimum) {
    const double sceneScale = std::max(bounds.minZ, sceneRadius);
    return std::max(absoluteMinimum, sceneScale * sceneScaleNearFloorFraction);
}

// Computes the fitted near plane after the camera has been pushed back by
// \p movementDelta along its forward axis. The nearest geometry sits at
// camera-space depth (bounds.minZ + movementDelta); pull the plane a little
// closer so surfaces exactly at that depth are not clipped, keep it ahead of
// the scene-scale near floor, and cap the far/near ratio for depth precision.
[[nodiscard]]
double fit_near_plane(const CameraSpaceBounds& bounds,
                      double movementDelta,
                      double sceneRadius,
                      double absoluteMinimum,
                      double farPlane) {
    const double frontDepth = bounds.minZ + movementDelta;
    const double nearFloor = scene_scale_near_floor(bounds, sceneRadius, absoluteMinimum);
    double nearPlane = std::max(nearFloor, frontDepth * (1.0 - halfScale * halfScale));
    nearPlane = std::max(nearPlane, farPlane / maxDepthRatio);
    return nearPlane;
}

void apply_perspective_auto_fit(CameraAutoFitResult& result,
                                CameraSpaceBounds& bounds,
                                const CameraAutoFitInput& input,
                                const CameraFrame& frame,
                                double sceneRadius,
                                double farPadding,
                                double& movementDelta) {
    const double tanHalfVerticalFov = std::tan(input.verticalFovDegrees * halfScale * degreesToRadians);
    const double requiredDeltaWithoutPan = get_perspective_required_delta(bounds,
                                                                          tanHalfVerticalFov,
                                                                          input.aspectRatio,
                                                                          input.settings.zoomOutPadding,
                                                                          input.nearPlane);

    CameraSpaceBounds centeredBounds = bounds;
    pan_bounds_to_scene_center(centeredBounds);
    const double requiredDeltaWithPan = get_perspective_required_delta(centeredBounds,
                                                                       tanHalfVerticalFov,
                                                                       input.aspectRatio,
                                                                       input.settings.zoomOutPadding,
                                                                       input.nearPlane);

    if (should_pan_to_scene_center(bounds, input, requiredDeltaWithoutPan, requiredDeltaWithPan)) {
        pan_result_to_bounds_center(result, bounds, centeredBounds, frame);
    }

    movementDelta = get_perspective_required_delta(bounds,
                                                   tanHalfVerticalFov,
                                                   input.aspectRatio,
                                                   input.settings.zoomOutPadding,
                                                   input.nearPlane);
    if (movementDelta > epsilon) {
        result.zoomedOut = true;
    } else if (input.settings.zoomInEnabled && !input.suppressZoomIn && sceneRadius > epsilon) {
        result.viewportOccupancy = get_perspective_occupancy(bounds, tanHalfVerticalFov, input.aspectRatio);
        if (result.viewportOccupancy < input.settings.minViewportOccupancy) {
            const double centerZ = (bounds.minZ + bounds.maxZ) * halfScale;
            const double desiredCenterZ = sceneRadius * input.settings.zoomOutPadding /
                                          (input.settings.targetViewportOccupancy * tanHalfVerticalFov);
            movementDelta = std::max(desiredCenterZ - centerZ, input.nearPlane - bounds.minZ + input.nearPlane);
            if (movementDelta < -epsilon) {
                result.zoomedIn = true;
            } else {
                movementDelta = 0.0;
            }
        }
    }

    result.position -= frame.forward * movementDelta;
    result.farPlane = std::max(input.nearPlane * nearPlaneMultiplier, bounds.maxZ + movementDelta + farPadding);
    result.nearPlane = fit_near_plane(bounds, movementDelta, sceneRadius, input.nearPlane, result.farPlane);
    result.viewportOccupancy = get_perspective_occupancy(bounds, tanHalfVerticalFov, input.aspectRatio);
}

void apply_orthographic_auto_fit(CameraAutoFitResult& result,
                                 CameraSpaceBounds& bounds,
                                 const CameraAutoFitInput& input,
                                 const CameraFrame& frame,
                                 double sceneRadius,
                                 double farPadding,
                                 double& movementDelta) {
    const double requiredHeightWithoutPan =
        get_required_orthographic_height(bounds, input.aspectRatio, input.settings.zoomOutPadding);
    CameraSpaceBounds centeredBounds = bounds;
    pan_bounds_to_scene_center(centeredBounds);
    const double requiredHeightWithPan =
        get_required_orthographic_height(centeredBounds, input.aspectRatio, input.settings.zoomOutPadding);

    if (should_pan_to_scene_center(bounds,
                                   input,
                                   requiredHeightWithoutPan - input.orthographicHeight,
                                   requiredHeightWithPan - input.orthographicHeight)) {
        pan_result_to_bounds_center(result, bounds, centeredBounds, frame);
    }

    double targetHeight = get_required_orthographic_height(bounds, input.aspectRatio, input.settings.zoomOutPadding);
    if (targetHeight > input.orthographicHeight + epsilon) {
        result.zoomedOut = true;
        result.orthographicHeight = targetHeight;
        result.orthographicWidth = targetHeight;
    } else {
        result.viewportOccupancy =
            get_orthographic_occupancy(bounds, input.orthographicWidth, input.orthographicHeight, input.aspectRatio);
        if (input.settings.zoomInEnabled && !input.suppressZoomIn && sceneRadius > epsilon &&
            result.viewportOccupancy < input.settings.minViewportOccupancy) {
            targetHeight =
                get_required_orthographic_height(bounds, input.aspectRatio, input.settings.targetViewportOccupancy);
            if (targetHeight > epsilon && targetHeight < input.orthographicHeight) {
                result.zoomedIn = true;
                result.orthographicHeight = targetHeight;
                result.orthographicWidth = targetHeight;
            }
        }
    }

    movementDelta = std::max(0.0, input.nearPlane - bounds.minZ + input.nearPlane);
    result.position -= frame.forward * movementDelta;
    result.farPlane = std::max(input.nearPlane * nearPlaneMultiplier, bounds.maxZ + movementDelta + farPadding);
    result.nearPlane = fit_near_plane(bounds, movementDelta, sceneRadius, input.nearPlane, result.farPlane);
    result.viewportOccupancy =
        get_orthographic_occupancy(bounds, result.orthographicWidth, result.orthographicHeight, input.aspectRatio);
}

} // namespace

CameraAutoFitResult calculate_camera_auto_fit(std::span<const std::span<const float>> vertexPositionBuffers,
                                              const CameraAutoFitInput& input) {
    CameraAutoFitResult result;
    result.position = input.position;
    result.target = input.target;
    result.vertical = input.vertical;
    result.orthographicWidth = input.orthographicWidth;
    result.orthographicHeight = input.orthographicHeight;
    result.nearPlane = input.nearPlane;

    if (!input.settings.enabled) {
        return result;
    }

    const CameraFrame frame = make_camera_frame(input);
    CameraSpaceBounds bounds = calculate_scene_bounds(vertexPositionBuffers, frame, input.position);
    if (!bounds.hasGeometry) {
        return result;
    }

    result.hasGeometry = true;
    const double sceneRadius = get_world_radius(bounds);
    const double farPadding = std::max(sceneRadius, 1.0) * input.farPlaneMultiplier;

    double movementDelta = 0.0;
    if (input.projectionType == CameraProjectionType::PERSPECTIVE) {
        apply_perspective_auto_fit(result, bounds, input, frame, sceneRadius, farPadding, movementDelta);
    } else {
        apply_orthographic_auto_fit(result, bounds, input, frame, sceneRadius, farPadding, movementDelta);
    }

    result.changed = result.zoomedOut || result.zoomedIn || result.panned || std::abs(movementDelta) > epsilon;
    return result;
}

CameraClipPlanes calculate_clip_planes(std::span<const std::span<const float>> vertexPositionBuffers,
                                       const CameraAutoFitInput& input) {
    CameraClipPlanes planes;
    planes.nearPlane = input.nearPlane;

    const CameraFrame frame = make_camera_frame(input);
    const CameraSpaceBounds bounds = calculate_scene_bounds(vertexPositionBuffers, frame, input.position);
    if (!bounds.hasGeometry) {
        return planes;
    }
    planes.hasGeometry = true;

    // Fixed pose: the camera does not move, so movementDelta is zero. Bracket the
    // geometry's camera-space depth range with the same near/far math the
    // auto-fit uses, keeping the near floor and far/near ratio clamp.
    const double sceneRadius = get_world_radius(bounds);
    const double farPadding = std::max(sceneRadius, 1.0) * input.farPlaneMultiplier;
    planes.farPlane = std::max(input.nearPlane * nearPlaneMultiplier, bounds.maxZ + farPadding);
    planes.nearPlane = fit_near_plane(bounds, 0.0, sceneRadius, input.nearPlane, planes.farPlane);
    return planes;
}

} // namespace renderer
