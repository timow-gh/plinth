#ifndef OPENGL_SPHEREIMPOSTORPROGRAM_HPP
#define OPENGL_SPHEREIMPOSTORPROGRAM_HPP

#include "OpenGL/Attribute.hpp"
#include "OpenGL/OpenGL.hpp"
#include "OpenGL/Programs/ProgramId.hpp"
#include "OpenGL/Uniform.hpp"
#include "OpenGL/opengl_export.h"
#include "plinth/Assert.hpp"

#include <utility>

namespace opengl {

/** @brief Program for sphere impostor rendering.
 *
 * Each sphere is drawn as one instance of a hard-coded 2-triangle quad (6 vertices derived from
 * gl_VertexID). The vertex shader expands the sphere center to a screen-aligned billboard; the
 * fragment shader performs a view-space ray-sphere intersection, writes gl_FragDepth for correct
 * occlusion, and computes Blinn-Phong shading.
 */
class OPENGL_EXPORT SphereImpostorProgram {
    ProgramHandle m_program;

    // Vertex uniforms
    Uniform m_modelMatrixLocation;
    Uniform m_viewMatrixLocation;
    Uniform m_projectionMatrixLocation;

    // Fragment uniforms
    Uniform m_invProjectionLocation;
    Uniform m_viewportSizeLocation;
    Uniform m_zeroToOneDepthLocation;
    Uniform m_lightPosLocation;
    Uniform m_lightColorLocation;
    Uniform m_fillLightDirectionLocation;
    Uniform m_fillLightColorLocation;
    Uniform m_ambientColorLocation;
    Uniform m_shininessLocation;
    Uniform m_lightAttenuationLocation;
    Uniform m_materialAmbientLocation;
    Uniform m_materialDiffuseLocation;
    Uniform m_materialSpecularLocation;
    Uniform m_pickModeLocation;
    Uniform m_pickColorLocation;

    // Per-instance attributes (divisor 1)
    Attribute m_sphereLocation; // a_sphere: vec4(center.xyz, radius)
    Attribute m_colorLocation;  // a_color:  vec4(r, g, b, a)

  public:
    SphereImpostorProgram() noexcept = default;
    SphereImpostorProgram(ProgramHandle program,
                          Uniform modelMatrixLocation,
                          Uniform viewMatrixLocation,
                          Uniform projectionMatrixLocation,
                          Uniform invProjectionLocation,
                          Uniform viewportSizeLocation,
                          Uniform zeroToOneDepthLocation,
                          Uniform lightPosLocation,
                          Uniform lightColorLocation,
                          Uniform fillLightDirectionLocation,
                          Uniform fillLightColorLocation,
                          Uniform ambientColorLocation,
                          Uniform shininessLocation,
                          Uniform lightAttenuationLocation,
                          Uniform materialAmbientLocation,
                          Uniform materialDiffuseLocation,
                          Uniform materialSpecularLocation,
                          Uniform pickModeLocation,
                          Uniform pickColorLocation,
                          Attribute sphereLocation,
                          Attribute colorLocation) noexcept;

    SphereImpostorProgram(const SphereImpostorProgram&) = delete;
    SphereImpostorProgram& operator=(const SphereImpostorProgram&) = delete;
    SphereImpostorProgram(SphereImpostorProgram&& other) noexcept { move_from(std::move(other)); }
    SphereImpostorProgram& operator=(SphereImpostorProgram&& other) noexcept {
        if (this != &other) {
            move_from(std::move(other));
        }
        return *this;
    }
    ~SphereImpostorProgram() = default;

    [[nodiscard]] bool is_valid() const noexcept { return m_program.is_valid(); }

    [[nodiscard]] constexpr Location get_model_matrix_location() const noexcept {
        return m_modelMatrixLocation.get_location();
    }
    [[nodiscard]] constexpr Location get_view_matrix_location() const noexcept {
        return m_viewMatrixLocation.get_location();
    }
    [[nodiscard]] constexpr Location get_projection_matrix_location() const noexcept {
        return m_projectionMatrixLocation.get_location();
    }
    [[nodiscard]] constexpr Location get_inv_projection_location() const noexcept {
        return m_invProjectionLocation.get_location();
    }
    [[nodiscard]] constexpr Location get_viewport_size_location() const noexcept {
        return m_viewportSizeLocation.get_location();
    }
    [[nodiscard]] constexpr Location get_zero_to_one_depth_location() const noexcept {
        return m_zeroToOneDepthLocation.get_location();
    }
    [[nodiscard]] constexpr Location get_light_pos_location() const noexcept {
        return m_lightPosLocation.get_location();
    }
    [[nodiscard]] constexpr Location get_light_color_location() const noexcept {
        return m_lightColorLocation.get_location();
    }
    [[nodiscard]] constexpr Location get_fill_light_direction_location() const noexcept {
        return m_fillLightDirectionLocation.get_location();
    }
    [[nodiscard]] constexpr Location get_fill_light_color_location() const noexcept {
        return m_fillLightColorLocation.get_location();
    }
    [[nodiscard]] constexpr Location get_ambient_color_location() const noexcept {
        return m_ambientColorLocation.get_location();
    }
    [[nodiscard]] constexpr Location get_shininess_location() const noexcept {
        return m_shininessLocation.get_location();
    }
    [[nodiscard]] constexpr Location get_light_attenuation_location() const noexcept {
        return m_lightAttenuationLocation.get_location();
    }
    [[nodiscard]] constexpr Location get_material_ambient_location() const noexcept {
        return m_materialAmbientLocation.get_location();
    }
    [[nodiscard]] constexpr Location get_material_diffuse_location() const noexcept {
        return m_materialDiffuseLocation.get_location();
    }
    [[nodiscard]] constexpr Location get_material_specular_location() const noexcept {
        return m_materialSpecularLocation.get_location();
    }
    [[nodiscard]] constexpr Location get_pick_mode_location() const noexcept {
        return m_pickModeLocation.get_location();
    }
    [[nodiscard]] constexpr Location get_pick_color_location() const noexcept {
        return m_pickColorLocation.get_location();
    }
    [[nodiscard]] constexpr Location get_sphere_location() const noexcept {
        return m_sphereLocation.get_location();
    }
    [[nodiscard]] constexpr Location get_color_location() const noexcept {
        return m_colorLocation.get_location();
    }

    void use() const;

  private:
    void move_from(SphereImpostorProgram&& other) noexcept {
        m_program = std::move(other.m_program);
        m_modelMatrixLocation = std::move(other.m_modelMatrixLocation);
        m_viewMatrixLocation = std::move(other.m_viewMatrixLocation);
        m_projectionMatrixLocation = std::move(other.m_projectionMatrixLocation);
        m_invProjectionLocation = std::move(other.m_invProjectionLocation);
        m_viewportSizeLocation = std::move(other.m_viewportSizeLocation);
        m_zeroToOneDepthLocation = std::move(other.m_zeroToOneDepthLocation);
        m_lightPosLocation = std::move(other.m_lightPosLocation);
        m_lightColorLocation = std::move(other.m_lightColorLocation);
        m_fillLightDirectionLocation = std::move(other.m_fillLightDirectionLocation);
        m_fillLightColorLocation = std::move(other.m_fillLightColorLocation);
        m_ambientColorLocation = std::move(other.m_ambientColorLocation);
        m_shininessLocation = std::move(other.m_shininessLocation);
        m_lightAttenuationLocation = std::move(other.m_lightAttenuationLocation);
        m_materialAmbientLocation = std::move(other.m_materialAmbientLocation);
        m_materialDiffuseLocation = std::move(other.m_materialDiffuseLocation);
        m_materialSpecularLocation = std::move(other.m_materialSpecularLocation);
        m_pickModeLocation = std::move(other.m_pickModeLocation);
        m_pickColorLocation = std::move(other.m_pickColorLocation);
        m_sphereLocation = std::move(other.m_sphereLocation);
        m_colorLocation = std::move(other.m_colorLocation);
    }
};

OPENGL_EXPORT SphereImpostorProgram make_sphere_impostor_program();

} // namespace opengl

#endif // OPENGL_SPHEREIMPOSTORPROGRAM_HPP
