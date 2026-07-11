#include "OpenGL/Drawable/MeshDrawable.hpp"
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

MeshDrawable::MeshDrawable(MeshProgram& program,
                           VertexArray vertexArray,
                           VertexBuffer vertexBuffer,
                           VertexBuffer vertexNormalsBuffer,
                           VertexBuffer colorBuffer,
                           IndexBuffer triangleIndicesBuffer,
                           DrawableTransparencyInfo transparencyInfo,
                           std::int32_t vertexDimension,
                           std::int32_t colorDimension,
                           std::vector<linal::float3> vertexPositions)
    : m_program(&program)
    , m_vertexArray(std::move(vertexArray))
    , m_vertexBuffer(std::move(vertexBuffer))
    , m_vertexNormalsBuffer(std::move(vertexNormalsBuffer))
    , m_colorBuffer(std::move(colorBuffer))
    , m_triangleIndicesBuffer(std::move(triangleIndicesBuffer))
    , m_vertexDimension(vertexDimension)
    , m_colorDimension(colorDimension)
    , m_transparencyInfo(transparencyInfo)
    , m_vertexPositions(std::move(vertexPositions)) {
}

MeshDrawable::MeshDrawable(MeshDrawable&& other) noexcept
    : m_program(std::exchange(other.m_program, nullptr))
    , m_vertexArray(std::move(other.m_vertexArray))
    , m_vertexBuffer(std::move(other.m_vertexBuffer))
    , m_vertexNormalsBuffer(std::move(other.m_vertexNormalsBuffer))
    , m_colorBuffer(std::move(other.m_colorBuffer))
    , m_triangleIndicesBuffer(std::move(other.m_triangleIndicesBuffer))
    , m_vertexDimension(other.m_vertexDimension)
    , m_colorDimension(other.m_colorDimension)
    , m_transparencyInfo(other.m_transparencyInfo)
    , m_vertexPositions(std::move(other.m_vertexPositions)) {
}

MeshDrawable& MeshDrawable::operator=(MeshDrawable&& other) noexcept {
    if (this != &other) {
        m_program = std::exchange(other.m_program, nullptr);
        m_vertexArray = std::move(other.m_vertexArray);
        m_vertexBuffer = std::move(other.m_vertexBuffer);
        m_vertexNormalsBuffer = std::move(other.m_vertexNormalsBuffer);
        m_colorBuffer = std::move(other.m_colorBuffer);
        m_triangleIndicesBuffer = std::move(other.m_triangleIndicesBuffer);
        m_vertexDimension = other.m_vertexDimension;
        m_colorDimension = other.m_colorDimension;
        m_transparencyInfo = other.m_transparencyInfo;
        m_vertexPositions = std::move(other.m_vertexPositions);
    }
    return *this;
}

void MeshDrawable::update_color_buffer(std::span<const float> colors, BufferAccessPattern accessPattern) {
    m_colorBuffer.update_buffer(colors, accessPattern);
    m_transparencyInfo.isTranslucent = contains_translucent_alpha(colors, m_colorDimension);
}

void MeshDrawable::draw(const linal::hmatf& modelMatrix,
                        const linal::hmatf& viewMatrix,
                        const linal::hmatf& projectionMatrix,
                        const linal::hmatf& normalMatrix,
                        const linal::float3& lightPosition,
                        const linal::float3& viewPos,
                        const linal::float3& lightColor,
                        const linal::float3& fillLightDirection,
                        const linal::float3& fillLightColor,
                        const linal::float3& ambientColor,
                        float shininess) const {
    RENDERER_ASSERT(m_program != nullptr);
    const auto& prog = *m_program;
    prog.use();

    glUniformMatrix4fv(prog.get_model_matrix_location().get_value(), 1, GL_FALSE, modelMatrix.data());
    glUniformMatrix4fv(prog.get_view_matrix_location().get_value(), 1, GL_FALSE, viewMatrix.data());
    glUniformMatrix4fv(prog.get_projection_matrix_location().get_value(), 1, GL_FALSE, projectionMatrix.data());
    glUniformMatrix4fv(prog.get_normal_matrix_location().get_value(), 1, GL_FALSE, normalMatrix.data());

    glUniform3fv(prog.get_light_pos_location().get_value(), 1, lightPosition.data());
    glUniform3fv(prog.get_view_pos_location().get_value(), 1, viewPos.data());
    glUniform3fv(prog.get_light_color_location().get_value(), 1, lightColor.data());
    glUniform3fv(prog.get_fill_light_direction_location().get_value(), 1, fillLightDirection.data());
    glUniform3fv(prog.get_fill_light_color_location().get_value(), 1, fillLightColor.data());
    glUniform3fv(prog.get_ambient_color_location().get_value(), 1, ambientColor.data());
    glUniform1f(prog.get_shininess_location().get_value(), shininess);

    m_vertexArray.bind();
    m_triangleIndicesBuffer.bind();

    glDrawElements(GL_TRIANGLES, m_triangleIndicesBuffer.get_index_count(), GL_UNSIGNED_INT, nullptr);
}

std::optional<MeshDrawable> make_mesh_soup(MeshProgram& program,
                                           std::span<const float> vertices,
                                           std::int32_t vertexDimension,
                                           std::span<const float> normals,
                                           std::span<const float> colors,
                                           std::int32_t colorDimension,
                                           std::span<const std::uint32_t> triangleIndices,
                                           BufferAccessPattern accessPattern) {
    // Order of creation matters! Create the vertex array first, then the buffers.
    auto vertexArray = VertexArray::create();
    if (!vertexArray.has_value()) {
        return make_failed_drawable<MeshDrawable>();
    }
    auto vertexBuffer = VertexBuffer::create(vertices, vertexDimension, program.get_pos_location(), accessPattern);
    if (!vertexBuffer.has_value()) {
        return make_failed_drawable<MeshDrawable>();
    }
    auto vertexNormalsBuffer =
        VertexBuffer::create(normals, vertexDimension, program.get_normal_location(), accessPattern);
    if (!vertexNormalsBuffer.has_value()) {
        return make_failed_drawable<MeshDrawable>();
    }
    auto colorBuffer = VertexBuffer::create(colors, colorDimension, program.get_color_location(), accessPattern);
    if (!colorBuffer.has_value()) {
        return make_failed_drawable<MeshDrawable>();
    }
    auto triangleIndicesBuffer = IndexBuffer::create(triangleIndices, accessPattern);
    if (!triangleIndicesBuffer.has_value()) {
        return make_failed_drawable<MeshDrawable>();
    }
    return MeshDrawable{program,
                        std::move(vertexArray.value()),
                        std::move(vertexBuffer.value()),
                        std::move(vertexNormalsBuffer.value()),
                        std::move(colorBuffer.value()),
                        std::move(triangleIndicesBuffer.value()),
                        make_drawable_transparency_info(vertices, vertexDimension, colors, colorDimension),
                        vertexDimension,
                        colorDimension,
                        make_vertex_sort_positions(vertices, vertexDimension)};
}

} // namespace opengl
