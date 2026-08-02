#ifndef OPENGL_LINEINSTANCEDATA_HPP
#define OPENGL_LINEINSTANCEDATA_HPP

#include "OpenGL/Drawable/DrawableTransparencyInfo.hpp"
#include "plinth/LineType.hpp"
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <linal/vec.hpp>
#include <span>
#include <vector>

namespace opengl {

using renderer::LineType;

// Per-instance layout for the instanced line-quad shader. Each line segment is one instance built
// from four vec4 attributes (16 floats / 64 bytes), all with attribute divisor 1:
//   a_p0     = (p0.xyz, dashedFlag)   dashedFlag is 0.0 or 1.0 (no integer attribute path)
//   a_p1     = (p1.xyz, arc0)         arc1 is derived in the shader as arc0 + distance(p0, p1)
//   a_color0 = start rgba
//   a_color1 = end rgba
inline constexpr std::size_t kLineInstanceFloats = 16U;
inline constexpr std::size_t kLineInstanceStrideBytes = kLineInstanceFloats * sizeof(float);

// The shared unit quad expanded per instance. .x in {0,1} selects the endpoint (0 at p0, 1 at p1);
// .y in {-0.5, 0.5} selects the side offset scaled by line width. Two triangles, 6 vertices.
[[nodiscard]]
inline std::array<float, 12> unit_quad_corners() noexcept {
    return {
        0.0F, -0.5F, 1.0F, -0.5F, 1.0F, 0.5F, // triangle 1
        0.0F, -0.5F, 1.0F, 0.5F,  0.0F, 0.5F, // triangle 2
    };
}

// Expands an index buffer into flat segment index pairs (i0, i1, i0, i1, ...) following the same
// primitive assembly GL performs, so per-segment dashing and transparency work for every LineType.
//   lines      -> indices are already consecutive pairs.
//   line_strip -> a running polyline: (i0,i1), (i1,i2), ...
//   line_loop  -> like strip plus a closing segment (iLast, i0).
[[nodiscard]]
inline std::vector<std::uint32_t> expand_indices_to_segment_pairs(std::span<const std::uint32_t> indices,
                                                                  const LineType& lineType) {
    std::vector<std::uint32_t> pairs;
    if (lineType.is_line_strip() || lineType.is_line_loop()) {
        if (indices.size() < 2) {
            return pairs;
        }
        pairs.reserve((indices.size() + 1U) * 2U);
        for (std::size_t i = 0; i + 1 < indices.size(); ++i) {
            pairs.push_back(indices[i]);
            pairs.push_back(indices[i + 1]);
        }
        if (lineType.is_line_loop()) {
            pairs.push_back(indices.back());
            pairs.push_back(indices.front());
        }
        return pairs;
    }

    // lines: keep the existing consecutive-pair layout (drop a dangling trailing index).
    pairs.assign(indices.begin(), indices.end() - static_cast<std::ptrdiff_t>(indices.size() % 2U));
    return pairs;
}

[[nodiscard]]
inline float segment_length(const linal::float3& a, const linal::float3& b) noexcept {
    const float dx = b[0] - a[0];
    const float dy = b[1] - a[1];
    const float dz = b[2] - a[2];
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

// Per-vertex cumulative arc length along the line, walked in the order dashing marches.
//   lines      -> each pair is independent; arc restarts at 0 at the pair's first vertex.
//   line_strip -> running sum along the run.
//   line_loop  -> running sum along the run; the closing segment continues from the last vertex.
// Indexed by original vertex index (matching make_vertex_sort_positions / translucency flags).
[[nodiscard]]
inline std::vector<float> make_vertex_arc_lengths(std::span<const std::uint32_t> indices,
                                                  std::span<const linal::float3> positions,
                                                  const LineType& lineType) {
    std::vector<float> arcLengths(positions.size(), 0.0F);
    if (positions.empty()) {
        return arcLengths;
    }

    if (lineType.is_line_strip() || lineType.is_line_loop()) {
        if (indices.empty()) {
            return arcLengths;
        }
        float accumulated = 0.0F;
        const auto first = static_cast<std::size_t>(indices[0]);
        if (first < arcLengths.size()) {
            arcLengths[first] = 0.0F;
        }
        for (std::size_t i = 1; i < indices.size(); ++i) {
            const auto prev = static_cast<std::size_t>(indices[i - 1]);
            const auto cur = static_cast<std::size_t>(indices[i]);
            if (prev < positions.size() && cur < positions.size()) {
                accumulated += segment_length(positions[prev], positions[cur]);
            }
            if (cur < arcLengths.size()) {
                arcLengths[cur] = accumulated;
            }
        }
        return arcLengths;
    }

    // lines: each consecutive pair is an independent segment starting at arc 0.
    for (std::size_t i = 0; i + 1 < indices.size(); i += 2) {
        const auto a = static_cast<std::size_t>(indices[i]);
        const auto b = static_cast<std::size_t>(indices[i + 1]);
        if (a < arcLengths.size()) {
            arcLengths[a] = 0.0F;
        }
        if (a < positions.size() && b < positions.size() && b < arcLengths.size()) {
            arcLengths[b] = segment_length(positions[a], positions[b]);
        }
    }
    return arcLengths;
}

[[nodiscard]]
inline std::array<float, 4> get_color_or_white(std::span<const float> colors,
                                               std::int32_t colorDimension,
                                               std::uint32_t index) noexcept {
    std::array<float, 4> color{1.0F, 1.0F, 1.0F, 1.0F};
    if (colorDimension < 1) {
        return color;
    }
    const std::size_t base = static_cast<std::size_t>(index) * static_cast<std::size_t>(colorDimension);
    for (std::int32_t c = 0; c < colorDimension && c < 4; ++c) {
        const std::size_t idx = base + static_cast<std::size_t>(c);
        if (idx < colors.size()) {
            color[static_cast<std::size_t>(c)] = colors[idx];
        }
    }
    return color;
}

[[nodiscard]]
inline bool dash_flag_at(std::span<const std::uint8_t> dashFlags, std::uint32_t index) noexcept {
    const auto i = static_cast<std::size_t>(index);
    return i < dashFlags.size() && dashFlags[i] != 0U;
}

// Builds the interleaved per-instance blob from flat segment index pairs. dashFlags may be empty
// (all segments solid); a segment is dashed if either endpoint is flagged.
//
// arcLengths holds the per-vertex cumulative arc length. It is meaningful only for strips/loops,
// where each vertex belongs to a single run position. For GL_LINES each segment is independent and
// a vertex can be shared by several segments, so a per-vertex value is ambiguous; independentArc0
// forces every segment's arc0 to 0 (the shader derives arc1 from the segment length), which is the
// correct behavior for disjoint line soup.
[[nodiscard]]
inline std::vector<float> make_line_instance_data(std::span<const std::uint32_t> segmentPairs,
                                                  std::span<const linal::float3> positions,
                                                  std::span<const float> colors,
                                                  std::int32_t colorDimension,
                                                  std::span<const float> arcLengths,
                                                  std::span<const std::uint8_t> dashFlags,
                                                  bool independentArc0 = false) {
    std::vector<float> data;
    const std::size_t segmentCount = segmentPairs.size() / 2U;
    data.reserve(segmentCount * kLineInstanceFloats);

    for (std::size_t s = 0; s + 1 < segmentPairs.size(); s += 2) {
        const std::uint32_t i0 = segmentPairs[s];
        const std::uint32_t i1 = segmentPairs[s + 1];
        const linal::float3 p0 = get_sort_position_or_origin(positions, i0);
        const linal::float3 p1 = get_sort_position_or_origin(positions, i1);
        const float dashed = (dash_flag_at(dashFlags, i0) || dash_flag_at(dashFlags, i1)) ? 1.0F : 0.0F;
        const float arc0 =
            independentArc0 ? 0.0F : (static_cast<std::size_t>(i0) < arcLengths.size() ? arcLengths[i0] : 0.0F);
        const std::array<float, 4> color0 = get_color_or_white(colors, colorDimension, i0);
        const std::array<float, 4> color1 = get_color_or_white(colors, colorDimension, i1);

        // a_p0 = (p0.xyz, dashedFlag)
        data.push_back(p0[0]);
        data.push_back(p0[1]);
        data.push_back(p0[2]);
        data.push_back(dashed);
        // a_p1 = (p1.xyz, arc0)
        data.push_back(p1[0]);
        data.push_back(p1[1]);
        data.push_back(p1[2]);
        data.push_back(arc0);
        // a_color0
        data.push_back(color0[0]);
        data.push_back(color0[1]);
        data.push_back(color0[2]);
        data.push_back(color0[3]);
        // a_color1
        data.push_back(color1[0]);
        data.push_back(color1[1]);
        data.push_back(color1[2]);
        data.push_back(color1[3]);
    }

    return data;
}

// Flattens sortable segments back into flat index pairs (for translucent instance rebuilds).
[[nodiscard]]
inline std::vector<std::uint32_t> segments_to_index_pairs(std::span<const SortableLineSegment> segments) {
    std::vector<std::uint32_t> pairs;
    pairs.reserve(segments.size() * 2U);
    for (const SortableLineSegment& segment: segments) {
        pairs.push_back(segment.firstIndex);
        pairs.push_back(segment.secondIndex);
    }
    return pairs;
}

} // namespace opengl

#endif // OPENGL_LINEINSTANCEDATA_HPP
