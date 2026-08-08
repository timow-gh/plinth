#include "OpenGL/Drawable/LineDrawable.hpp"

#include "OpenGL/Drawable/LineInstanceData.hpp"
#include "plinth/Assert.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <utility>
#include <vector>

namespace opengl {

namespace {

// Attribute specs describing the interleaved per-instance layout (see LineInstanceData.hpp).
// Byte offsets match kLineInstanceFloats layout:
//   floats  0- 3  a_p0     offset  0
//   floats  4- 7  a_p1     offset 16
//   floats  8-11  a_color0 offset 32
//   floats 12-15  a_color1 offset 48
//   floats 16-19  a_pPrev  offset 64
//   floats 20-23  a_pNext  offset 80
std::array<InstanceAttribSpec, 6> make_instance_attribs(const LineProgram& program) {
    return {
        InstanceAttribSpec{program.get_p0_location(), 4, 0},
        InstanceAttribSpec{program.get_p1_location(), 4, 4 * static_cast<GLsizei>(sizeof(float))},
        InstanceAttribSpec{program.get_color0_location(), 4, 8 * static_cast<GLsizei>(sizeof(float))},
        InstanceAttribSpec{program.get_color1_location(), 4, 12 * static_cast<GLsizei>(sizeof(float))},
        InstanceAttribSpec{program.get_p_prev_location(), 4, 16 * static_cast<GLsizei>(sizeof(float))},
        InstanceAttribSpec{program.get_p_next_location(), 4, 20 * static_cast<GLsizei>(sizeof(float))},
    };
}

} // namespace

LineDrawable::LineDrawable(LineProgram& program,
                           VertexArray vertexArray,
                           VertexBuffer quadCornerBuffer,
                           InstanceBuffer opaqueInstanceBuffer,
                           InstanceBuffer translucentInstanceBuffer,
                           float lineThickness,
                           float pointSize,
                           LineType lineType,
                           DrawableTransparencyInfo transparencyInfo,
                           std::int32_t vertexDimension,
                           std::int32_t colorDimension,
                           std::vector<linal::float3> vertexPositions,
                           std::vector<float> vertexColors,
                           std::vector<std::uint8_t> vertexTranslucency,
                           std::vector<float> vertexArcLengths,
                           std::vector<std::uint8_t> vertexDashFlags,
                           std::vector<std::uint32_t> lineIndices,
                           std::vector<SortableLineSegment> translucentLineSegments)
    : m_program(&program)
    , m_vertexArray(std::move(vertexArray))
    , m_quadCornerBuffer(std::move(quadCornerBuffer))
    , m_opaqueInstanceBuffer(std::move(opaqueInstanceBuffer))
    , m_translucentInstanceBuffer(std::move(translucentInstanceBuffer))
    , m_lineThickness(lineThickness)
    , m_lineType(lineType)
    , m_pointSize(pointSize)
    , m_vertexDimension(vertexDimension)
    , m_colorDimension(colorDimension)
    , m_cap(LineCap::Butt)
    , m_join(LineJoin::Miter)
    , m_vertexPositions(std::move(vertexPositions))
    , m_vertexColors(std::move(vertexColors))
    , m_vertexTranslucency(std::move(vertexTranslucency))
    , m_vertexArcLengths(std::move(vertexArcLengths))
    , m_vertexDashFlags(std::move(vertexDashFlags))
    , m_lineIndices(std::move(lineIndices))
    , m_translucentLineSegments(std::move(translucentLineSegments))
    , m_transparencyInfo(transparencyInfo) {
}

LineDrawable::LineDrawable(LineDrawable&& other) noexcept
    : m_program(std::exchange(other.m_program, nullptr))
    , m_vertexArray(std::move(other.m_vertexArray))
    , m_quadCornerBuffer(std::move(other.m_quadCornerBuffer))
    , m_opaqueInstanceBuffer(std::move(other.m_opaqueInstanceBuffer))
    , m_translucentInstanceBuffer(std::move(other.m_translucentInstanceBuffer))
    , m_lineThickness(other.m_lineThickness)
    , m_lineType(other.m_lineType)
    , m_pointSize(other.m_pointSize)
    , m_vertexDimension(other.m_vertexDimension)
    , m_colorDimension(other.m_colorDimension)
    , m_cap(other.m_cap)
    , m_join(other.m_join)
    , m_dashPattern(std::move(other.m_dashPattern))
    , m_dashPhase(other.m_dashPhase)
    , m_dashSpace(other.m_dashSpace)
    , m_vertexPositions(std::move(other.m_vertexPositions))
    , m_vertexColors(std::move(other.m_vertexColors))
    , m_vertexTranslucency(std::move(other.m_vertexTranslucency))
    , m_vertexArcLengths(std::move(other.m_vertexArcLengths))
    , m_vertexDashFlags(std::move(other.m_vertexDashFlags))
    , m_lineIndices(std::move(other.m_lineIndices))
    , m_translucentLineSegments(std::move(other.m_translucentLineSegments))
    , m_transparencyInfo(other.m_transparencyInfo) {
}

LineDrawable& LineDrawable::operator=(LineDrawable&& other) noexcept {
    if (this != &other) {
        m_program = std::exchange(other.m_program, nullptr);
        m_vertexArray = std::move(other.m_vertexArray);
        m_quadCornerBuffer = std::move(other.m_quadCornerBuffer);
        m_opaqueInstanceBuffer = std::move(other.m_opaqueInstanceBuffer);
        m_translucentInstanceBuffer = std::move(other.m_translucentInstanceBuffer);
        m_lineThickness = other.m_lineThickness;
        m_lineType = other.m_lineType;
        m_pointSize = other.m_pointSize;
        m_vertexDimension = other.m_vertexDimension;
        m_colorDimension = other.m_colorDimension;
        m_cap = other.m_cap;
        m_join = other.m_join;
        m_dashPattern = std::move(other.m_dashPattern);
        m_dashPhase = other.m_dashPhase;
        m_dashSpace = other.m_dashSpace;
        m_vertexPositions = std::move(other.m_vertexPositions);
        m_vertexColors = std::move(other.m_vertexColors);
        m_vertexTranslucency = std::move(other.m_vertexTranslucency);
        m_vertexArcLengths = std::move(other.m_vertexArcLengths);
        m_vertexDashFlags = std::move(other.m_vertexDashFlags);
        m_lineIndices = std::move(other.m_lineIndices);
        m_translucentLineSegments = std::move(other.m_translucentLineSegments);
        m_transparencyInfo = other.m_transparencyInfo;
    }
    return *this;
}

void LineDrawable::recompute_derived_data(std::span<const float> vertices, std::span<const float> colors) {
    m_vertexPositions = make_vertex_sort_positions(vertices, m_vertexDimension);
    m_vertexColors.assign(colors.begin(), colors.end());
    m_vertexTranslucency = make_vertex_translucency_flags(colors, m_colorDimension);
    m_vertexArcLengths = make_vertex_arc_lengths(m_lineIndices, m_vertexPositions, m_lineType);
    m_transparencyInfo.sortCenter = calc_sort_center(vertices, m_vertexDimension);
}

void LineDrawable::update_vertex_buffer(std::span<const float> vertices, BufferAccessPattern accessPattern) {
    m_vertexPositions = make_vertex_sort_positions(vertices, m_vertexDimension);
    m_vertexArcLengths = make_vertex_arc_lengths(m_lineIndices, m_vertexPositions, m_lineType);
    m_transparencyInfo.sortCenter = calc_sort_center(vertices, m_vertexDimension);
    rebuild_instance_buffers(accessPattern);
}

void LineDrawable::update_color_buffer(std::span<const float> colors, BufferAccessPattern accessPattern) {
    m_vertexColors.assign(colors.begin(), colors.end());
    m_vertexTranslucency = make_vertex_translucency_flags(colors, m_colorDimension);
    rebuild_instance_buffers(accessPattern);
}

void LineDrawable::update_indices_buffer(std::span<const std::uint32_t> indices, BufferAccessPattern accessPattern) {
    m_lineIndices.assign(indices.begin(), indices.end());
    m_vertexArcLengths = make_vertex_arc_lengths(m_lineIndices, m_vertexPositions, m_lineType);
    rebuild_instance_buffers(accessPattern);
}

void LineDrawable::update_line_drawable(std::span<const float> vertices,
                                        std::span<const float> colors,
                                        std::span<const std::uint32_t> indices,
                                        BufferAccessPattern accessPattern) {
    m_lineIndices.assign(indices.begin(), indices.end());
    recompute_derived_data(vertices, colors);
    rebuild_instance_buffers(accessPattern);
}

void LineDrawable::set_dash_flags(std::span<const std::uint8_t> dashFlags, BufferAccessPattern accessPattern) {
    m_vertexDashFlags.assign(dashFlags.begin(), dashFlags.end());
    rebuild_instance_buffers(accessPattern);
}

void LineDrawable::set_common_uniforms(const linal::hmatf& mvp,
                                       const linal::hmatf& modelMatrix,
                                       const linal::float2& viewportSize) const {
    auto& prog = *m_program;
    glUniformMatrix4fv(prog.get_view_projection_location().get_value(), 1, GL_TRUE, mvp.data());
    glUniformMatrix4fv(prog.get_model_matrix_location().get_value(), 1, GL_TRUE, modelMatrix.data());
    glUniform2f(prog.get_viewport_size_location().get_value(), viewportSize[0], viewportSize[1]);
    glUniform1f(prog.get_line_width_location().get_value(), m_lineThickness);
    glUniform1i(prog.get_dash_space_location().get_value(), static_cast<GLint>(m_dashSpace));
    glUniform1f(prog.get_dash_phase_location().get_value(), m_dashPhase);
    glUniform1i(prog.get_cap_style_location().get_value(), static_cast<GLint>(m_cap));
    glUniform1i(prog.get_join_style_location().get_value(), static_cast<GLint>(m_join));

    // Dash pattern: clamp to DASH_PATTERN_MAX (16) entries, upload count + array.
    constexpr GLint kMaxDashPattern = 16;
    const GLint patternCount = static_cast<GLint>(m_dashPattern.size() < static_cast<std::size_t>(kMaxDashPattern)
                                                      ? m_dashPattern.size()
                                                      : static_cast<std::size_t>(kMaxDashPattern));
    glUniform1i(prog.get_dash_pattern_count_location().get_value(), patternCount);
    if (patternCount > 0) {
        glUniform1fv(prog.get_dash_pattern_location().get_value(), patternCount, m_dashPattern.data());
    }
}

void LineDrawable::draw_instances(const linal::hmatf& mvp,
                                  const linal::hmatf& modelMatrix,
                                  const linal::float2& viewportSize,
                                  const InstanceBuffer& instanceBuffer) const {
    if (instanceBuffer.get_instance_count() == 0) {
        return;
    }
    RENDERER_ASSERT(m_program != nullptr);
    auto& prog = *m_program;
    prog.use();
    set_common_uniforms(mvp, modelMatrix, viewportSize);
    m_vertexArray.bind();
    m_quadCornerBuffer.bind();
    instanceBuffer.bind();
    glDrawArraysInstanced(GL_TRIANGLES, 0, 6, instanceBuffer.get_instance_count());
}

void LineDrawable::draw(const linal::hmatf& mvp,
                        const linal::hmatf& modelMatrix,
                        const linal::float2& viewportSize) const {
    draw_opaque(mvp, modelMatrix, viewportSize);
    draw_instances(mvp, modelMatrix, viewportSize, m_translucentInstanceBuffer);
}

void LineDrawable::draw_opaque(const linal::hmatf& mvp,
                               const linal::hmatf& modelMatrix,
                               const linal::float2& viewportSize) const {
    draw_instances(mvp, modelMatrix, viewportSize, m_opaqueInstanceBuffer);
}

void LineDrawable::draw_translucent(const linal::hmatf& mvp,
                                    const linal::hmatf& modelMatrix,
                                    const linal::float2& viewportSize,
                                    const linal::double3& viewPosition) {
    // Reorder translucent segments back-to-front (reusing the shared sort), then rebuild the
    // instance blob in that order so translucent segments blend correctly.
    const std::vector<std::uint32_t> sortedPairs =
        sort_translucent_line_indices_back_to_front(m_translucentLineSegments, viewPosition);
    const SegmentNeighbours neighbours = make_segment_neighbours(sortedPairs, m_vertexPositions, m_lineType);
    const std::vector<float> instanceData = make_line_instance_data(sortedPairs,
                                                                    m_vertexPositions,
                                                                    m_vertexColors,
                                                                    m_colorDimension,
                                                                    m_vertexArcLengths,
                                                                    m_vertexDashFlags,
                                                                    m_lineType.is_lines(),
                                                                    &neighbours);
    m_translucentInstanceBuffer.update(instanceData, BufferAccessPattern::Stream);
    draw_instances(mvp, modelMatrix, viewportSize, m_translucentInstanceBuffer);
}

void LineDrawable::draw_pick(const linal::hmatf& mvp,
                             const linal::hmatf& modelMatrix,
                             const linal::float2& viewportSize,
                             const std::array<float, 3>& pickColor) const {
    RENDERER_ASSERT(m_program != nullptr);
    auto& prog = *m_program;
    const auto draw_buffer = [&](const InstanceBuffer& instanceBuffer) {
        if (instanceBuffer.get_instance_count() == 0) {
            return;
        }
        prog.use();
        set_common_uniforms(mvp, modelMatrix, viewportSize);
        glUniform1i(prog.get_pick_mode_location().get_value(), GL_TRUE);
        glUniform3fv(prog.get_pick_color_location().get_value(), 1, pickColor.data());
        m_vertexArray.bind();
        m_quadCornerBuffer.bind();
        instanceBuffer.bind();
        glDrawArraysInstanced(GL_TRIANGLES, 0, 6, instanceBuffer.get_instance_count());
    };

    // Pick both opaque and translucent segments: a translucent line is still selectable.
    draw_buffer(m_opaqueInstanceBuffer);
    draw_buffer(m_translucentInstanceBuffer);

    glUniform1i(prog.get_pick_mode_location().get_value(), GL_FALSE);
}

void LineDrawable::rebuild_instance_buffers(BufferAccessPattern accessPattern) {
    // Expand primitive runs into per-segment pairs, then split into opaque/translucent instance
    // ranges. Colors, arc lengths, and dash flags come from the cached per-vertex data.
    const std::vector<std::uint32_t> segmentPairs = expand_indices_to_segment_pairs(m_lineIndices, m_lineType);
    LineTransparencyIndexSplit split =
        split_line_indices_by_transparency(segmentPairs, m_vertexPositions, m_vertexTranslucency);
    m_translucentLineSegments = std::move(split.translucentSegments);

    const bool independentArc0 = m_lineType.is_lines();
    const SegmentNeighbours neighbours = make_segment_neighbours(segmentPairs, m_vertexPositions, m_lineType);

    const std::vector<float> opaqueData = make_line_instance_data(split.opaqueIndices,
                                                                  m_vertexPositions,
                                                                  m_vertexColors,
                                                                  m_colorDimension,
                                                                  m_vertexArcLengths,
                                                                  m_vertexDashFlags,
                                                                  independentArc0,
                                                                  &neighbours);
    const std::vector<std::uint32_t> translucentPairs = segments_to_index_pairs(m_translucentLineSegments);
    const std::vector<float> translucentData = make_line_instance_data(translucentPairs,
                                                                       m_vertexPositions,
                                                                       m_vertexColors,
                                                                       m_colorDimension,
                                                                       m_vertexArcLengths,
                                                                       m_vertexDashFlags,
                                                                       independentArc0,
                                                                       &neighbours);
    m_opaqueInstanceBuffer.update(opaqueData, accessPattern);
    m_translucentInstanceBuffer.update(translucentData, accessPattern);
    m_transparencyInfo.isTranslucent = !m_translucentLineSegments.empty();
}

std::optional<LineDrawable> make_line_drawable(LineProgram& program,
                                               std::span<const float> lineVertices,
                                               std::int32_t lineVertexDimension,
                                               std::span<const std::uint32_t> lineIndices,
                                               std::span<const float> lineColors,
                                               std::int32_t lineColorDimension,
                                               LineType lineType,
                                               float lineThickness,
                                               float pointThickness,
                                               BufferAccessPattern accessPattern,
                                               LineCap cap,
                                               LineJoin join,
                                               std::span<const float> dashPattern,
                                               DashSpace dashSpace,
                                               std::span<const std::uint8_t> perVertexDashFlags) {
    auto vertexArray = VertexArray::create();
    if (!vertexArray.has_value()) {
        RENDERER_ASSERT(false);
        return std::nullopt;
    }

    // Shared unit quad (divisor 0). Explicitly pin its divisor to 0 so instancing state from a
    // reused attribute location can never bleed into this per-vertex attribute.
    const std::array<float, 12> corners = unit_quad_corners();
    auto quadCornerBuffer = VertexBuffer::create(corners, 2, program.get_corner_location(), accessPattern);
    if (!quadCornerBuffer.has_value()) {
        return std::nullopt;
    }
    glVertexAttribDivisor(program.get_corner_location().get_as_unsigned(), 0);

    // Auto-derive per-vertex data (positions, translucency, arc length). Arc length is computed from
    // the original index run per LineType so dashing is continuous along strips/loops; segment pairs
    // are only expanded for instance construction.
    const std::vector<std::uint32_t> originalIndices(lineIndices.begin(), lineIndices.end());
    auto vertexPositions = make_vertex_sort_positions(lineVertices, lineVertexDimension);
    auto vertexTranslucency = make_vertex_translucency_flags(lineColors, lineColorDimension);
    auto vertexArcLengths = make_vertex_arc_lengths(originalIndices, vertexPositions, lineType);
    std::vector<std::uint8_t> dashFlags(perVertexDashFlags.begin(), perVertexDashFlags.end());
    if (!dashPattern.empty() && dashFlags.empty()) {
        const std::size_t vertexCount = lineVertices.size() / static_cast<std::size_t>(lineVertexDimension);
        dashFlags.assign(vertexCount, 1U);
    }
    std::vector<float> vertexColors(lineColors.begin(), lineColors.end());

    const std::vector<std::uint32_t> segmentPairs = expand_indices_to_segment_pairs(lineIndices, lineType);
    auto split = split_line_indices_by_transparency(segmentPairs, vertexPositions, vertexTranslucency);

    const bool independentArc0 = lineType.is_lines();
    const SegmentNeighbours neighbours = make_segment_neighbours(segmentPairs, vertexPositions, lineType);

    const std::vector<float> opaqueData = make_line_instance_data(split.opaqueIndices,
                                                                  vertexPositions,
                                                                  lineColors,
                                                                  lineColorDimension,
                                                                  vertexArcLengths,
                                                                  dashFlags,
                                                                  independentArc0,
                                                                  &neighbours);
    const std::vector<std::uint32_t> translucentPairs = segments_to_index_pairs(split.translucentSegments);
    const std::vector<float> translucentData = make_line_instance_data(translucentPairs,
                                                                       vertexPositions,
                                                                       lineColors,
                                                                       lineColorDimension,
                                                                       vertexArcLengths,
                                                                       dashFlags,
                                                                       independentArc0,
                                                                       &neighbours);

    const std::array<InstanceAttribSpec, 6> attribs = make_instance_attribs(program);
    auto opaqueInstanceBuffer =
        InstanceBuffer::create(opaqueData, static_cast<GLsizei>(kLineInstanceStrideBytes), attribs, accessPattern);
    if (!opaqueInstanceBuffer.has_value()) {
        return std::nullopt;
    }
    auto translucentInstanceBuffer = InstanceBuffer::create(translucentData,
                                                            static_cast<GLsizei>(kLineInstanceStrideBytes),
                                                            attribs,
                                                            BufferAccessPattern::Stream);
    if (!translucentInstanceBuffer.has_value()) {
        return std::nullopt;
    }

    LineDrawable drawable{
        program,
        std::move(vertexArray.value()),
        std::move(quadCornerBuffer.value()),
        std::move(opaqueInstanceBuffer.value()),
        std::move(translucentInstanceBuffer.value()),
        lineThickness,
        pointThickness,
        lineType,
        make_drawable_transparency_info(lineVertices, lineVertexDimension, lineColors, lineColorDimension),
        lineVertexDimension,
        lineColorDimension,
        std::move(vertexPositions),
        std::move(vertexColors),
        std::move(vertexTranslucency),
        std::move(vertexArcLengths),
        std::move(dashFlags),
        originalIndices,
        std::move(split.translucentSegments)};
    drawable.set_line_cap(cap);
    drawable.set_line_join(join);
    drawable.set_line_dash_pattern(dashPattern);
    drawable.set_line_dash_space(dashSpace);
    return drawable;
}

} // namespace opengl
