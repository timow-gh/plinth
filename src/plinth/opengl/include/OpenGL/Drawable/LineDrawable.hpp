#ifndef OPENGL_LINESOUP_HPP
#define OPENGL_LINESOUP_HPP

#include "OpenGL/BufferAccessPattern.hpp"
#include "OpenGL/Drawable/DrawableTransparencyInfo.hpp"
#include "OpenGL/InstanceBuffer.hpp"
#include "OpenGL/LineType.hpp"
#include "OpenGL/Programs/LineProgram.hpp"
#include "OpenGL/UpdateBuffer.hpp"
#include "OpenGL/VertexArray.hpp"
#include "OpenGL/VertexBuffer.hpp"
#include "OpenGL/opengl_export.h"
#include "plinth/DashSpace.hpp"
#include "plinth/Warnings.hpp"
#include <array>
#include <cstdint>
#include <linal/hmat.hpp>
#include <linal/vec.hpp>
#include <optional>
#include <span>
#include <vector>
RENDERER_DISABLE_ALL_WARNINGS
namespace opengl {

using renderer::DashSpace;

class OPENGL_EXPORT LineDrawable {
    LineProgram* m_program{nullptr};
    VertexArray m_vertexArray;
    VertexBuffer m_quadCornerBuffer;
    InstanceBuffer m_opaqueInstanceBuffer;
    InstanceBuffer m_translucentInstanceBuffer;
    float m_lineThickness{1.0F};
    LineType m_lineType{};
    float m_pointSize{1.0F};
    std::int32_t m_vertexDimension{0};
    std::int32_t m_colorDimension{0};
    bool m_dashEnabled{false};
    float m_dashSize{10.0F};
    float m_gapSize{10.0F};
    float m_dashPhase{0.0F};
    DashSpace m_dashSpace{DashSpace::World};
    std::vector<linal::float3> m_vertexPositions;
    std::vector<float> m_vertexColors;
    std::vector<std::uint8_t> m_vertexTranslucency;
    std::vector<float> m_vertexArcLengths;
    std::vector<std::uint8_t> m_vertexDashFlags;
    std::vector<std::uint32_t> m_lineIndices;
    std::vector<SortableLineSegment> m_translucentLineSegments;
    DrawableTransparencyInfo m_transparencyInfo;
    mutable std::vector<float> m_vertexPositionsFlatCache;

  public:
    LineDrawable(LineProgram& program,
                 VertexArray vertexArray,
                 VertexBuffer quadCornerBuffer,
                 InstanceBuffer opaqueInstanceBuffer,
                 InstanceBuffer translucentInstanceBuffer,
                 float lineThickness,
                 float pointSize = 0.0F,
                 LineType lineType = LineType::lines(),
                 DrawableTransparencyInfo transparencyInfo = {},
                 std::int32_t vertexDimension = 3,
                 std::int32_t colorDimension = 4,
                 std::vector<linal::float3> vertexPositions = {},
                 std::vector<float> vertexColors = {},
                 std::vector<std::uint8_t> vertexTranslucency = {},
                 std::vector<float> vertexArcLengths = {},
                 std::vector<std::uint8_t> vertexDashFlags = {},
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

    // Dashing controls. Empty dash flags combined with dashing enabled dash the whole line; a
    // per-vertex flag array (see set_dash_flags) enables per-segment control.
    void set_line_dash_enabled(bool enabled) { m_dashEnabled = enabled; }
    [[nodiscard]]
    bool is_line_dash_enabled() const {
        return m_dashEnabled;
    }
    void set_line_dash(float dashSize, float gapSize) {
        m_dashSize = dashSize;
        m_gapSize = gapSize;
    }
    void set_line_dash_phase(float phase) { m_dashPhase = phase; }
    [[nodiscard]]
    float get_line_dash_phase() const {
        return m_dashPhase;
    }
    void set_line_dash_space(DashSpace space) { m_dashSpace = space; }
    [[nodiscard]]
    DashSpace get_line_dash_space() const {
        return m_dashSpace;
    }
    // Replaces the per-vertex dashed flags and rebuilds the instance buffers.
    void set_dash_flags(std::span<const std::uint8_t> dashFlags, BufferAccessPattern accessPattern);

    void update_vertex_buffer(std::span<const float> vertices, BufferAccessPattern accessPattern);

    void update_color_buffer(std::span<const float> colors, BufferAccessPattern accessPattern);

    void update_indices_buffer(std::span<const std::uint32_t> indices, BufferAccessPattern accessPattern);

    void update_line_drawable(std::span<const float> vertices,
                              std::span<const float> colors,
                              std::span<const std::uint32_t> indices,
                              BufferAccessPattern accessPattern);

    void draw(const linal::hmatf& mvp, const linal::hmatf& modelMatrix, const linal::float2& viewportSize) const;

    void draw_opaque(const linal::hmatf& mvp, const linal::hmatf& modelMatrix, const linal::float2& viewportSize) const;

    void draw_translucent(const linal::hmatf& mvp,
                          const linal::hmatf& modelMatrix,
                          const linal::float2& viewportSize,
                          const linal::double3& viewPosition);

    /** Draw all line segments (opaque and translucent) into a color-ID pick pass using a flat
     * pickColor. The thick-quad footprint is used so the visible line is selectable. */
    void draw_pick(const linal::hmatf& mvp,
                   const linal::hmatf& modelMatrix,
                   const linal::float2& viewportSize,
                   const std::array<float, 3>& pickColor) const;

    [[nodiscard]]
    bool has_opaque_primitives() const noexcept {
        return m_opaqueInstanceBuffer.get_instance_count() > 0;
    }

    [[nodiscard]]
    bool has_translucent_primitives() const noexcept {
        return m_translucentInstanceBuffer.get_instance_count() > 0;
    }

    [[nodiscard]]
    bool is_translucent() const noexcept {
        return has_translucent_primitives();
    }

    [[nodiscard]]
    double distance_squared_to(const linal::double3& viewPosition) const noexcept {
        return m_transparencyInfo.distance_squared_to(viewPosition);
    }

    [[nodiscard]]
    double distance_squared_to(const linal::double3& viewPosition, const linal::hmatf& transform) const noexcept {
        return m_transparencyInfo.distance_squared_to(viewPosition, transform);
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
    void draw_instances(const linal::hmatf& mvp,
                        const linal::hmatf& modelMatrix,
                        const linal::float2& viewportSize,
                        const InstanceBuffer& instanceBuffer) const;

    void set_common_uniforms(const linal::hmatf& mvp,
                             const linal::hmatf& modelMatrix,
                             const linal::float2& viewportSize) const;

    void rebuild_instance_buffers(BufferAccessPattern accessPattern);

    void recompute_derived_data(std::span<const float> vertices, std::span<const float> colors);
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
                                                             opengl::BufferAccessPattern accessPattern,
                                                             bool dashEnabled = false,
                                                             float dashSize = 10.0F,
                                                             float gapSize = 10.0F,
                                                             DashSpace dashSpace = DashSpace::World,
                                                             std::span<const std::uint8_t> perVertexDashFlags = {});

} // namespace opengl

RENDERER_ENABLE_ALL_WARNINGS

#endif // OPENGL_LINESOUP_HPP
