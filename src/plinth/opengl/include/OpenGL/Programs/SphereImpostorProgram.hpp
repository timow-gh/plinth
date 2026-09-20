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
 * fragment shader transforms the view ray into sphere-local space, writes gl_FragDepth for correct
 * occlusion, and computes Blinn-Phong shading in view space.
 *
 * This class is the C++ half of a runtime shader interface. When the shader contract changes, keep
 * its members, constructor/move plumbing, factory lookups, drawable uploads, and graphics tests in
 * sync. See docs/sphere-point-rendering.md for that checklist and the coordinate-space contract.
 */
class OPENGL_EXPORT SphereImpostorProgram {
    ProgramHandle m_program;

    // Matrix uniforms are used across both shader stages. In particular, inverse model-view is
    // required by visible and picking passes; the normal matrix is required only for lighting.
    // view/projection/inverse-projection are frame-constant and live in the FrameBlock UBO.
    Uniform m_modelMatrixLocation;
    Uniform m_inverseModelViewMatrixLocation;
    Uniform m_normalMatrixLocation;

    // Fragment uniforms
    Uniform m_sizeSpaceLocation;
    Uniform m_pickModeLocation;
    Uniform m_pickColorLocation;

    // Per-instance attributes (divisor 1)
    Attribute m_sphereLocation; // a_sphere: vec4(center.xyz, radius)
    Attribute m_colorLocation;  // a_color:  vec4(r, g, b, a)

  public:
    SphereImpostorProgram() noexcept = default;
    SphereImpostorProgram(ProgramHandle program,
                          Uniform modelMatrixLocation,
                          Uniform inverseModelViewMatrixLocation,
                          Uniform normalMatrixLocation,
                          Uniform sizeSpaceLocation,
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
    [[nodiscard]] constexpr Location get_inverse_model_view_matrix_location() const noexcept {
        return m_inverseModelViewMatrixLocation.get_location();
    }
    [[nodiscard]] constexpr Location get_normal_matrix_location() const noexcept {
        return m_normalMatrixLocation.get_location();
    }
    [[nodiscard]] constexpr Location get_size_space_location() const noexcept {
        return m_sizeSpaceLocation.get_location();
    }
    [[nodiscard]] constexpr Location get_pick_mode_location() const noexcept {
        return m_pickModeLocation.get_location();
    }
    [[nodiscard]] constexpr Location get_pick_color_location() const noexcept {
        return m_pickColorLocation.get_location();
    }
    [[nodiscard]] constexpr Location get_sphere_location() const noexcept { return m_sphereLocation.get_location(); }
    [[nodiscard]] constexpr Location get_color_location() const noexcept { return m_colorLocation.get_location(); }

    void use() const;

  private:
    void move_from(SphereImpostorProgram&& other) noexcept {
        m_program = std::move(other.m_program);
        m_modelMatrixLocation = std::move(other.m_modelMatrixLocation);
        m_inverseModelViewMatrixLocation = std::move(other.m_inverseModelViewMatrixLocation);
        m_normalMatrixLocation = std::move(other.m_normalMatrixLocation);
        m_sizeSpaceLocation = std::move(other.m_sizeSpaceLocation);
        m_pickModeLocation = std::move(other.m_pickModeLocation);
        m_pickColorLocation = std::move(other.m_pickColorLocation);
        m_sphereLocation = std::move(other.m_sphereLocation);
        m_colorLocation = std::move(other.m_colorLocation);
    }
};

OPENGL_EXPORT SphereImpostorProgram make_sphere_impostor_program();

} // namespace opengl

#endif // OPENGL_SPHEREIMPOSTORPROGRAM_HPP
