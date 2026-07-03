#include <Renderer/RayPlaneIntersection.hpp>
#include <gtest/gtest.h>

namespace {

constexpr double tolerance = 1.0e-12;

} // namespace

TEST(RayPlaneIntersectionTest, FindsIntersectionInFrontOfRay) {
    const renderer::Plane plane{linal::double3{0.0, 0.0, 0.0}, linal::double3{0.0, 0.0, 1.0}};
    linal::double3 result{0.0};

    const bool hit =
        renderer::ray_plane_intersection(linal::double3{1.0, 2.0, 5.0}, linal::double3{0.0, 0.0, -1.0}, plane, result);

    EXPECT_TRUE(hit);
    EXPECT_NEAR(1.0, result[0], tolerance);
    EXPECT_NEAR(2.0, result[1], tolerance);
    EXPECT_NEAR(0.0, result[2], tolerance);
}

TEST(RayPlaneIntersectionTest, RejectsParallelRay) {
    const renderer::Plane plane{linal::double3{0.0, 0.0, 0.0}, linal::double3{0.0, 0.0, 1.0}};
    linal::double3 result{9.0, 9.0, 9.0};

    const bool hit =
        renderer::ray_plane_intersection(linal::double3{0.0, 0.0, 5.0}, linal::double3{1.0, 0.0, 0.0}, plane, result);

    EXPECT_FALSE(hit);
    EXPECT_EQ((linal::double3{9.0, 9.0, 9.0}), result);
}

TEST(RayPlaneIntersectionTest, RejectsIntersectionBehindRayOrigin) {
    const renderer::Plane plane{linal::double3{0.0, 0.0, 0.0}, linal::double3{0.0, 0.0, 1.0}};
    linal::double3 result{9.0, 9.0, 9.0};

    const bool hit =
        renderer::ray_plane_intersection(linal::double3{0.0, 0.0, 5.0}, linal::double3{0.0, 0.0, 1.0}, plane, result);

    EXPECT_FALSE(hit);
    EXPECT_EQ((linal::double3{9.0, 9.0, 9.0}), result);
}
