#include "PlinthTestMatchers.hpp"
#include <plinth/Camera.hpp>
#include <cmath>
#include <gtest/gtest.h>

namespace {

constexpr double tolerance = 1.0e-9;

renderer::Camera make_camera() {
    renderer::Camera camera;
    camera.look_at(glm::dvec3{0.0, 0.0, 10.0}, glm::dvec3{0.0, 0.0, 0.0}, glm::dvec3{0.0, 1.0, 0.0});
    camera.set_viewport(0, 0, 100, 100);
    return camera;
}

} // namespace

TEST(CameraTest, CreatesPerspectiveRaysThroughViewport) {
    renderer::Camera camera = make_camera();

    const renderer::PickRay centerRay = camera.get_center_ray();
    EXPECT_TRUE(Double3Near(centerRay.origin, linal::double3{0.0, 0.0, 10.0}, tolerance));
    EXPECT_TRUE(Double3Near(centerRay.direction, linal::double3{0.0, 0.0, -1.0}, tolerance));

    const renderer::PickRay ndcRay = camera.get_ray_from_ndc(0.0, 0.0);
    EXPECT_TRUE(Double3Near(ndcRay.origin, centerRay.origin, tolerance));
    EXPECT_TRUE(Double3Near(ndcRay.direction, centerRay.direction, tolerance));

    const renderer::PickRay cornerRay = camera.create_perspective_ray(100.0, 100.0);
    EXPECT_GT(cornerRay.direction[0], 0.0);
    EXPECT_GT(cornerRay.direction[1], 0.0);
    EXPECT_LT(cornerRay.direction[2], 0.0);
    EXPECT_NEAR(linal::length(cornerRay.direction), 1.0, tolerance);
}

TEST(CameraTest, CreatesOrthographicRaysWithOffsetOrigins) {
    renderer::Camera camera = make_camera();
    camera.set_projection_type(renderer::CameraProjectionType::ORTHOGRAPHIC);
    camera.set_orthographic_size(10.0, 10.0);
    camera.set_viewport(0, 0, 100, 50);

    const renderer::PickRay centerRay = camera.get_center_ray();
    EXPECT_TRUE(Double3Near(centerRay.origin, linal::double3{0.0, 0.0, 10.0}, tolerance));
    EXPECT_TRUE(Double3Near(centerRay.direction, linal::double3{0.0, 0.0, -1.0}, tolerance));

    const renderer::PickRay cornerRay = camera.create_orthographic_ray(100.0, 50.0);
    EXPECT_TRUE(Double3Near(cornerRay.origin, linal::double3{10.0, 5.0, 10.0}, tolerance));
    EXPECT_TRUE(Double3Near(cornerRay.direction, centerRay.direction, tolerance));
}

TEST(CameraTest, ConvertsScreenAndNormalizedCoordinates) {
    renderer::Camera camera = make_camera();
    camera.set_viewport(10, 20, 200, 100);

    const auto [ndcX, ndcY] = camera.screen_to_ndc(100.0, 50.0);
    EXPECT_NEAR(ndcX, 0.0, tolerance);
    EXPECT_NEAR(ndcY, 0.0, tolerance);

    const auto [screenX, screenY] = camera.ndc_to_screen(-0.5, 0.5);
    EXPECT_NEAR(screenX, 50.0, tolerance);
    EXPECT_NEAR(screenY, 75.0, tolerance);

    EXPECT_TRUE(camera.is_point_in_viewport(0.0, 0.0));
    EXPECT_TRUE(camera.is_point_in_viewport(199.0, 99.0));
    EXPECT_FALSE(camera.is_point_in_viewport(200.0, 100.0));
}

TEST(CameraTest, UpdatesPerspectiveAndOrthographicParameters) {
    renderer::Camera camera = make_camera();

    camera.set_perspective_params(renderer::Camera::PerspectiveParams{45.0, 0.5, 500.0});
    EXPECT_DOUBLE_EQ(camera.get_fov(), 45.0);
    EXPECT_DOUBLE_EQ(camera.get_near_plane(), 0.5);
    EXPECT_DOUBLE_EQ(camera.get_far_plane(), 500.0);

    camera.set_projection_type(renderer::CameraProjectionType::ORTHOGRAPHIC);
    camera.set_orthographic_params(renderer::Camera::OrthographicParams{20.0, 12.0, 0.25, 250.0});
    EXPECT_DOUBLE_EQ(camera.get_near_plane(), 0.25);
    EXPECT_DOUBLE_EQ(camera.get_far_plane(), 250.0);

    camera.set_near_plane(0.75);
    camera.set_far_plane(300.0);
    EXPECT_DOUBLE_EQ(camera.get_orthographic_params().near_plane, 0.75);
    EXPECT_DOUBLE_EQ(camera.get_orthographic_params().far_plane, 300.0);

    const glm::mat4 projection = camera.get_projection_matrix();
    const glm::mat4 orthographicProjection = camera.get_ortho_projection_matrix();
    EXPECT_NEAR(projection[0][0], orthographicProjection[0][0], 1.0e-6);
    (void)camera.get_ortho_mvp();
}

TEST(CameraTest, ZoomsPerspectiveAndOrthographicProjections) {
    renderer::Camera camera = make_camera();

    camera.zoom_perspective_projection(2.0);
    EXPECT_NEAR(camera.get_position().z, 8.0, tolerance);

    camera.set_projection_type(renderer::CameraProjectionType::ORTHOGRAPHIC);
    camera.set_orthographic_size(10.0, 10.0);
    camera.zoom_ortho_projection(1.0);
    EXPECT_DOUBLE_EQ(camera.get_orthographic_params().width, 5.0);
    EXPECT_DOUBLE_EQ(camera.get_orthographic_params().height, 5.0);
}
