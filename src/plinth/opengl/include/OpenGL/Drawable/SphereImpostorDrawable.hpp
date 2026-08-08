#ifndef OPENGL_SPHEREIMPOSTORDRAWABLE_HPP
#define OPENGL_SPHEREIMPOSTORDRAWABLE_HPP

#include "OpenGL/BufferAccessPattern.hpp"
#include "OpenGL/Drawable/DrawableTransparencyInfo.hpp"
#include "OpenGL/InstanceBuffer.hpp"
#include "OpenGL/Programs/SphereImpostorProgram.hpp"
#include "OpenGL/VertexArray.hpp"
#include "OpenGL/opengl_export.h"
#include "plinth/LightingConfig.hpp"
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

class OPENGL_EXPORT SphereImpostorDrawable {
  public:
    // Per-sphere translucent data for live re-sorting.
    struct SortableSphereInstance {
        std::array<float, 8> data{};  // interleaved (center.xyz, radius, r, g, b, a)
        linal::float3 sortCenter{0.0F, 0.0F, 0.0F};
    };

  private:
    SphereImpostorProgram* m_program{nullptr};
    VertexArray            m_vertexArray;
    InstanceBuffer         m_opaqueInstanceBuffer;
    InstanceBuffer         m_translucentInstanceBuffer;
    DrawableTransparencyInfo m_transparencyInfo;
    std::vector<SortableSphereInstance> m_translucentSpheres;

    std::vector<linal::float3> m_centers;            // for get_vertex_positions()
    mutable std::vector<float> m_positionsCache;

  public:
    SphereImpostorDrawable(SphereImpostorProgram& program,
                           VertexArray vertexArray,
                           InstanceBuffer opaqueInstanceBuffer,
                           InstanceBuffer translucentInstanceBuffer,
                           DrawableTransparencyInfo transparencyInfo,
                           std::vector<SortableSphereInstance> translucentSpheres,
                           std::vector<linal::float3> centers);

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

    [[nodiscard]] double distance_squared_to(const linal::double3& viewPosition) const noexcept {
        return m_transparencyInfo.distance_squared_to(viewPosition);
    }

    [[nodiscard]] double distance_squared_to(const linal::double3& viewPosition,
                                             const linal::hmatf& transform) const noexcept {
        return m_transparencyInfo.distance_squared_to(viewPosition, transform);
    }

    // World-space xyz-triplet vertex positions for scene-bounds computation.
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
