#include "OpenGL/Drawable/DrawableTransparencyInfo.hpp"
#include "OpenGL/Drawable/LineInstanceData.hpp"
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

TEST(LineInstanceDataTest, ExpandsIndicesToSegmentPairsPerLineType) {
    const std::vector<std::uint32_t> indices = {0U, 1U, 2U, 3U};

    // lines: consecutive pairs are kept as-is.
    const std::vector<std::uint32_t> linePairs =
        opengl::expand_indices_to_segment_pairs(indices, opengl::LineType::lines());
    EXPECT_EQ((std::vector<std::uint32_t>{0U, 1U, 2U, 3U}), linePairs);

    // strip: (0,1),(1,2),(2,3).
    const std::vector<std::uint32_t> stripPairs =
        opengl::expand_indices_to_segment_pairs(indices, opengl::LineType::line_strip());
    EXPECT_EQ((std::vector<std::uint32_t>{0U, 1U, 1U, 2U, 2U, 3U}), stripPairs);

    // loop: strip plus a closing (3,0) segment.
    const std::vector<std::uint32_t> loopPairs =
        opengl::expand_indices_to_segment_pairs(indices, opengl::LineType::line_loop());
    EXPECT_EQ((std::vector<std::uint32_t>{0U, 1U, 1U, 2U, 2U, 3U, 3U, 0U}), loopPairs);

    // A dangling trailing index in a lines list is dropped.
    const std::vector<std::uint32_t> odd = {0U, 1U, 2U};
    EXPECT_EQ((std::vector<std::uint32_t>{0U, 1U}),
              opengl::expand_indices_to_segment_pairs(odd, opengl::LineType::lines()));
}

TEST(LineInstanceDataTest, ArcLengthAccumulatesAlongStripRuns) {
    // Three colinear vertices 2 units apart along X.
    const std::vector<linal::float3> positions = {
        linal::float3{0.0F, 0.0F, 0.0F},
        linal::float3{2.0F, 0.0F, 0.0F},
        linal::float3{4.0F, 0.0F, 0.0F},
    };

    // strip: cumulative 0, 2, 4.
    const std::vector<std::uint32_t> stripIndices = {0U, 1U, 2U};
    const std::vector<float> stripArc =
        opengl::make_vertex_arc_lengths(stripIndices, positions, opengl::LineType::line_strip());
    ASSERT_EQ(3U, stripArc.size());
    EXPECT_FLOAT_EQ(0.0F, stripArc[0]);
    EXPECT_FLOAT_EQ(2.0F, stripArc[1]);
    EXPECT_FLOAT_EQ(4.0F, stripArc[2]);
}

TEST(LineInstanceDataTest, IndependentArc0ForcesPerSegmentStart) {
    // Four distinct vertices forming two independent line segments.
    const std::vector<linal::float3> positions = {
        linal::float3{0.0F, 0.0F, 0.0F},
        linal::float3{2.0F, 0.0F, 0.0F},
        linal::float3{5.0F, 0.0F, 0.0F},
        linal::float3{9.0F, 0.0F, 0.0F},
    };
    // A non-trivial per-vertex arc buffer that would leak into arc0 if not overridden.
    const std::vector<float> arcLengths = {0.0F, 2.0F, 100.0F, 200.0F};
    const std::vector<std::uint32_t> pairs = {0U, 1U, 2U, 3U};

    // With independentArc0 (GL_LINES), every segment's stored arc0 (a_p1.w, index 7) must be 0.
    const std::vector<float> lineData =
        opengl::make_line_instance_data(pairs, positions, {}, 0, arcLengths, {}, /*independentArc0=*/true);
    ASSERT_EQ(2U * opengl::kLineInstanceFloats, lineData.size());
    EXPECT_FLOAT_EQ(0.0F, lineData[7]);                              // segment 0 arc0
    EXPECT_FLOAT_EQ(0.0F, lineData[opengl::kLineInstanceFloats + 7]); // segment 1 arc0

    // Without it (strips), arc0 comes from the per-vertex arc buffer.
    const std::vector<float> stripData =
        opengl::make_line_instance_data(pairs, positions, {}, 0, arcLengths, {}, /*independentArc0=*/false);
    EXPECT_FLOAT_EQ(0.0F, stripData[7]);                                // arcLengths[0]
    EXPECT_FLOAT_EQ(100.0F, stripData[opengl::kLineInstanceFloats + 7]); // arcLengths[2]
}

TEST(LineInstanceDataTest, BuildsInterleavedInstanceBlobWithDashAndColors) {
    const std::vector<linal::float3> positions = {
        linal::float3{0.0F, 0.0F, 0.0F},
        linal::float3{3.0F, 0.0F, 0.0F},
    };
    const std::vector<float> colors = {
        1.0F, 0.0F, 0.0F, 1.0F, // vertex 0 red
        0.0F, 1.0F, 0.0F, 0.5F, // vertex 1 translucent green
    };
    const std::vector<float> arcLengths = {0.0F, 3.0F};
    const std::vector<std::uint8_t> dashFlags = {1U, 0U}; // segment dashed because one endpoint is flagged

    const std::vector<std::uint32_t> pairs = {0U, 1U};
    const std::vector<float> blob =
        opengl::make_line_instance_data(pairs, positions, colors, 4, arcLengths, dashFlags);

    ASSERT_EQ(opengl::kLineInstanceFloats, blob.size());
    // a_p0 = (p0.xyz, dashedFlag)
    EXPECT_FLOAT_EQ(0.0F, blob[0]);
    EXPECT_FLOAT_EQ(0.0F, blob[1]);
    EXPECT_FLOAT_EQ(0.0F, blob[2]);
    EXPECT_FLOAT_EQ(1.0F, blob[3]); // dashed
    // a_p1 = (p1.xyz, arc0)
    EXPECT_FLOAT_EQ(3.0F, blob[4]);
    EXPECT_FLOAT_EQ(0.0F, blob[7]); // arc0
    // a_color0
    EXPECT_FLOAT_EQ(1.0F, blob[8]);
    EXPECT_FLOAT_EQ(1.0F, blob[11]);
    // a_color1
    EXPECT_FLOAT_EQ(0.0F, blob[12]);
    EXPECT_FLOAT_EQ(1.0F, blob[13]);
    EXPECT_FLOAT_EQ(0.5F, blob[15]);
}

TEST(LineInstanceDataTest, EmptyDashFlagsProduceSolidSegments) {
    const std::vector<linal::float3> positions = {
        linal::float3{0.0F, 0.0F, 0.0F},
        linal::float3{1.0F, 0.0F, 0.0F},
    };
    const std::vector<std::uint32_t> pairs = {0U, 1U};
    const std::vector<float> blob = opengl::make_line_instance_data(pairs, positions, {}, 0, {}, {});
    ASSERT_EQ(opengl::kLineInstanceFloats, blob.size());
    EXPECT_FLOAT_EQ(0.0F, blob[3]); // dashedFlag defaults to solid
    // With no colors provided, colors fall back to white.
    EXPECT_FLOAT_EQ(1.0F, blob[8]);
    EXPECT_FLOAT_EQ(1.0F, blob[15]);
}
