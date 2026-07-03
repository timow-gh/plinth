#ifndef OPENGL_DRAWABLETRANSPARENCYINFO_HPP
#define OPENGL_DRAWABLETRANSPARENCYINFO_HPP

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <linal/vec.hpp>
#include <span>
#include <vector>

namespace opengl {

struct DrawableTransparencyInfo {
    bool isTranslucent{false};
    linal::float3 sortCenter{0.0F, 0.0F, 0.0F};

    [[nodiscard]]
    double distance_squared_to(const linal::double3& viewPosition) const noexcept {
        const double dx = static_cast<double>(sortCenter[0]) - viewPosition[0];
        const double dy = static_cast<double>(sortCenter[1]) - viewPosition[1];
        const double dz = static_cast<double>(sortCenter[2]) - viewPosition[2];
        return dx * dx + dy * dy + dz * dz;
    }
};

struct SortablePointIndex {
    std::uint32_t index{};
    linal::float3 sortCenter{0.0F, 0.0F, 0.0F};
};

struct SortableLineSegment {
    std::uint32_t firstIndex{};
    std::uint32_t secondIndex{};
    linal::float3 sortCenter{0.0F, 0.0F, 0.0F};
};

struct PointTransparencyIndexSplit {
    std::vector<std::uint32_t> opaqueIndices;
    std::vector<SortablePointIndex> translucentIndices;
};

struct LineTransparencyIndexSplit {
    std::vector<std::uint32_t> opaqueIndices;
    std::vector<SortableLineSegment> translucentSegments;
};

[[nodiscard]]
inline bool contains_translucent_alpha(std::span<const float> colors, std::int32_t colorDimension) noexcept {
    if (colorDimension < 4) {
        return false;
    }

    for (std::size_t alphaIndex = 3; alphaIndex < colors.size();
         alphaIndex += static_cast<std::size_t>(colorDimension)) {
        if (colors[alphaIndex] < 1.0F) {
            return true;
        }
    }

    return false;
}

[[nodiscard]]
inline std::vector<std::uint8_t> make_vertex_translucency_flags(std::span<const float> colors,
                                                                std::int32_t colorDimension) {
    std::vector<std::uint8_t> flags;
    if (colorDimension < 4) {
        return flags;
    }

    const std::size_t colorStride = static_cast<std::size_t>(colorDimension);
    const std::size_t vertexCount = colors.size() / colorStride;
    flags.resize(vertexCount, 0);

    for (std::size_t vertexIndex = 0; vertexIndex < vertexCount; ++vertexIndex) {
        const std::size_t alphaIndex = vertexIndex * colorStride + 3;
        flags[vertexIndex] = colors[alphaIndex] < 1.0F ? 1U : 0U;
    }

    return flags;
}

[[nodiscard]]
inline linal::float3 calc_sort_center(std::span<const float> vertices, std::int32_t vertexDimension) noexcept {
    if (vertexDimension < 3 || vertices.empty()) {
        return linal::float3{0.0F, 0.0F, 0.0F};
    }

    linal::float3 center{0.0F, 0.0F, 0.0F};
    const std::size_t pointCount = vertices.size() / static_cast<std::size_t>(vertexDimension);
    if (pointCount == 0) {
        return center;
    }

    for (std::size_t i = 0; i < pointCount; ++i) {
        const std::size_t base = i * static_cast<std::size_t>(vertexDimension);
        center[0] += vertices[base];
        center[1] += vertices[base + 1];
        center[2] += vertices[base + 2];
    }

    const float invPointCount = 1.0F / static_cast<float>(pointCount);
    center[0] *= invPointCount;
    center[1] *= invPointCount;
    center[2] *= invPointCount;
    return center;
}

[[nodiscard]]
inline std::vector<linal::float3> make_vertex_sort_positions(std::span<const float> vertices,
                                                             std::int32_t vertexDimension) {
    std::vector<linal::float3> positions;
    if (vertexDimension < 3) {
        return positions;
    }

    const std::size_t vertexStride = static_cast<std::size_t>(vertexDimension);
    const std::size_t vertexCount = vertices.size() / vertexStride;
    positions.reserve(vertexCount);

    for (std::size_t vertexIndex = 0; vertexIndex < vertexCount; ++vertexIndex) {
        const std::size_t base = vertexIndex * vertexStride;
        positions.push_back(linal::float3{vertices[base], vertices[base + 1], vertices[base + 2]});
    }

    return positions;
}

[[nodiscard]]
inline linal::float3 get_sort_position_or_origin(std::span<const linal::float3> positions,
                                                 std::uint32_t index) noexcept {
    const auto positionIndex = static_cast<std::size_t>(index);
    if (positionIndex >= positions.size()) {
        return linal::float3{0.0F, 0.0F, 0.0F};
    }

    return *(positions.begin() + static_cast<std::ptrdiff_t>(positionIndex));
}

[[nodiscard]]
inline bool is_vertex_translucent(std::span<const std::uint8_t> vertexTranslucency, std::uint32_t index) noexcept {
    const auto vertexIndex = static_cast<std::size_t>(index);
    return vertexIndex < vertexTranslucency.size() && vertexTranslucency[vertexIndex] != 0U;
}

[[nodiscard]]
inline PointTransparencyIndexSplit
split_point_indices_by_transparency(std::span<const std::uint32_t> indices,
                                    std::span<const linal::float3> positions,
                                    std::span<const std::uint8_t> vertexTranslucency) {
    PointTransparencyIndexSplit split;
    split.opaqueIndices.reserve(indices.size());
    split.translucentIndices.reserve(indices.size());

    for (std::uint32_t index: indices) {
        if (is_vertex_translucent(vertexTranslucency, index)) {
            split.translucentIndices.push_back(
                SortablePointIndex{index, get_sort_position_or_origin(positions, index)});
        } else {
            split.opaqueIndices.push_back(index);
        }
    }

    return split;
}

[[nodiscard]]
inline PointTransparencyIndexSplit split_point_indices_by_transparency(std::span<const std::uint32_t> indices,
                                                                       std::span<const float> vertices,
                                                                       std::int32_t vertexDimension,
                                                                       std::span<const float> colors,
                                                                       std::int32_t colorDimension) {
    return split_point_indices_by_transparency(indices,
                                               make_vertex_sort_positions(vertices, vertexDimension),
                                               make_vertex_translucency_flags(colors, colorDimension));
}

[[nodiscard]]
inline linal::float3 calc_midpoint(const linal::float3& first, const linal::float3& second) noexcept {
    return linal::float3{(first[0] + second[0]) * 0.5F, (first[1] + second[1]) * 0.5F, (first[2] + second[2]) * 0.5F};
}

[[nodiscard]]
inline LineTransparencyIndexSplit split_line_indices_by_transparency(std::span<const std::uint32_t> indices,
                                                                     std::span<const linal::float3> positions,
                                                                     std::span<const std::uint8_t> vertexTranslucency) {
    LineTransparencyIndexSplit split;
    split.opaqueIndices.reserve(indices.size());
    split.translucentSegments.reserve(indices.size() / 2U);

    for (std::size_t i = 0; i + 1 < indices.size(); i += 2) {
        const std::uint32_t firstIndex = indices[i];
        const std::uint32_t secondIndex = indices[i + 1];
        if (is_vertex_translucent(vertexTranslucency, firstIndex) ||
            is_vertex_translucent(vertexTranslucency, secondIndex)) {
            split.translucentSegments.push_back(
                SortableLineSegment{firstIndex,
                                    secondIndex,
                                    calc_midpoint(get_sort_position_or_origin(positions, firstIndex),
                                                  get_sort_position_or_origin(positions, secondIndex))});
        } else {
            split.opaqueIndices.push_back(firstIndex);
            split.opaqueIndices.push_back(secondIndex);
        }
    }

    return split;
}

[[nodiscard]]
inline LineTransparencyIndexSplit split_line_indices_by_transparency(std::span<const std::uint32_t> indices,
                                                                     std::span<const float> vertices,
                                                                     std::int32_t vertexDimension,
                                                                     std::span<const float> colors,
                                                                     std::int32_t colorDimension) {
    return split_line_indices_by_transparency(indices,
                                              make_vertex_sort_positions(vertices, vertexDimension),
                                              make_vertex_translucency_flags(colors, colorDimension));
}

[[nodiscard]]
inline double distance_squared_to(const linal::float3& sortCenter, const linal::double3& viewPosition) noexcept {
    const double dx = static_cast<double>(sortCenter[0]) - viewPosition[0];
    const double dy = static_cast<double>(sortCenter[1]) - viewPosition[1];
    const double dz = static_cast<double>(sortCenter[2]) - viewPosition[2];
    return dx * dx + dy * dy + dz * dz;
}

[[nodiscard]]
inline std::vector<std::uint32_t> flatten_translucent_point_indices(std::span<const SortablePointIndex> indices) {
    std::vector<std::uint32_t> flattened;
    flattened.reserve(indices.size());
    for (const SortablePointIndex& index: indices) {
        flattened.push_back(index.index);
    }
    return flattened;
}

[[nodiscard]]
inline std::vector<std::uint32_t> flatten_translucent_line_indices(std::span<const SortableLineSegment> segments) {
    std::vector<std::uint32_t> flattened;
    flattened.reserve(segments.size() * 2U);
    for (const SortableLineSegment& segment: segments) {
        flattened.push_back(segment.firstIndex);
        flattened.push_back(segment.secondIndex);
    }
    return flattened;
}

[[nodiscard]]
inline std::vector<std::uint32_t>
sort_translucent_point_indices_back_to_front(std::span<const SortablePointIndex> points,
                                             const linal::double3& viewPosition) {
    std::vector<SortablePointIndex> sortedPoints(points.begin(), points.end());
    std::sort(sortedPoints.begin(),
              sortedPoints.end(),
              [&viewPosition](const SortablePointIndex& lhs, const SortablePointIndex& rhs) {
                  return distance_squared_to(lhs.sortCenter, viewPosition) >
                         distance_squared_to(rhs.sortCenter, viewPosition);
              });

    std::vector<std::uint32_t> sortedIndices;
    sortedIndices.reserve(sortedPoints.size());
    for (const SortablePointIndex& point: sortedPoints) {
        sortedIndices.push_back(point.index);
    }

    return sortedIndices;
}

[[nodiscard]]
inline std::vector<std::uint32_t>
sort_translucent_line_indices_back_to_front(std::span<const SortableLineSegment> segments,
                                            const linal::double3& viewPosition) {
    std::vector<SortableLineSegment> sortedSegments(segments.begin(), segments.end());
    std::sort(sortedSegments.begin(),
              sortedSegments.end(),
              [&viewPosition](const SortableLineSegment& lhs, const SortableLineSegment& rhs) {
                  return distance_squared_to(lhs.sortCenter, viewPosition) >
                         distance_squared_to(rhs.sortCenter, viewPosition);
              });

    std::vector<std::uint32_t> sortedIndices;
    sortedIndices.reserve(sortedSegments.size() * 2U);
    for (const SortableLineSegment& segment: sortedSegments) {
        sortedIndices.push_back(segment.firstIndex);
        sortedIndices.push_back(segment.secondIndex);
    }

    return sortedIndices;
}

[[nodiscard]]
inline DrawableTransparencyInfo make_drawable_transparency_info(std::span<const float> vertices,
                                                                std::int32_t vertexDimension,
                                                                std::span<const float> colors,
                                                                std::int32_t colorDimension) noexcept {
    return DrawableTransparencyInfo{contains_translucent_alpha(colors, colorDimension),
                                    calc_sort_center(vertices, vertexDimension)};
}

} // namespace opengl

#endif // OPENGL_DRAWABLETRANSPARENCYINFO_HPP
