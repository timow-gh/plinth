#include "plinth/CameraAutoFit.hpp"
#include <array>
#include <cmath>
#include <cstdint>
#include <gtest/gtest.h>
#include <span>
#include <vector>

namespace {

renderer::CameraAutoFitInput make_input() {
    renderer::CameraAutoFitInput input;
    input.position = linal::double3{0.0, 0.0, 10.0};
    input.target = linal::double3{0.0, 0.0, 0.0};
    input.vertical = linal::double3{0.0, 1.0, 0.0};
    input.aspectRatio = 1.0;
    input.verticalFovDegrees = 30.0;
    input.nearPlane = 0.01;
    input.farPlaneMultiplier = 3.0;
    return input;
}

bool is_finite3(const linal::double3& v) {
    return std::isfinite(v[0]) && std::isfinite(v[1]) && std::isfinite(v[2]);
}

} // namespace

TEST(CameraAutoFitTest, ZoomsOutWhenGeometryIsOutsideHorizontalFrustum) {
    const std::vector<float> lineVertices{-100.0f, 0.0f, 0.0f, 100.0f, 0.0f, 0.0f};
    const std::array<std::span<const float>, 1> vertexPositionBuffers{std::span<const float>{lineVertices}};

    const renderer::CameraAutoFitInput input = make_input();
    const renderer::CameraAutoFitResult result =
        renderer::calculate_camera_auto_fit(std::span<const std::span<const float>>{vertexPositionBuffers}, input);

    EXPECT_TRUE(result.hasGeometry);
    EXPECT_TRUE(result.zoomedOut);
    EXPECT_TRUE(result.changed);
    EXPECT_GT(result.position[2], input.position[2]);
    EXPECT_GT(result.farPlane, input.nearPlane);
}

TEST(CameraAutoFitTest, RepositionsSafelyWhenGeometryStartsBehindCamera) {
    const std::vector<float> pointVertices{0.0f, 0.0f, 20.0f};
    const std::array<std::span<const float>, 1> vertexPositionBuffers{std::span<const float>{pointVertices}};

    const renderer::CameraAutoFitInput input = make_input();
    const renderer::CameraAutoFitResult result =
        renderer::calculate_camera_auto_fit(std::span<const std::span<const float>>{vertexPositionBuffers}, input);

    EXPECT_TRUE(result.hasGeometry);
    EXPECT_TRUE(result.zoomedOut);
    EXPECT_TRUE(result.changed);
    EXPECT_GT(result.position[2], 20.0);
    EXPECT_NE(result.position, result.target);
}

TEST(CameraAutoFitTest, ZoomsInWhenSceneOccupiesTooLittleViewport) {
    const std::vector<float> lineVertices{-1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f};
    const std::array<std::span<const float>, 1> vertexPositionBuffers{std::span<const float>{lineVertices}};

    renderer::CameraAutoFitInput input = make_input();
    input.position = linal::double3{0.0, 0.0, 100.0};
    input.target = linal::double3{0.0, 0.0, 0.0};

    const renderer::CameraAutoFitResult result =
        renderer::calculate_camera_auto_fit(std::span<const std::span<const float>>{vertexPositionBuffers}, input);

    EXPECT_TRUE(result.hasGeometry);
    EXPECT_TRUE(result.zoomedIn);
    EXPECT_TRUE(result.changed);
    EXPECT_LT(result.position[2], input.position[2]);
}

TEST(CameraAutoFitTest, MeshOnlySceneContributesToBounds) {
    const std::vector<float> lineVertices{-1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f};
    const std::array<std::span<const float>, 1> vertexPositionBuffers{std::span<const float>{lineVertices}};

    renderer::CameraAutoFitInput input = make_input();
    input.position = linal::double3{0.0, 0.0, 100.0};
    input.target = linal::double3{0.0, 0.0, 0.0};

    const renderer::CameraAutoFitResult result =
        renderer::calculate_camera_auto_fit(std::span<const std::span<const float>>{vertexPositionBuffers}, input);

    EXPECT_TRUE(result.hasGeometry);
    EXPECT_TRUE(result.zoomedIn);
    EXPECT_TRUE(result.changed);
    EXPECT_LT(result.position[2], input.position[2]);
}

TEST(CameraAutoFitTest, SuppressesZoomInAfterRecentCameraInteraction) {
    const std::vector<float> lineVertices{-1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f};
    const std::array<std::span<const float>, 1> vertexPositionBuffers{std::span<const float>{lineVertices}};

    renderer::CameraAutoFitInput input = make_input();
    input.position = linal::double3{0.0, 0.0, 100.0};
    input.target = linal::double3{0.0, 0.0, 0.0};
    input.suppressZoomIn = true;

    const renderer::CameraAutoFitResult result =
        renderer::calculate_camera_auto_fit(std::span<const std::span<const float>>{vertexPositionBuffers}, input);

    EXPECT_TRUE(result.hasGeometry);
    EXPECT_FALSE(result.zoomedIn);
    EXPECT_FALSE(result.changed);
}

TEST(CameraAutoFitTest, SuppressionDoesNotBlockZoomOut) {
    const std::vector<float> lineVertices{-100.0f, 0.0f, 0.0f, 100.0f, 0.0f, 0.0f};
    const std::array<std::span<const float>, 1> vertexPositionBuffers{std::span<const float>{lineVertices}};

    renderer::CameraAutoFitInput input = make_input();
    input.suppressZoomIn = true;

    const renderer::CameraAutoFitResult result =
        renderer::calculate_camera_auto_fit(std::span<const std::span<const float>>{vertexPositionBuffers}, input);

    EXPECT_TRUE(result.hasGeometry);
    EXPECT_TRUE(result.zoomedOut);
    EXPECT_TRUE(result.changed);
}

TEST(CameraAutoFitTest, ReturnsUnchangedResultWhenDisabled) {
    const std::vector<float> pointVertices{100.0f, 0.0f, 0.0f};
    const std::array<std::span<const float>, 1> vertexPositionBuffers{std::span<const float>{pointVertices}};

    renderer::CameraAutoFitInput input = make_input();
    input.settings.enabled = false;

    const renderer::CameraAutoFitResult result =
        renderer::calculate_camera_auto_fit(std::span<const std::span<const float>>{vertexPositionBuffers}, input);

    EXPECT_FALSE(result.hasGeometry);
    EXPECT_FALSE(result.changed);
    EXPECT_EQ(result.position, input.position);
    EXPECT_EQ(result.target, input.target);
}

TEST(CameraAutoFitTest, ReturnsUnchangedResultForEmptyScene) {
    const std::array<std::span<const float>, 0> vertexPositionBuffers{};
    const renderer::CameraAutoFitInput input = make_input();

    const renderer::CameraAutoFitResult result =
        renderer::calculate_camera_auto_fit(std::span<const std::span<const float>>{vertexPositionBuffers}, input);

    EXPECT_FALSE(result.hasGeometry);
    EXPECT_FALSE(result.changed);
    EXPECT_EQ(result.position, input.position);
    EXPECT_EQ(result.target, input.target);
}

TEST(CameraAutoFitTest, OrthographicZoomsOutWhenGeometryExceedsViewport) {
    const std::vector<float> lineVertices{-100.0f, 0.0f, 0.0f, 100.0f, 0.0f, 0.0f};
    const std::array<std::span<const float>, 1> vertexPositionBuffers{std::span<const float>{lineVertices}};

    renderer::CameraAutoFitInput input = make_input();
    input.projectionType = renderer::CameraProjectionType::ORTHOGRAPHIC;
    input.orthographicWidth = 10.0;
    input.orthographicHeight = 10.0;

    const renderer::CameraAutoFitResult result =
        renderer::calculate_camera_auto_fit(std::span<const std::span<const float>>{vertexPositionBuffers}, input);

    EXPECT_TRUE(result.hasGeometry);
    EXPECT_TRUE(result.zoomedOut);
    EXPECT_TRUE(result.changed);
    EXPECT_GT(result.orthographicHeight, input.orthographicHeight);
    EXPECT_DOUBLE_EQ(result.orthographicWidth, result.orthographicHeight);
}

TEST(CameraAutoFitTest, OrthographicZoomsInWhenSceneOccupiesTooLittleViewport) {
    const std::vector<float> lineVertices{-1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f};
    const std::array<std::span<const float>, 1> vertexPositionBuffers{std::span<const float>{lineVertices}};

    renderer::CameraAutoFitInput input = make_input();
    input.projectionType = renderer::CameraProjectionType::ORTHOGRAPHIC;
    input.orthographicWidth = 100.0;
    input.orthographicHeight = 100.0;

    const renderer::CameraAutoFitResult result =
        renderer::calculate_camera_auto_fit(std::span<const std::span<const float>>{vertexPositionBuffers}, input);

    EXPECT_TRUE(result.hasGeometry);
    EXPECT_TRUE(result.zoomedIn);
    EXPECT_TRUE(result.changed);
    EXPECT_LT(result.orthographicHeight, input.orthographicHeight);
    EXPECT_GT(result.viewportOccupancy, 0.0);
}

TEST(CameraAutoFitTest, OrthographicSuppressesZoomInAfterRecentCameraInteraction) {
    const std::vector<float> lineVertices{-1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f};
    const std::array<std::span<const float>, 1> vertexPositionBuffers{std::span<const float>{lineVertices}};

    renderer::CameraAutoFitInput input = make_input();
    input.projectionType = renderer::CameraProjectionType::ORTHOGRAPHIC;
    input.orthographicWidth = 100.0;
    input.orthographicHeight = 100.0;
    input.suppressZoomIn = true;

    const renderer::CameraAutoFitResult result =
        renderer::calculate_camera_auto_fit(std::span<const std::span<const float>>{vertexPositionBuffers}, input);

    EXPECT_TRUE(result.hasGeometry);
    EXPECT_FALSE(result.zoomedIn);
    EXPECT_FALSE(result.changed);
    EXPECT_DOUBLE_EQ(result.orthographicHeight, input.orthographicHeight);
}

TEST(CameraAutoFitTest, OrthographicPansToOffCenterGeometry) {
    const std::vector<float> pointVertices{100.0f, 0.0f, 0.0f};
    const std::array<std::span<const float>, 1> vertexPositionBuffers{std::span<const float>{pointVertices}};

    renderer::CameraAutoFitInput input = make_input();
    input.projectionType = renderer::CameraProjectionType::ORTHOGRAPHIC;
    input.orthographicWidth = 50.0;
    input.orthographicHeight = 50.0;

    const renderer::CameraAutoFitResult result =
        renderer::calculate_camera_auto_fit(std::span<const std::span<const float>>{vertexPositionBuffers}, input);

    EXPECT_TRUE(result.hasGeometry);
    EXPECT_TRUE(result.panned);
    EXPECT_TRUE(result.changed);
    EXPECT_GT(result.position[0], input.position[0]);
    EXPECT_GT(result.target[0], input.target[0]);
}

TEST(CameraAutoFitTest, EmptyBuffersReportNoGeometryAndStayFinite) {
    const std::array<std::span<const float>, 0> vertexPositionBuffers{};
    const renderer::CameraAutoFitInput input = make_input();

    const renderer::CameraAutoFitResult result =
        renderer::calculate_camera_auto_fit(std::span<const std::span<const float>>{vertexPositionBuffers}, input);

    EXPECT_FALSE(result.hasGeometry);
    EXPECT_FALSE(result.changed);
    EXPECT_TRUE(is_finite3(result.position));
    EXPECT_TRUE(is_finite3(result.target));
}

TEST(CameraAutoFitTest, SinglePointProducesFiniteResult) {
    const std::vector<float> pointVertices{5.0f, 5.0f, 5.0f};
    const std::array<std::span<const float>, 1> vertexPositionBuffers{std::span<const float>{pointVertices}};

    const renderer::CameraAutoFitInput input = make_input();
    const renderer::CameraAutoFitResult result =
        renderer::calculate_camera_auto_fit(std::span<const std::span<const float>>{vertexPositionBuffers}, input);

    EXPECT_TRUE(is_finite3(result.position));
    EXPECT_TRUE(is_finite3(result.target));
    EXPECT_TRUE(std::isfinite(result.orthographicWidth));
    EXPECT_TRUE(std::isfinite(result.orthographicHeight));
}

TEST(CameraAutoFitTest, CoincidentPointsProduceFiniteResult) {
    const std::vector<float> pointVertices{
        3.0f, 3.0f, 3.0f, 3.0f, 3.0f, 3.0f, 3.0f, 3.0f, 3.0f, 3.0f, 3.0f, 3.0f};
    const std::array<std::span<const float>, 1> vertexPositionBuffers{std::span<const float>{pointVertices}};

    const renderer::CameraAutoFitInput input = make_input();
    const renderer::CameraAutoFitResult result =
        renderer::calculate_camera_auto_fit(std::span<const std::span<const float>>{vertexPositionBuffers}, input);

    EXPECT_TRUE(is_finite3(result.position));
    EXPECT_TRUE(is_finite3(result.target));
    EXPECT_TRUE(std::isfinite(result.orthographicWidth));
    EXPECT_TRUE(std::isfinite(result.orthographicHeight));
}

TEST(CameraAutoFitTest, ClipPlanesBracketGeometryWithoutMovingCamera) {
    // A box spanning world z in [-10, 10]; a camera at z=100 looking toward -z
    // sees it at camera-space depth [90, 110].
    const std::vector<float> boxVertices{
        -5.0f, -5.0f, -10.0f, 5.0f, 5.0f, 10.0f, -5.0f, 5.0f, 10.0f, 5.0f, -5.0f, -10.0f};
    const std::array<std::span<const float>, 1> vertexPositionBuffers{std::span<const float>{boxVertices}};

    renderer::CameraAutoFitInput input = make_input();
    input.position = linal::double3{0.0, 0.0, 100.0};
    input.target = linal::double3{0.0, 0.0, 0.0};

    const renderer::CameraClipPlanes planes =
        renderer::calculate_clip_planes(std::span<const std::span<const float>>{vertexPositionBuffers}, input);

    EXPECT_TRUE(planes.hasGeometry);
    // Near in front of nearest geometry (depth 90), far behind farthest (110).
    EXPECT_LT(planes.nearPlane, 90.0);
    EXPECT_GT(planes.farPlane, 110.0);
    EXPECT_GE(planes.nearPlane, input.nearPlane);
}

TEST(CameraAutoFitTest, ClipPlanesCapFarNearRatioForDistantGeometry) {
    // A tiny object very far away would otherwise need a huge far with the near
    // floor at 0.01, giving a ratio of ~1e6. The clamp must keep it bounded.
    const std::vector<float> pointVertices{0.0f, 0.0f, 0.0f, 0.1f, 0.1f, 0.1f};
    const std::array<std::span<const float>, 1> vertexPositionBuffers{std::span<const float>{pointVertices}};

    renderer::CameraAutoFitInput input = make_input();
    input.position = linal::double3{0.0, 0.0, 10000.0};
    input.target = linal::double3{0.0, 0.0, 0.0};

    const renderer::CameraClipPlanes planes =
        renderer::calculate_clip_planes(std::span<const std::span<const float>>{vertexPositionBuffers}, input);

    EXPECT_TRUE(planes.hasGeometry);
    EXPECT_GT(planes.nearPlane, 0.0);
    // Ratio clamp is 1e4 in the implementation; allow a small margin.
    EXPECT_LE(planes.farPlane / planes.nearPlane, 1.0e4 + 1.0);
    // Geometry must still be inside: nearest depth ~9999.83 must exceed near.
    EXPECT_LT(planes.nearPlane, 9999.0);
    EXPECT_GT(planes.farPlane, 10000.0);
}

TEST(CameraAutoFitTest, ClipPlanesReportNoGeometryForEmptyScene) {
    const std::array<std::span<const float>, 0> vertexPositionBuffers{};
    const renderer::CameraAutoFitInput input = make_input();

    const renderer::CameraClipPlanes planes =
        renderer::calculate_clip_planes(std::span<const std::span<const float>>{vertexPositionBuffers}, input);

    EXPECT_FALSE(planes.hasGeometry);
    EXPECT_DOUBLE_EQ(planes.nearPlane, input.nearPlane);
}

TEST(CameraAutoFitTest, ClipPlanesDoNotRatchetAcrossZoomOutThenZoomIn) {
    // Regression for the disappearing-mesh bug: fit clip planes while zoomed far
    // out from the geometry, then fit again while zoomed in close. The zoomed-in
    // near plane must sit in front of the nearest geometry so the mesh is not
    // clipped. Previously the fitter used the camera's current near plane as its
    // floor, so the large zoomed-out near plane ratcheted forward and clipped the
    // geometry after zooming back in.
    const std::vector<float> boxVertices{
        -5.0f, -5.0f, -5.0f, 5.0f, 5.0f, 5.0f, -5.0f, 5.0f, 5.0f, 5.0f, -5.0f, -5.0f};
    const std::array<std::span<const float>, 1> vertexPositionBuffers{std::span<const float>{boxVertices}};

    // Zoomed far out: camera at z = 1000, geometry front at camera-space depth ~995.
    renderer::CameraAutoFitInput zoomedOut = make_input();
    zoomedOut.position = linal::double3{0.0, 0.0, 1000.0};
    zoomedOut.target = linal::double3{0.0, 0.0, 0.0};
    const renderer::CameraClipPlanes farPlanes =
        renderer::calculate_clip_planes(std::span<const std::span<const float>>{vertexPositionBuffers}, zoomedOut);
    ASSERT_TRUE(farPlanes.hasGeometry);

    // Zoomed in close: camera at z = 20, geometry front at camera-space depth ~15.
    // calculate_clip_planes is stateless w.r.t. the previous near plane, so the
    // large farPlanes.nearPlane must not leak into this fit.
    renderer::CameraAutoFitInput zoomedIn = make_input();
    zoomedIn.position = linal::double3{0.0, 0.0, 20.0};
    zoomedIn.target = linal::double3{0.0, 0.0, 0.0};
    const renderer::CameraClipPlanes nearPlanes =
        renderer::calculate_clip_planes(std::span<const std::span<const float>>{vertexPositionBuffers}, zoomedIn);
    ASSERT_TRUE(nearPlanes.hasGeometry);

    // Nearest geometry sits at camera-space depth 15 (z=20 looking to origin,
    // box front face at world z=5). The near plane must be in front of it.
    EXPECT_LT(nearPlanes.nearPlane, 15.0);
    // And the zoomed-in near plane must not have inherited the zoomed-out value.
    EXPECT_LT(nearPlanes.nearPlane, farPlanes.nearPlane);
}
