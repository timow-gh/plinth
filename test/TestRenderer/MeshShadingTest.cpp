#include "plinth/loader/MeshShading.hpp"

#include "plinth/loader/MeshData.hpp"
#include "plinth/loader/MeshLoader.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <gtest/gtest.h>
#include <string>
#include <string_view>

namespace {

using renderer::apply_shading;
using renderer::MeshData;
using renderer::ShadingMode;

// Two triangles forming a quad in the z=0 plane, sharing an edge. Corner
// vertices are shared between the triangles (indexed topology).
MeshData make_quad() {
    MeshData mesh;
    mesh.vertices = {
        0.0F,
        0.0F,
        0.0F, // 0
        1.0F,
        0.0F,
        0.0F, // 1
        1.0F,
        1.0F,
        0.0F, // 2
        0.0F,
        1.0F,
        0.0F, // 3
    };
    mesh.triangleIndices = {0U, 1U, 2U, 0U, 2U, 3U};
    return mesh;
}

TEST(MeshShadingTest, PreserveReturnsInputUnchanged) {
    const MeshData quad = make_quad();
    const MeshData out = apply_shading(quad, ShadingMode::Preserve);
    EXPECT_EQ(out.vertices, quad.vertices);
    EXPECT_EQ(out.triangleIndices, quad.triangleIndices);
    EXPECT_TRUE(out.normals.empty());
}

TEST(MeshShadingTest, FlatUnsharesVerticesAndAssignsFaceNormals) {
    const MeshData out = apply_shading(make_quad(), ShadingMode::Flat);
    // 2 triangles * 3 unshared vertices.
    EXPECT_EQ(out.vertices.size(), 2U * 3U * 3U);
    ASSERT_EQ(out.normals.size(), 2U * 3U * 3U);
    EXPECT_EQ(out.triangleIndices.size(), 6U);
    // Sequential (no reuse) indices.
    for (std::uint32_t i = 0; i < out.triangleIndices.size(); ++i) {
        EXPECT_EQ(out.triangleIndices[i], i);
    }
    // Every normal points +z (the quad lies in z=0, wound CCW).
    for (std::size_t n = 0; n < out.normals.size(); n += 3U) {
        EXPECT_NEAR(out.normals[n + 0U], 0.0F, 1.0e-5F);
        EXPECT_NEAR(out.normals[n + 1U], 0.0F, 1.0e-5F);
        EXPECT_NEAR(out.normals[n + 2U], 1.0F, 1.0e-5F);
    }
}

TEST(MeshShadingTest, FlatOnCubeGivesAxisAlignedFaceNormals) {
    // A cube through the loader: 8 shared positions, 12 triangles.
    constexpr std::string_view cube =
        "v -0.5 -0.5 -0.5\nv 0.5 -0.5 -0.5\nv 0.5 0.5 -0.5\nv -0.5 0.5 -0.5\n"
        "v -0.5 -0.5 0.5\nv 0.5 -0.5 0.5\nv 0.5 0.5 0.5\nv -0.5 0.5 0.5\n"
        "f 4 3 2 1\nf 6 7 8 5\nf 2 6 5 1\nf 8 7 3 4\nf 5 8 4 1\nf 3 7 6 2\n";
    const auto smooth = renderer::load_mesh(".obj", std::string{cube});
    ASSERT_TRUE(smooth.has_value());
    // Smooth (preserve, no vn) leaves normals for the renderer => empty here,
    // and welds to 8 vertices.
    EXPECT_EQ(smooth->vertices.size(), 8U * 3U);

    const auto flat =
        renderer::load_mesh(".obj", std::string{cube}, renderer::MeshLoadOptions{.shading = ShadingMode::Flat});
    ASSERT_TRUE(flat.has_value());
    // 12 triangles * 3 unshared vertices.
    EXPECT_EQ(flat->vertices.size(), 12U * 3U * 3U);
    ASSERT_EQ(flat->normals.size(), 12U * 3U * 3U);
    // Each flat normal must be axis-aligned (one component +/-1, others 0).
    for (std::size_t n = 0; n < flat->normals.size(); n += 3U) {
        const float ax = std::abs(flat->normals[n + 0U]);
        const float ay = std::abs(flat->normals[n + 1U]);
        const float az = std::abs(flat->normals[n + 2U]);
        const float maxComponent = std::max({ax, ay, az});
        const float sum = ax + ay + az;
        EXPECT_NEAR(maxComponent, 1.0F, 1.0e-5F);
        EXPECT_NEAR(sum, 1.0F, 1.0e-5F); // exactly one non-zero component
    }
}

TEST(MeshShadingTest, SmoothWeldsSharedPositions) {
    // Start from an unshared (flat) quad and re-weld it.
    const MeshData flat = apply_shading(make_quad(), ShadingMode::Flat);
    EXPECT_EQ(flat.vertices.size(), 2U * 3U * 3U);
    const MeshData smooth = apply_shading(flat, ShadingMode::Smooth);
    // The quad has 4 distinct positions.
    EXPECT_EQ(smooth.vertices.size(), 4U * 3U);
    ASSERT_EQ(smooth.normals.size(), 4U * 3U);
    EXPECT_EQ(smooth.triangleIndices.size(), 6U);
    // All normals +z for a planar quad.
    for (std::size_t n = 0; n < smooth.normals.size(); n += 3U) {
        EXPECT_NEAR(smooth.normals[n + 2U], 1.0F, 1.0e-5F);
    }
}

TEST(MeshShadingTest, MeshWithoutTrianglesIsUnchanged) {
    MeshData points;
    points.vertices = {0.0F, 0.0F, 0.0F};
    const MeshData out = apply_shading(points, ShadingMode::Flat);
    EXPECT_EQ(out.vertices, points.vertices);
    EXPECT_TRUE(out.normals.empty());
}

} // namespace
