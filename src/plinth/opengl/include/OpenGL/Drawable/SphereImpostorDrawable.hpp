#ifndef OPENGL_SPHEREIMPOSTORDRAWABLE_HPP
#define OPENGL_SPHEREIMPOSTORDRAWABLE_HPP

#include "OpenGL/BufferAccessPattern.hpp"
#include "OpenGL/Drawable/DrawableTransparencyInfo.hpp"
#include "OpenGL/InstanceBuffer.hpp"
#include "OpenGL/Programs/SphereImpostorProgram.hpp"
#include "OpenGL/VertexArray.hpp"
#include "OpenGL/opengl_export.h"
#include "plinth/LightingConfig.hpp"
#include "plinth/SphereSizeSpace.hpp"
#include "plinth/Warnings.hpp"

#include <linal/hmat.hpp>
#include <linal/vec.hpp>

#include <array>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

RENDERER_DISABLE_ALL_WARNINGS

namespace opengl {

using renderer::LightingConfig;

/** Owns instanced sphere-point data and bridges renderer state to the impostor shader.
 *
 * Centers/radii remain in local space. The shader intersects there so a drawable model transform
 * can represent any non-singular affine sphere transform. CPU sorting is the exception: local
 * centers are transformed to world space before comparison with the world-space camera.
 *
 * Opaque and translucent data are deliberately stored in separate buffers. Translucent instances
 * additionally retain their interleaved CPU record so it can be reordered and streamed as the
 * camera moves. See docs/sphere-point-rendering.md for the complete design and extension contract.
 */
class OPENGL_EXPORT SphereImpostorDrawable {
  public:
    // Keep this record identical to the layout declared in SphereInstanceData.hpp. It is a full
    // record, rather than only an index, because translucent order is uploaded as a flat stream.
    struct SortableSphereInstance {
        std::array<float, 8> data{};  // interleaved (center.xyz, radius, r, g, b, a)
        linal::float3 sortCenter{0.0F, 0.0F, 0.0F}; // local space; transform before sorting
    };

  private:
    SphereImpostorProgram* m_program{nullptr};
    VertexArray            m_vertexArray;
    InstanceBuffer         m_opaqueInstanceBuffer;
    InstanceBuffer         m_translucentInstanceBuffer;
    DrawableTransparencyInfo m_transparencyInfo;
    std::vector<SortableSphereInstance> m_translucentSpheres;

    std::vector<linal::float3> m_centers;            // for get_vertex_positions()
    std::vector<float>         m_radii;              // parallel to m_centers; kept so update_colors
                                                     // can rebuild instance data without the caller
                                                     // re-supplying geometry
    mutable std::vector<float> m_positionsCache;

    // Per-drawable: how a_sphere.w is interpreted. World = local/world-space radius; Screen = a
    // pixel diameter, converted to a view-space radius in the vertex shader. Defaulted to Screen to
    // match the public default (renderer::SphereStyle::sizeSpace); DrawablesManager always sets this
    // explicitly per add, so this initializer is only a safe fallback.
    renderer::SphereSizeSpace m_sizeSpace{renderer::SphereSizeSpace::Screen};

  public:
    SphereImpostorDrawable(SphereImpostorProgram& program,
                           VertexArray vertexArray,
                           InstanceBuffer opaqueInstanceBuffer,
                           InstanceBuffer translucentInstanceBuffer,
                           DrawableTransparencyInfo transparencyInfo,
                           std::vector<SortableSphereInstance> translucentSpheres,
                           std::vector<linal::float3> centers,
                           std::vector<float> radii);

    SphereImpostorDrawable(const SphereImpostorDrawable&) = delete;
    SphereImpostorDrawable& operator=(const SphereImpostorDrawable&) = delete;
    SphereImpostorDrawable(SphereImpostorDrawable&& other) noexcept;
    SphereImpostorDrawable& operator=(SphereImpostorDrawable&& other) noexcept;
    ~SphereImpostorDrawable() = default;

    void draw(const linal::hmatf& viewMatrix,
              const linal::hmatf& projectionMatrix,
              const linal::hmatf& inverseProjectionMatrix,
              const linal::hmatf& modelMatrix,
              const linal::float2& viewportSize,
              bool zeroToOneDepth,
              const LightingConfig& lighting) const;

    void draw_opaque(const linal::hmatf& viewMatrix,
                     const linal::hmatf& projectionMatrix,
                     const linal::hmatf& inverseProjectionMatrix,
                     const linal::hmatf& modelMatrix,
                     const linal::float2& viewportSize,
                     bool zeroToOneDepth,
                     const LightingConfig& lighting) const;

    void draw_translucent(const linal::hmatf& viewMatrix,
                          const linal::hmatf& projectionMatrix,
                          const linal::hmatf& inverseProjectionMatrix,
                          const linal::hmatf& modelMatrix,
                          const linal::float2& viewportSize,
                          bool zeroToOneDepth,
                          const LightingConfig& lighting,
                          const linal::double3& viewPosition);

    void draw_pick(const linal::hmatf& viewMatrix,
                   const linal::hmatf& projectionMatrix,
                   const linal::hmatf& inverseProjectionMatrix,
                   const linal::hmatf& modelMatrix,
                   const linal::float2& viewportSize,
                   bool zeroToOneDepth,
                   const std::array<float, 3>& pickColor) const;

    [[nodiscard]] bool has_opaque_primitives() const noexcept {
        return m_opaqueInstanceBuffer.get_instance_count() > 0;
    }

    [[nodiscard]] bool has_translucent_primitives() const noexcept {
        return m_translucentInstanceBuffer.get_instance_count() > 0;
    }

    [[nodiscard]] bool is_translucent() const noexcept { return has_translucent_primitives(); }

    void set_size_space(renderer::SphereSizeSpace space) noexcept { m_sizeSpace = space; }
    [[nodiscard]] renderer::SphereSizeSpace get_size_space() const noexcept { return m_sizeSpace; }

    // Replaces only the per-instance colors, keeping the retained centers/radii. colors is N*4
    // (rgba per sphere) where N is the current sphere count. Because alpha can move a sphere between
    // the opaque and translucent buffers, this re-splits and re-uploads both instance buffers (via
    // InstanceBuffer::update, so no VAO/attribute recreation) and refreshes transparency info.
    // Returns false (leaving state unchanged) if colors has the wrong length.
    bool update_colors(std::span<const float> colors, BufferAccessPattern accessPattern);

    [[nodiscard]] double distance_squared_to(const linal::double3& viewPosition) const noexcept {
        return m_transparencyInfo.distance_squared_to(viewPosition);
    }

    [[nodiscard]] double distance_squared_to(const linal::double3& viewPosition,
                                             const linal::hmatf& transform) const noexcept {
        return m_transparencyInfo.distance_squared_to(viewPosition, transform);
    }

    // Local-space xyz-triplet centers for scene-bounds computation; the manager applies the model
    // transform. The generic bounds interface has no extent channel, so radii are not represented.
    [[nodiscard]] std::span<const float> get_vertex_positions() const noexcept {
        m_positionsCache.clear();
        m_positionsCache.reserve(m_centers.size() * 3U);
        for (const auto& p: m_centers) {
            m_positionsCache.push_back(p[0]);
            m_positionsCache.push_back(p[1]);
            m_positionsCache.push_back(p[2]);
        }
        return m_positionsCache;
    }

  private:
    void set_common_uniforms(const linal::hmatf& viewMatrix,
                             const linal::hmatf& projectionMatrix,
                             const linal::hmatf& inverseProjectionMatrix,
                             const linal::hmatf& modelMatrix,
                             const linal::float2& viewportSize,
                             bool zeroToOneDepth,
                             const LightingConfig& lighting) const;

    void draw_instances(const InstanceBuffer& instanceBuffer) const;
};

OPENGL_EXPORT std::optional<SphereImpostorDrawable>
make_sphere_impostor_drawable(SphereImpostorProgram& program,
                              std::span<const float> centers,
                              std::span<const float> radii,
                              std::span<const float> colors,
                              BufferAccessPattern accessPattern);

} // namespace opengl

RENDERER_ENABLE_ALL_WARNINGS

#endif // OPENGL_SPHEREIMPOSTORDRAWABLE_HPP
