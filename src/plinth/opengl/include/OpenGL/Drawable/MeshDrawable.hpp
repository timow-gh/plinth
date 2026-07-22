#ifndef OPENGL_MESHSOUP_HPP
#define OPENGL_MESHSOUP_HPP

#include "OpenGL/Drawable/DrawableTransparencyInfo.hpp"
#include "OpenGL/IndexBuffer.hpp"
#include "OpenGL/Programs/MeshProgram.hpp"
#include "OpenGL/VertexArray.hpp"
#include "OpenGL/VertexBuffer.hpp"
#include "OpenGL/Texture2D.hpp"
#include "OpenGL/opengl_export.h"
#include <plinth/Warnings.hpp>
#include <cstdint>
#include <linal/hmat.hpp>
#include <linal/vec.hpp>
#include <memory>
#include <span>
#include <vector>
RENDERER_DISABLE_ALL_WARNINGS

namespace opengl {

class OPENGL_EXPORT MeshDrawable {
    MeshProgram* m_program{nullptr};
    VertexArray m_vertexArray;
    VertexBuffer m_vertexBuffer;
    VertexBuffer m_vertexNormalsBuffer;
    VertexBuffer m_colorBuffer;
    VertexBuffer m_textureCoordinateBuffer;
    IndexBuffer m_triangleIndicesBuffer;
    std::shared_ptr<Texture2D> m_texture;
    std::int32_t m_vertexDimension{0};
    std::int32_t m_colorDimension{0};
    DrawableTransparencyInfo m_transparencyInfo;
    std::vector<linal::float3> m_vertexPositions;
    mutable std::vector<float> m_vertexPositionsFlatCache;

  public:
    MeshDrawable(MeshProgram& program,
                 VertexArray vertexArray,
                 VertexBuffer vertexBuffer,
                 VertexBuffer vertexNormalsBuffer,
                  VertexBuffer colorBuffer,
                  VertexBuffer textureCoordinateBuffer,
                  IndexBuffer triangleIndicesBuffer,
                  std::shared_ptr<Texture2D> texture = {},
                 DrawableTransparencyInfo transparencyInfo = {},
                 std::int32_t vertexDimension = 3,
                 std::int32_t colorDimension = 4,
                 std::vector<linal::float3> vertexPositions = {});

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
              float shininess,
              const linal::float3& lightAttenuation,
              const linal::float3& materialAmbient,
              const linal::float3& materialDiffuse,
              const linal::float3& materialSpecular) const;

    [[nodiscard]]
    bool is_translucent() const noexcept {
        return m_transparencyInfo.isTranslucent;
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
};

OPENGL_EXPORT std::optional<MeshDrawable> make_mesh_soup(MeshProgram& program,
                                                          std::span<const float> vertices,
                                                          std::int32_t vertexDimension,
                                                          std::span<const float> normals,
                                                          std::span<const float> colors,
                                                          std::int32_t colorDimension,
                                                          std::span<const std::uint32_t> triangleIndices,
                                                          BufferAccessPattern accessPattern);

OPENGL_EXPORT std::optional<MeshDrawable> make_mesh_soup(MeshProgram& program,
                                                          std::span<const float> vertices,
                                                         std::int32_t vertexDimension,
                                                          std::span<const float> normals,
                                                          std::span<const float> textureCoordinates,
                                                          std::span<const float> colors,
                                                         std::int32_t colorDimension,
                                                         std::span<const std::uint32_t> triangleIndices,
                                                          BufferAccessPattern accessPattern,
                                                          std::shared_ptr<Texture2D> texture = {});

RENDERER_ENABLE_ALL_WARNINGS

} // namespace opengl

#endif // OPENGL_MESHSOUP_HPP
