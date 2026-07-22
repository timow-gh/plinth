#include "OpenGL/Drawable/PointDrawable.hpp"
#include <plinth/Assert.hpp>
#include <utility>

namespace opengl {

namespace {

template <typename Drawable>
std::optional<Drawable> make_failed_drawable() {
    RENDERER_ASSERT(false);
    return std::nullopt;
}

} // namespace

PointDrawable::PointDrawable(PointProgram& program,
                             VertexArray vertexArray,
                             VertexBuffer vertexBuffer,
                             VertexBuffer colorBuffer,
                             IndexBuffer opaquePointIndicesBuffer,
                             IndexBuffer translucentPointIndicesBuffer,
                             float pointSize,
                             DrawableTransparencyInfo transparencyInfo,
                             std::int32_t vertexDimension,
                             std::int32_t colorDimension,
                             std::vector<linal::float3> vertexPositions,
                             std::vector<std::uint8_t> vertexTranslucency,
                             std::vector<std::uint32_t> pointIndices,
                             std::vector<SortablePointIndex> translucentPointIndices) noexcept
    : m_program(&program)
    , m_vertexArray(std::move(vertexArray))
    , m_vertexBuffer(std::move(vertexBuffer))
    , m_colorBuffer(std::move(colorBuffer))
    , m_opaquePointIndicesBuffer(std::move(opaquePointIndicesBuffer))
    , m_translucentPointIndicesBuffer(std::move(translucentPointIndicesBuffer))
    , m_pointSize(pointSize)
    , m_vertexDimension(vertexDimension)
    , m_colorDimension(colorDimension)
    , m_vertexPositions(std::move(vertexPositions))
    , m_vertexTranslucency(std::move(vertexTranslucency))
    , m_pointIndices(std::move(pointIndices))
    , m_translucentPointIndices(std::move(translucentPointIndices))
    , m_transparencyInfo(transparencyInfo) {
}

PointDrawable::PointDrawable(PointDrawable&& other) noexcept
    : m_program(std::exchange(other.m_program, nullptr))
    , m_vertexArray(std::move(other.m_vertexArray))
    , m_vertexBuffer(std::move(other.m_vertexBuffer))
    , m_colorBuffer(std::move(other.m_colorBuffer))
    , m_opaquePointIndicesBuffer(std::move(other.m_opaquePointIndicesBuffer))
    , m_translucentPointIndicesBuffer(std::move(other.m_translucentPointIndicesBuffer))
    , m_pointSize(other.m_pointSize)
    , m_vertexDimension(other.m_vertexDimension)
    , m_colorDimension(other.m_colorDimension)
    , m_vertexPositions(std::move(other.m_vertexPositions))
    , m_vertexTranslucency(std::move(other.m_vertexTranslucency))
    , m_pointIndices(std::move(other.m_pointIndices))
    , m_translucentPointIndices(std::move(other.m_translucentPointIndices))
    , m_transparencyInfo(other.m_transparencyInfo) {
}

PointDrawable& PointDrawable::operator=(PointDrawable&& other) noexcept {
    if (this != &other) {
        m_program = std::exchange(other.m_program, nullptr);
        m_vertexArray = std::move(other.m_vertexArray);
        m_vertexBuffer = std::move(other.m_vertexBuffer);
        m_colorBuffer = std::move(other.m_colorBuffer);
        m_opaquePointIndicesBuffer = std::move(other.m_opaquePointIndicesBuffer);
        m_translucentPointIndicesBuffer = std::move(other.m_translucentPointIndicesBuffer);
        m_pointSize = other.m_pointSize;
        m_vertexDimension = other.m_vertexDimension;
        m_colorDimension = other.m_colorDimension;
        m_vertexPositions = std::move(other.m_vertexPositions);
        m_vertexTranslucency = std::move(other.m_vertexTranslucency);
        m_pointIndices = std::move(other.m_pointIndices);
        m_translucentPointIndices = std::move(other.m_translucentPointIndices);
        m_transparencyInfo = other.m_transparencyInfo;
    }
    return *this;
}

void PointDrawable::update_vertex_buffer(std::span<const float> vertices, BufferAccessPattern accessPattern) {
    m_vertexBuffer.update_buffer(vertices, accessPattern);
    m_vertexPositions = make_vertex_sort_positions(vertices, m_vertexDimension);
    m_transparencyInfo.sortCenter = calc_sort_center(vertices, m_vertexDimension);
    rebuild_index_buffers(accessPattern);
}

void PointDrawable::update_color_buffer(std::span<const float> colors, BufferAccessPattern accessPattern) {
    m_colorBuffer.update_buffer(colors, accessPattern);
    m_vertexTranslucency = make_vertex_translucency_flags(colors, m_colorDimension);
    rebuild_index_buffers(accessPattern);
}

void PointDrawable::update_indices_buffer(std::span<const std::uint32_t> indices, BufferAccessPattern accessPattern) {
    m_pointIndices.assign(indices.begin(), indices.end());
    rebuild_index_buffers(accessPattern);
}

void PointDrawable::update_point_drawable(std::span<const float> vertices,
                                          std::span<const float> colors,
                                          std::span<const std::uint32_t> indices,
                                          BufferAccessPattern accessPattern) {
    m_vertexBuffer.update_buffer(vertices, accessPattern);
    m_colorBuffer.update_buffer(colors, accessPattern);
    m_vertexPositions = make_vertex_sort_positions(vertices, m_vertexDimension);
    m_vertexTranslucency = make_vertex_translucency_flags(colors, m_colorDimension);
    m_pointIndices.assign(indices.begin(), indices.end());
    m_transparencyInfo.sortCenter = calc_sort_center(vertices, m_vertexDimension);
    rebuild_index_buffers(accessPattern);
}

void PointDrawable::draw(const linal::hmatf& mvp, const linal::hmatf& modelMatrix) const {
    draw_opaque(mvp, modelMatrix);
    draw_index_buffer(mvp, modelMatrix, m_translucentPointIndicesBuffer);
}

void PointDrawable::draw_opaque(const linal::hmatf& mvp, const linal::hmatf& modelMatrix) const {
    draw_index_buffer(mvp, modelMatrix, m_opaquePointIndicesBuffer);
}

void PointDrawable::draw_translucent(const linal::hmatf& mvp,
                                     const linal::hmatf& modelMatrix,
                                     const linal::double3& viewPosition) {
    const std::vector<std::uint32_t> sortedIndices =
        sort_translucent_point_indices_back_to_front(m_translucentPointIndices, viewPosition);
    m_translucentPointIndicesBuffer.update_indices_buffer(sortedIndices, BufferAccessPattern::Stream);
    draw_index_buffer(mvp, modelMatrix, m_translucentPointIndicesBuffer);
}

void PointDrawable::draw_index_buffer(const linal::hmatf& mvp,
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
    glPointSize(m_pointSize);
    m_vertexArray.bind();
    indexBuffer.bind();

    glDrawElements(GL_POINTS, indexBuffer.get_index_count(), GL_UNSIGNED_INT, nullptr);
}

void PointDrawable::rebuild_index_buffers(BufferAccessPattern accessPattern) {
    PointTransparencyIndexSplit split =
        split_point_indices_by_transparency(m_pointIndices, m_vertexPositions, m_vertexTranslucency);
    m_translucentPointIndices = std::move(split.translucentIndices);
    const std::vector<std::uint32_t> flattenedTranslucentIndices =
        flatten_translucent_point_indices(m_translucentPointIndices);
    m_opaquePointIndicesBuffer.update_indices_buffer(split.opaqueIndices, accessPattern);
    m_translucentPointIndicesBuffer.update_indices_buffer(flattenedTranslucentIndices, accessPattern);
    m_transparencyInfo.isTranslucent = !m_translucentPointIndices.empty();
}

std::optional<PointDrawable> make_point_drawable(PointProgram& program,
                                                 std::span<const float> vertices,
                                                 std::int32_t vertexDimension,
                                                 std::span<const float> colors,
                                                 std::int32_t colorDimension,
                                                 std::span<const std::uint32_t> indices,
                                                 float pointSize,
                                                 BufferAccessPattern accessPattern) {
    auto vertexArray = opengl::VertexArray::create();
    if (!vertexArray.has_value()) {
        return make_failed_drawable<PointDrawable>();
    }
    auto vertexBuffer =
        opengl::VertexBuffer::create(vertices, vertexDimension, program.get_pos_location(), accessPattern);
    if (!vertexBuffer.has_value()) {
        return make_failed_drawable<PointDrawable>();
    }
    auto colorBuffer =
        opengl::VertexBuffer::create(colors, colorDimension, program.get_color_location(), accessPattern);
    if (!colorBuffer.has_value()) {
        return make_failed_drawable<PointDrawable>();
    }
    auto vertexPositions = make_vertex_sort_positions(vertices, vertexDimension);
    auto vertexTranslucency = make_vertex_translucency_flags(colors, colorDimension);
    auto pointIndices = std::vector<std::uint32_t>(indices.begin(), indices.end());
    auto split = split_point_indices_by_transparency(pointIndices, vertexPositions, vertexTranslucency);
    std::vector<std::uint32_t> translucentIndices = flatten_translucent_point_indices(split.translucentIndices);
    auto opaqueIndexBuffer = opengl::IndexBuffer::create(split.opaqueIndices, accessPattern);
    if (!opaqueIndexBuffer.has_value()) {
        return make_failed_drawable<PointDrawable>();
    }
    auto translucentIndexBuffer = opengl::IndexBuffer::create(translucentIndices, accessPattern);
    if (!translucentIndexBuffer.has_value()) {
        return make_failed_drawable<PointDrawable>();
    }
    return PointDrawable{program,
                         std::move(vertexArray.value()),
                         std::move(vertexBuffer.value()),
                         std::move(colorBuffer.value()),
                         std::move(opaqueIndexBuffer.value()),
                         std::move(translucentIndexBuffer.value()),
                         pointSize,
                         make_drawable_transparency_info(vertices, vertexDimension, colors, colorDimension),
                         vertexDimension,
                         colorDimension,
                         std::move(vertexPositions),
                         std::move(vertexTranslucency),
                         std::move(pointIndices),
                         std::move(split.translucentIndices)};
}

} // namespace opengl
