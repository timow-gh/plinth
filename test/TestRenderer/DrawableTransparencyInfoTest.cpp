#include <OpenGL/Drawable/DrawableTransparencyInfo.hpp>
#include <gtest/gtest.h>
#include <linal/hmat.hpp>
#include <vector>

namespace {

linal::hmatf make_translation(float x, float y, float z) {
    linal::hmatf result = linal::hmatf::identity();
    result.set_translation(linal::float3{x, y, z});
    return result;
}

} // namespace

TEST(DrawableTransparencyInfoTest, HandlesEdgeCasesAndSortableCollections) {
    const std::vector<float> translucentColors = {
        1.0F,
        0.0F,
        0.0F,
        1.0F,
        0.0F,
        1.0F,
        0.0F,
        0.5F,
    };
    const std::vector<float> rgbColors = {
        1.0F,
        0.0F,
        0.0F,
        0.0F,
        1.0F,
        0.0F,
    };
    EXPECT_TRUE(opengl::contains_translucent_alpha(translucentColors, 4));
    EXPECT_FALSE(opengl::contains_translucent_alpha(rgbColors, 3));
    EXPECT_TRUE(opengl::make_vertex_translucency_flags(rgbColors, 3).empty());

    const opengl::DrawableTransparencyInfo info{true, linal::float3{1.0F, 2.0F, 3.0F}};
    EXPECT_DOUBLE_EQ(14.0, info.distance_squared_to(linal::double3{2.0, 4.0, 6.0}));

    const std::vector<float> vertices = {
        0.0F,
        0.0F,
        0.0F,
        2.0F,
        4.0F,
        6.0F,
    };
    const linal::float3 center = opengl::calc_sort_center(vertices, 3);
    EXPECT_FLOAT_EQ(1.0F, center[0]);
    EXPECT_FLOAT_EQ(2.0F, center[1]);
    EXPECT_FLOAT_EQ(3.0F, center[2]);

    const std::vector<float> incompleteVertex = {1.0F, 2.0F};
    EXPECT_FLOAT_EQ(0.0F, opengl::calc_sort_center(vertices, 2)[0]);
    EXPECT_FLOAT_EQ(0.0F, opengl::calc_sort_center(std::span<const float>{}, 3)[0]);
    EXPECT_FLOAT_EQ(0.0F, opengl::calc_sort_center(incompleteVertex, 3)[0]);
    EXPECT_TRUE(opengl::make_vertex_sort_positions(vertices, 2).empty());

    const std::vector<linal::float3> positions = {
        linal::float3{0.0F, 0.0F, 0.0F},
        linal::float3{1.0F, 0.0F, 0.0F},
    };
    EXPECT_FLOAT_EQ(0.0F, opengl::get_sort_position_or_origin(positions, 7U)[0]);

    const std::vector<std::uint32_t> pointIndices = {0U, 1U, 7U};
    const std::vector<std::uint8_t> vertexTranslucency = {0U, 1U};
    const opengl::PointTransparencyIndexSplit pointSplit =
        opengl::split_point_indices_by_transparency(pointIndices, positions, vertexTranslucency);
    ASSERT_EQ(2U, pointSplit.opaqueIndices.size());
    ASSERT_EQ(1U, pointSplit.translucentIndices.size());
    EXPECT_EQ(1U, opengl::flatten_translucent_point_indices(pointSplit.translucentIndices).front());

    const std::vector<std::uint32_t> lineIndices = {0U, 1U, 7U};
    const opengl::LineTransparencyIndexSplit lineSplit =
        opengl::split_line_indices_by_transparency(lineIndices, positions, vertexTranslucency);
    EXPECT_TRUE(lineSplit.opaqueIndices.empty());
    ASSERT_EQ(1U, lineSplit.translucentSegments.size());
    const std::vector<std::uint32_t> flatLines =
        opengl::flatten_translucent_line_indices(lineSplit.translucentSegments);
    ASSERT_EQ(2U, flatLines.size());
    EXPECT_EQ(0U, flatLines[0]);
    EXPECT_EQ(1U, flatLines[1]);

    const std::vector<opengl::SortablePointIndex> sortablePoints = {
        {1U, linal::float3{1.0F, 0.0F, 0.0F}},
        {2U, linal::float3{4.0F, 0.0F, 0.0F}},
    };
    const std::vector<std::uint32_t> sortedPoints =
        opengl::sort_translucent_point_indices_back_to_front(sortablePoints, linal::double3{0.0, 0.0, 0.0});
    EXPECT_EQ(2U, sortedPoints.front());

    const std::vector<opengl::SortableLineSegment> sortableLines = {
        {1U, 2U, linal::float3{1.0F, 0.0F, 0.0F}},
        {3U, 4U, linal::float3{4.0F, 0.0F, 0.0F}},
    };
    const std::vector<std::uint32_t> sortedLines =
        opengl::sort_translucent_line_indices_back_to_front(sortableLines, linal::double3{0.0, 0.0, 0.0});
    EXPECT_EQ(3U, sortedLines.front());
}

TEST(DrawableTransparencyInfoTransformTest, DistanceSquaredToTransformsSortCenterBeforeComparing) {
    const opengl::DrawableTransparencyInfo info{true, linal::float3{1.0F, 0.0F, 0.0F}};
    const linal::hmatf translation = make_translation(10.0F, 0.0F, 0.0F);

    const linal::double3 viewPosition{0.0, 0.0, 0.0};
    const double expected = 11.0 * 11.0;
    EXPECT_DOUBLE_EQ(expected, info.distance_squared_to(viewPosition, translation));
    EXPECT_NE(expected, info.distance_squared_to(viewPosition));
}
