#include "OpenGL/Drawable/LineDrawable.hpp"
#include <plinth/Assert.hpp>
#include <utility>

namespace opengl {

LineDrawable::LineDrawable(LineProgram& program,
                           VertexArray vertexArray,
                           VertexBuffer vertexBuffer,
                           VertexBuffer colorBuffer,
                           IndexBuffer opaqueLineIndicesBuffer,
                           IndexBuffer translucentLineIndicesBuffer,
                           float lineThickness,
                           float pointSize,
                           LineType lineType,
                           DrawableTransparencyInfo transparencyInfo,
                           std::int32_t vertexDimension,
                           std::int32_t colorDimension,
                           std::vector<linal::float3> vertexPositions,
                           std::vector<std::uint8_t> vertexTranslucency,
                           std::vector<std::uint32_t> lineIndices,
                           std::vector<SortableLineSegment> translucentLineSegments)
    : m_program(&program)
    , m_vertexArray(std::move(vertexArray))
    , m_vertexBuffer(std::move(vertexBuffer))
    , m_colorBuffer(std::move(colorBuffer))
    , m_opaqueLineIndicesBuffer(std::move(opaqueLineIndicesBuffer))
    , m_translucentLineIndicesBuffer(std::move(translucentLineIndicesBuffer))
    , m_lineThickness(lineThickness)
    , m_lineType(lineType)
    , m_pointSize(pointSize)
    , m_vertexDimension(vertexDimension)
    , m_colorDimension(colorDimension)
    , m_vertexPositions(std::move(vertexPositions))
    , m_vertexTranslucency(std::move(vertexTranslucency))
    , m_lineIndices(std::move(lineIndices))
    , m_translucentLineSegments(std::move(translucentLineSegments))
    , m_transparencyInfo(transparencyInfo) {
}

LineDrawable::LineDrawable(LineDrawable&& other) noexcept
    : m_program(std::exchange(other.m_program, nullptr))
    , m_vertexArray(std::move(other.m_vertexArray))
    , m_vertexBuffer(std::move(other.m_vertexBuffer))
    , m_colorBuffer(std::move(other.m_colorBuffer))
    , m_opaqueLineIndicesBuffer(std::move(other.m_opaqueLineIndicesBuffer))
    , m_translucentLineIndicesBuffer(std::move(other.m_translucentLineIndicesBuffer))
    , m_lineThickness(other.m_lineThickness)
    , m_lineType(other.m_lineType)
    , m_pointSize(other.m_pointSize)
    , m_vertexDimension(other.m_vertexDimension)
    , m_colorDimension(other.m_colorDimension)
    , m_vertexPositions(std::move(other.m_vertexPositions))
    , m_vertexTranslucency(std::move(other.m_vertexTranslucency))
    , m_lineIndices(std::move(other.m_lineIndices))
    , m_translucentLineSegments(std::move(other.m_translucentLineSegments))
    , m_transparencyInfo(other.m_transparencyInfo) {
}

LineDrawable& LineDrawable::operator=(LineDrawable&& other) noexcept {
    if (this != &other) {
        m_program = std::exchange(other.m_program, nullptr);
        m_vertexArray = std::move(other.m_vertexArray);
        m_vertexBuffer = std::move(other.m_vertexBuffer);
        m_colorBuffer = std::move(other.m_colorBuffer);
        m_opaqueLineIndicesBuffer = std::move(other.m_opaqueLineIndicesBuffer);
        m_translucentLineIndicesBuffer = std::move(other.m_translucentLineIndicesBuffer);
        m_lineThickness = other.m_lineThickness;
        m_lineType = other.m_lineType;
        m_pointSize = other.m_pointSize;
        m_vertexDimension = other.m_vertexDimension;
        m_colorDimension = other.m_colorDimension;
        m_vertexPositions = std::move(other.m_vertexPositions);
        m_vertexTranslucency = std::move(other.m_vertexTranslucency);
        m_lineIndices = std::move(other.m_lineIndices);
        m_translucentLineSegments = std::move(other.m_translucentLineSegments);
        m_transparencyInfo = other.m_transparencyInfo;
    }
    return *this;
}

void LineDrawable::update_vertex_buffer(std::span<const float> vertices, BufferAccessPattern accessPattern) {
    m_vertexBuffer.update_buffer(vertices, accessPattern);
    m_vertexPositions = make_vertex_sort_positions(vertices, m_vertexDimension);
    m_transparencyInfo.sortCenter = calc_sort_center(vertices, m_vertexDimension);
    rebuild_index_buffers(accessPattern);
}

void LineDrawable::update_color_buffer(std::span<const float> colors, BufferAccessPattern accessPattern) {
    m_colorBuffer.update_buffer(colors, accessPattern);
    m_vertexTranslucency = make_vertex_translucency_flags(colors, m_colorDimension);
    rebuild_index_buffers(accessPattern);
}

void LineDrawable::update_indices_buffer(std::span<const std::uint32_t> indices, BufferAccessPattern accessPattern) {
    m_lineIndices.assign(indices.begin(), indices.end());
    rebuild_index_buffers(accessPattern);
}

void LineDrawable::update_line_drawable(std::span<const float> vertices,
                                        std::span<const float> colors,
                                        std::span<const std::uint32_t> indices,
                                        BufferAccessPattern accessPattern) {
    m_vertexBuffer.update_buffer(vertices, accessPattern);
    m_colorBuffer.update_buffer(colors, accessPattern);
    m_vertexPositions = make_vertex_sort_positions(vertices, m_vertexDimension);
    m_vertexTranslucency = make_vertex_translucency_flags(colors, m_colorDimension);
    m_lineIndices.assign(indices.begin(), indices.end());
    m_transparencyInfo.sortCenter = calc_sort_center(vertices, m_vertexDimension);
    rebuild_index_buffers(accessPattern);
}

void LineDrawable::draw(const linal::hmatf& mvp, const linal::hmatf& modelMatrix) const {
    draw_opaque(mvp, modelMatrix);
    draw_index_buffer(mvp, modelMatrix, m_translucentLineIndicesBuffer);
}

void LineDrawable::draw_opaque(const linal::hmatf& mvp, const linal::hmatf& modelMatrix) const {
    draw_index_buffer(mvp, modelMatrix, m_opaqueLineIndicesBuffer);
}

void LineDrawable::draw_translucent(const linal::hmatf& mvp,
                                    const linal::hmatf& modelMatrix,
                                    const linal::double3& viewPosition) {
    const std::vector<std::uint32_t> sortedIndices =
        sort_translucent_line_indices_back_to_front(m_translucentLineSegments, viewPosition);
    m_translucentLineIndicesBuffer.update_indices_buffer(sortedIndices, BufferAccessPattern::Stream);
    draw_index_buffer(mvp, modelMatrix, m_translucentLineIndicesBuffer);
}

void LineDrawable::draw_index_buffer(const linal::hmatf& mvp,
                                     const linal::hmatf& modelMatrix,
                                     const IndexBuffer& indexBuffer) const {
    if (indexBuffer.get_index_count() == 0) {
        return;
    }

    RENDERER_ASSERT(m_program != nullptr);
    auto& prog = *m_program;
    prog.use();
    glUniformMatrix4fv(prog.get_view_projection_location().get_value(), 1, GL_FALSE, mvp.data());
    glUniformMatrix4fv(prog.get_model_matrix_location().get_value(), 1, GL_FALSE, modelMatrix.data());
    glLineWidth(m_lineThickness);
    m_vertexArray.bind();
    indexBuffer.bind();
    glDrawElements(to_gl_primitive(m_lineType), indexBuffer.get_index_count(), GL_UNSIGNED_INT, nullptr);

    if (m_pointSize != 0.0F) {
        glPointSize(m_pointSize);
        glDrawElements(GL_POINTS, indexBuffer.get_index_count(), GL_UNSIGNED_INT, nullptr);
    }
}

void LineDrawable::rebuild_index_buffers(BufferAccessPattern accessPattern) {
    LineTransparencyIndexSplit split =
        split_line_indices_by_transparency(m_lineIndices, m_vertexPositions, m_vertexTranslucency);
    m_translucentLineSegments = std::move(split.translucentSegments);
    const std::vector<std::uint32_t> flattenedTranslucentIndices =
        flatten_translucent_line_indices(m_translucentLineSegments);
    m_opaqueLineIndicesBuffer.update_indices_buffer(split.opaqueIndices, accessPattern);
    m_translucentLineIndicesBuffer.update_indices_buffer(flattenedTranslucentIndices, accessPattern);
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
                                               BufferAccessPattern accessPattern) {
    auto vertexArray = VertexArray::create();
    if (!vertexArray.has_value()) {
        RENDERER_ASSERT(false);
        return std::nullopt;
    }
    auto vertexBuffer =
        VertexBuffer::create(lineVertices, lineVertexDimension, program.get_vertex_location(), accessPattern);
    if (!vertexBuffer.has_value()) {
        return std::nullopt;
    }
    auto colorBuffer =
        VertexBuffer::create(lineColors, lineColorDimension, program.get_color_location(), accessPattern);
    if (!colorBuffer.has_value()) {
        return std::nullopt;
    }
    auto vertexPositions = make_vertex_sort_positions(lineVertices, lineVertexDimension);
    auto vertexTranslucency = make_vertex_translucency_flags(lineColors, lineColorDimension);
    auto lineIndexList = std::vector<std::uint32_t>(lineIndices.begin(), lineIndices.end());
    auto split = split_line_indices_by_transparency(lineIndexList, vertexPositions, vertexTranslucency);
    std::vector<std::uint32_t> translucentIndices = flatten_translucent_line_indices(split.translucentSegments);
    auto opaqueLineIndicesBuffer = IndexBuffer::create(split.opaqueIndices, accessPattern);
    if (!opaqueLineIndicesBuffer.has_value()) {
        return std::nullopt;
    }
    auto translucentLineIndicesBuffer = IndexBuffer::create(translucentIndices, accessPattern);
    if (!translucentLineIndicesBuffer.has_value()) {
        return std::nullopt;
    }
    return LineDrawable{
        program,
        std::move(vertexArray.value()),
        std::move(vertexBuffer.value()),
        std::move(colorBuffer.value()),
        std::move(opaqueLineIndicesBuffer.value()),
        std::move(translucentLineIndicesBuffer.value()),
        lineThickness,
        pointThickness,
        lineType,
        make_drawable_transparency_info(lineVertices, lineVertexDimension, lineColors, lineColorDimension),
        lineVertexDimension,
        lineColorDimension,
        std::move(vertexPositions),
        std::move(vertexTranslucency),
        std::move(lineIndexList),
        std::move(split.translucentSegments)};
}

} // namespace opengl
