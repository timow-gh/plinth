#ifndef OPENGL_POINTSOUP_HPP
#define OPENGL_POINTSOUP_HPP

#include "OpenGL/BufferAccessPattern.hpp"
#include "OpenGL/Drawable/DrawableTransparencyInfo.hpp"
#include "OpenGL/IndexBuffer.hpp"
#include "OpenGL/Programs/PointProgram.hpp"
#include "OpenGL/UpdateBuffer.hpp"
#include "OpenGL/VertexArray.hpp"
#include "OpenGL/VertexBuffer.hpp"
#include "OpenGL/opengl_export.h"
#include <Renderer/Warnings.hpp>
#include <cstdint>
#include <linal/hmat.hpp>
#include <span>
#include <utility>
#include <vector>

RENDERER_DISABLE_ALL_WARNINGS

namespace opengl {

class OPENGL_EXPORT PointDrawable {
    PointProgram* m_program{nullptr};
    opengl::VertexArray m_vertexArray;
    opengl::VertexBuffer m_vertexBuffer;
    opengl::VertexBuffer m_colorBuffer;
    opengl::IndexBuffer m_opaquePointIndicesBuffer;
    opengl::IndexBuffer m_translucentPointIndicesBuffer;
    float m_pointSize{1.0F};
    std::int32_t m_vertexDimension{0};
    std::int32_t m_colorDimension{0};
    std::vector<linal::float3> m_vertexPositions;
    std::vector<std::uint8_t> m_vertexTranslucency;
    std::vector<std::uint32_t> m_pointIndices;
    std::vector<SortablePointIndex> m_translucentPointIndices;
    DrawableTransparencyInfo m_transparencyInfo;

  public:
    PointDrawable(PointProgram& program,
                  VertexArray vertexArray,
                  VertexBuffer vertexBuffer,
                  VertexBuffer colorBuffer,
                  IndexBuffer opaquePointIndicesBuffer,
                  IndexBuffer translucentPointIndicesBuffer,
                  float pointSize = 1.0F,
                  DrawableTransparencyInfo transparencyInfo = {},
                  std::int32_t vertexDimension = 3,
                  std::int32_t colorDimension = 4,
                  std::vector<linal::float3> vertexPositions = {},
                  std::vector<std::uint8_t> vertexTranslucency = {},
                  std::vector<std::uint32_t> pointIndices = {},
                  std::vector<SortablePointIndex> translucentPointIndices = {}) noexcept;

    constexpr PointDrawable(const PointDrawable& other) = delete;
    constexpr PointDrawable& operator=(const PointDrawable& other) = delete;
    PointDrawable(PointDrawable&& other) noexcept;
    PointDrawable& operator=(PointDrawable&& other) noexcept;
    ~PointDrawable() = default;

    constexpr void set_point_size(float pointSize) noexcept { m_pointSize = pointSize; }

    void update_vertex_buffer(std::span<const float> vertices, BufferAccessPattern accessPattern);

    void update_color_buffer(std::span<const float> colors, BufferAccessPattern accessPattern);

    void update_indices_buffer(std::span<const std::uint32_t> indices, BufferAccessPattern accessPattern);

    void update_point_drawable(std::span<const float> vertices,
                               std::span<const float> colors,
                               std::span<const std::uint32_t> indices,
                               BufferAccessPattern accessPattern);

    void draw(const linal::hmatf& mvp) const;

    void draw_opaque(const linal::hmatf& mvp) const;

    void draw_translucent(const linal::hmatf& mvp, const linal::double3& viewPosition);

    [[nodiscard]]
    bool has_opaque_primitives() const noexcept {
        return m_opaquePointIndicesBuffer.get_index_count() > 0;
    }

    [[nodiscard]]
    bool has_translucent_primitives() const noexcept {
        return m_translucentPointIndicesBuffer.get_index_count() > 0;
    }

    [[nodiscard]]
    bool is_translucent() const noexcept {
        return has_translucent_primitives();
    }

    [[nodiscard]]
    double distance_squared_to(const linal::double3& viewPosition) const noexcept {
        return m_transparencyInfo.distance_squared_to(viewPosition);
    }

  private:
    void draw_index_buffer(const linal::hmatf& mvp, const IndexBuffer& indexBuffer) const;

    void rebuild_index_buffers(BufferAccessPattern accessPattern);
};

OPENGL_EXPORT std::optional<PointDrawable> make_point_drawable(PointProgram& program,
                                                               std::span<const float> vertices,
                                                               std::int32_t vertexDimension,
                                                               std::span<const float> colors,
                                                               std::int32_t colorDimension,
                                                               std::span<const std::uint32_t> indices,
                                                               float pointSize,
                                                               BufferAccessPattern accessPattern);

} // namespace opengl

RENDERER_ENABLE_ALL_WARNINGS

#endif // OPENGL_POINTSOUP_HPP
