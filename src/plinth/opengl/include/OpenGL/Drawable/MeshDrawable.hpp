#ifndef OPENGL_MESHSOUP_HPP
#define OPENGL_MESHSOUP_HPP

#include "OpenGL/Drawable/DrawableTransparencyInfo.hpp"
#include "OpenGL/IndexBuffer.hpp"
#include "OpenGL/Programs/MeshProgram.hpp"
#include "OpenGL/VertexArray.hpp"
#include "OpenGL/VertexBuffer.hpp"
#include "OpenGL/opengl_export.h"
#include <Renderer/Warnings.hpp>
#include <cstdint>
#include <linal/hmat.hpp>
#include <span>
RENDERER_DISABLE_ALL_WARNINGS

namespace opengl {

class OPENGL_EXPORT MeshDrawable {
    MeshProgram* m_program{nullptr};
    VertexArray m_vertexArray;
    VertexBuffer m_vertexBuffer;
    VertexBuffer m_vertexNormalsBuffer;
    VertexBuffer m_colorBuffer;
    IndexBuffer m_triangleIndicesBuffer;
    std::int32_t m_vertexDimension{0};
    std::int32_t m_colorDimension{0};
    DrawableTransparencyInfo m_transparencyInfo;

  public:
    MeshDrawable(MeshProgram& program,
                 VertexArray vertexArray,
                 VertexBuffer vertexBuffer,
                 VertexBuffer vertexNormalsBuffer,
                 VertexBuffer colorBuffer,
                 IndexBuffer triangleIndicesBuffer,
                 DrawableTransparencyInfo transparencyInfo = {},
                 std::int32_t vertexDimension = 3,
                 std::int32_t colorDimension = 4);

    MeshDrawable(const MeshDrawable&) = delete;
    MeshDrawable& operator=(const MeshDrawable&) = delete;
    MeshDrawable(MeshDrawable&& other) noexcept;
    MeshDrawable& operator=(MeshDrawable&& other) noexcept;
    ~MeshDrawable() = default;

    void update_color_buffer(std::span<const float> colors, BufferAccessPattern accessPattern);

    /** Draw the mesh Drawable.
     * @param mvp Model-View-Projection matrix.
     * @param viewPos View position (camera position) in world space.
     */
    void draw(const linal::hmatf& modelMatrix,
              const linal::hmatf& viewMatrix,
              const linal::hmatf& projectionMatrix,
              const linal::hmatf& normalMatrix,
              const linal::float3& lightPosition,
              const linal::float3& viewPos,
              const linal::float3& lightColor,
              const linal::float3& fillLightDirection,
              const linal::float3& fillLightColor,
              const linal::float3& ambientColor,
              float shininess) const;

    [[nodiscard]]
    bool is_translucent() const noexcept {
        return m_transparencyInfo.isTranslucent;
    }

    [[nodiscard]]
    double distance_squared_to(const linal::double3& viewPosition) const noexcept {
        return m_transparencyInfo.distance_squared_to(viewPosition);
    }
};

OPENGL_EXPORT std::optional<MeshDrawable> make_mesh_soup(MeshProgram& program,
                                                         std::span<const float> vertices,
                                                         std::int32_t vertexDimension,
                                                         std::span<const float> normals,
                                                         std::span<const float> colors,
                                                         std::int32_t colorDimension,
                                                         std::span<const std::uint32_t> triangleIndices,
                                                         BufferAccessPattern accessPattern);

RENDERER_ENABLE_ALL_WARNINGS

} // namespace opengl

#endif // OPENGL_MESHSOUP_HPP
