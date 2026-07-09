#ifndef OPENGL_LINESOUP_HPP
#define OPENGL_LINESOUP_HPP

#include "OpenGL/BufferAccessPattern.hpp"
#include "OpenGL/Drawable/DrawableTransparencyInfo.hpp"
#include "OpenGL/IndexBuffer.hpp"
#include "OpenGL/LineType.hpp"
#include "OpenGL/Programs/LineProgram.hpp"
#include "OpenGL/UpdateBuffer.hpp"
#include "OpenGL/VertexArray.hpp"
#include "OpenGL/VertexBuffer.hpp"
#include "OpenGL/opengl_export.h"
#include <plinth/Warnings.hpp>
#include <cstdint>
#include <linal/hmat.hpp>
#include <span>
#include <vector>
RENDERER_DISABLE_ALL_WARNINGS
namespace opengl {

class OPENGL_EXPORT LineDrawable {
    LineProgram* m_program{nullptr};
    VertexArray m_vertexArray;
    VertexBuffer m_vertexBuffer;
    VertexBuffer m_colorBuffer;
    IndexBuffer m_opaqueLineIndicesBuffer;
    IndexBuffer m_translucentLineIndicesBuffer;
    float m_lineThickness{1.0F};
    LineType m_lineType{};
    float m_pointSize{1.0F};
    std::int32_t m_vertexDimension{0};
    std::int32_t m_colorDimension{0};
    std::vector<linal::float3> m_vertexPositions;
    std::vector<std::uint8_t> m_vertexTranslucency;
    std::vector<std::uint32_t> m_lineIndices;
    std::vector<SortableLineSegment> m_translucentLineSegments;
    DrawableTransparencyInfo m_transparencyInfo;
    mutable std::vector<float> m_vertexPositionsFlatCache;

  public:
    LineDrawable(LineProgram& program,
                 VertexArray vertexArray,
                 VertexBuffer vertexBuffer,
                 VertexBuffer colorBuffer,
                 IndexBuffer opaqueLineIndicesBuffer,
                 IndexBuffer translucentLineIndicesBuffer,
                 float lineThickness,
                 float pointSize = 1.0F,
                 LineType lineType = LineType::lines(),
                 DrawableTransparencyInfo transparencyInfo = {},
                 std::int32_t vertexDimension = 3,
                 std::int32_t colorDimension = 4,
                 std::vector<linal::float3> vertexPositions = {},
                 std::vector<std::uint8_t> vertexTranslucency = {},
                 std::vector<std::uint32_t> lineIndices = {},
                 std::vector<SortableLineSegment> translucentLineSegments = {});

    LineDrawable(const LineDrawable&) = delete;
    LineDrawable& operator=(const LineDrawable&) = delete;
    LineDrawable(LineDrawable&& other) noexcept;
    LineDrawable& operator=(LineDrawable&& other) noexcept;
    ~LineDrawable() = default;

    [[nodiscard]]
    float get_line_thickness() const {
        return m_lineThickness;
    }
    void set_line_thickness(float lineThickness) { m_lineThickness = lineThickness; }

    [[nodiscard]]
    float get_point_size() const {
        return m_pointSize;
    }
    void set_point_size(float pointSize) { m_pointSize = pointSize; }

    void update_vertex_buffer(std::span<const float> vertices, BufferAccessPattern accessPattern);

    void update_color_buffer(std::span<const float> colors, BufferAccessPattern accessPattern);

    void update_indices_buffer(std::span<const std::uint32_t> indices, BufferAccessPattern accessPattern);

    void update_line_drawable(std::span<const float> vertices,
                              std::span<const float> colors,
                              std::span<const std::uint32_t> indices,
                              BufferAccessPattern accessPattern);

    void draw(const linal::hmatf& mvp) const;

    void draw_opaque(const linal::hmatf& mvp) const;

    void draw_translucent(const linal::hmatf& mvp, const linal::double3& viewPosition);

    [[nodiscard]]
    bool has_opaque_primitives() const noexcept {
        return m_opaqueLineIndicesBuffer.get_index_count() > 0;
    }

    [[nodiscard]]
    bool has_translucent_primitives() const noexcept {
        return m_translucentLineIndicesBuffer.get_index_count() > 0;
    }

    [[nodiscard]]
    bool is_translucent() const noexcept {
        return has_translucent_primitives();
    }

    [[nodiscard]]
    double distance_squared_to(const linal::double3& viewPosition) const noexcept {
        return m_transparencyInfo.distance_squared_to(viewPosition);
    }

    // World-space xyz-triplet vertex positions, for scene-bounds computation (e.g. camera auto-fit).
    // Flattens element-by-element rather than reinterpreting m_vertexPositions' memory directly -
    // linal::float3 is not guaranteed to be a tightly-packed 3-float struct (its policy-mixin base
    // classes can add padding/alignment), so a raw reinterpret_cast is not safe here.
    [[nodiscard]]
    std::span<const float> get_vertex_positions() const noexcept {
        m_vertexPositionsFlatCache.clear();
        m_vertexPositionsFlatCache.reserve(m_vertexPositions.size() * 3U);
        for (const auto& p: m_vertexPositions) {
            m_vertexPositionsFlatCache.push_back(p[0]);
            m_vertexPositionsFlatCache.push_back(p[1]);
            m_vertexPositionsFlatCache.push_back(p[2]);
        }
        return m_vertexPositionsFlatCache;
    }

  private:
    void draw_index_buffer(const linal::hmatf& mvp, const IndexBuffer& indexBuffer) const;

    void rebuild_index_buffers(BufferAccessPattern accessPattern);
};

OPENGL_EXPORT std::optional<LineDrawable> make_line_drawable(LineProgram& program,
                                                             std::span<const float> lineVertices,
                                                             std::int32_t lineVertexDimension,
                                                             std::span<const std::uint32_t> lineIndices,
                                                             std::span<const float> lineColors,
                                                             std::int32_t lineColorDimension,
                                                             LineType lineType,
                                                             float lineThickness,
                                                             float pointThickness,
                                                             opengl::BufferAccessPattern accessPattern);

} // namespace opengl

RENDERER_ENABLE_ALL_WARNINGS

#endif // OPENGL_LINESOUP_HPP
