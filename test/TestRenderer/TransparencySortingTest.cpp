#include <OpenGL/Drawable/DrawableTransparencyInfo.hpp>
#include <gtest/gtest.h>
#include <vector>

TEST(TransparencySortingTest, Points_AllOpaqueColorsProduceOnlyOpaqueIndices) {
    const std::vector<float> vertices = {0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 2.0F};
    const std::vector<float> colors = {1.0F, 0.0F, 0.0F, 1.0F, 0.0F, 1.0F, 0.0F, 1.0F};
    const std::vector<std::uint32_t> indices = {0U, 1U};

    const auto split = opengl::split_point_indices_by_transparency(indices, vertices, 3, colors, 4);

    EXPECT_EQ(split.opaqueIndices, indices);
    EXPECT_TRUE(split.translucentIndices.empty());
}

TEST(TransparencySortingTest, Points_MixedAlphaSplitsIndices) {
    const std::vector<float> vertices = {0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 2.0F, 0.0F, 0.0F, 3.0F};
    const std::vector<float> colors = {
        1.0F,
        0.0F,
        0.0F,
        1.0F,
        0.0F,
        1.0F,
        0.0F,
        0.5F,
        0.0F,
        0.0F,
        1.0F,
        1.0F,
    };
    const std::vector<std::uint32_t> indices = {0U, 1U, 2U};

    const auto split = opengl::split_point_indices_by_transparency(indices, vertices, 3, colors, 4);

    const std::vector<std::uint32_t> expectedOpaque = {0U, 2U};
    ASSERT_EQ(split.opaqueIndices, expectedOpaque);
    ASSERT_EQ(split.translucentIndices.size(), 1U);
    EXPECT_EQ(split.translucentIndices[0].index, 1U);
}

TEST(TransparencySortingTest, Points_TranslucentIndicesSortFarToNear) {
    const std::vector<opengl::SortablePointIndex> points = {
        {0U, linal::float3{0.0F, 0.0F, 1.0F}},
        {1U, linal::float3{0.0F, 0.0F, 5.0F}},
        {2U, linal::float3{0.0F, 0.0F, 3.0F}},
    };
    const linal::double3 viewPosition{0.0, 0.0, 0.0};

    const auto sorted = opengl::sort_translucent_point_indices_back_to_front(points, viewPosition);

    const std::vector<std::uint32_t> expected = {1U, 2U, 0U};
    EXPECT_EQ(sorted, expected);
}

TEST(TransparencySortingTest, Lines_SegmentIsTranslucentWhenEitherEndpointAlphaIsTranslucent) {
    const std::vector<float> vertices = {
        0.0F,
        0.0F,
        1.0F,
        0.0F,
        0.0F,
        2.0F,
        0.0F,
        0.0F,
        3.0F,
        0.0F,
        0.0F,
        4.0F,
    };
    const std::vector<float> colors = {
        1.0F,
        0.0F,
        0.0F,
        1.0F,
        1.0F,
        0.0F,
        0.0F,
        1.0F,
        0.0F,
        1.0F,
        0.0F,
        1.0F,
        0.0F,
        1.0F,
        0.0F,
        0.5F,
    };
    const std::vector<std::uint32_t> indices = {0U, 1U, 2U, 3U};

    const auto split = opengl::split_line_indices_by_transparency(indices, vertices, 3, colors, 4);

    const std::vector<std::uint32_t> expectedOpaque = {0U, 1U};
    ASSERT_EQ(split.opaqueIndices, expectedOpaque);
    ASSERT_EQ(split.translucentSegments.size(), 1U);
    EXPECT_EQ(split.translucentSegments[0].firstIndex, 2U);
    EXPECT_EQ(split.translucentSegments[0].secondIndex, 3U);
}

TEST(TransparencySortingTest, Lines_TranslucentSegmentsSortByMidpointFarToNear) {
    const std::vector<opengl::SortableLineSegment> segments = {
        {0U, 1U, linal::float3{0.0F, 0.0F, 2.0F}},
        {2U, 3U, linal::float3{0.0F, 0.0F, 10.0F}},
        {4U, 5U, linal::float3{0.0F, 0.0F, 5.0F}},
    };
    const linal::double3 viewPosition{0.0, 0.0, 0.0};

    const auto sorted = opengl::sort_translucent_line_indices_back_to_front(segments, viewPosition);

    const std::vector<std::uint32_t> expected = {2U, 3U, 4U, 5U, 0U, 1U};
    EXPECT_EQ(sorted, expected);
}
