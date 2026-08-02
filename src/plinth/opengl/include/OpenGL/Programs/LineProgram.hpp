#ifndef OPENGL_LINEPROGRAM_HPP
#define OPENGL_LINEPROGRAM_HPP

#include "OpenGL/Attribute.hpp"
#include "OpenGL/OpenGL.hpp"
#include "OpenGL/Programs/ProgramId.hpp"
#include "OpenGL/Uniform.hpp"
#include "OpenGL/opengl_export.h"
#include "plinth/Assert.hpp"
#include <utility>

namespace opengl {

/** @brief GPU program for instanced quad-expanded thick lines with optional dashing.
 *
 * Each line segment is drawn as one instance of a shared unit quad (attribute a_corner, divisor 0)
 * expanded to screen-space thickness in the vertex shader. Per-segment data (a_p0/a_p1/a_color0/
 * a_color1) is supplied through an InstanceBuffer with divisor 1.
 */
class OPENGL_EXPORT LineProgram {
    ProgramHandle m_program;
    Uniform m_viewProjectionLocation;
    Uniform m_modelMatrixLocation;
    Uniform m_viewportSizeLocation;
    Uniform m_lineWidthLocation;
    Uniform m_dashSpaceLocation;
    Uniform m_dashEnabledLocation;
    Uniform m_dashSizeLocation;
    Uniform m_gapSizeLocation;
    Uniform m_dashPhaseLocation;
    Uniform m_pickModeLocation;
    Uniform m_pickColorLocation;
    Attribute m_cornerLocation;
    Attribute m_p0Location;
    Attribute m_p1Location;
    Attribute m_color0Location;
    Attribute m_color1Location;

  public:
    LineProgram() noexcept = default;
    LineProgram(ProgramHandle program,
                Uniform viewProjectionLocation,
                Uniform modelMatrixLocation,
                Uniform viewportSizeLocation,
                Uniform lineWidthLocation,
                Uniform dashSpaceLocation,
                Uniform dashEnabledLocation,
                Uniform dashSizeLocation,
                Uniform gapSizeLocation,
                Uniform dashPhaseLocation,
                Uniform pickModeLocation,
                Uniform pickColorLocation,
                Attribute cornerLocation,
                Attribute p0Location,
                Attribute p1Location,
                Attribute color0Location,
                Attribute color1Location) noexcept;

    LineProgram(const LineProgram&) = delete;
    LineProgram& operator=(const LineProgram&) = delete;
    LineProgram(LineProgram&& other) noexcept {
        move_from(std::move(other));
    }
    LineProgram& operator=(LineProgram&& other) noexcept {
        if (this != &other) {
            move_from(std::move(other));
        }
        return *this;
    }
    ~LineProgram() = default;

    [[nodiscard]]
    bool is_valid() const noexcept {
        return m_program.is_valid();
    }

    [[nodiscard]]
    constexpr Location get_view_projection_location() const {
        return m_viewProjectionLocation.get_location();
    }
    [[nodiscard]]
    constexpr Location get_model_matrix_location() const {
        return m_modelMatrixLocation.get_location();
    }
    [[nodiscard]]
    constexpr Location get_viewport_size_location() const {
        return m_viewportSizeLocation.get_location();
    }
    [[nodiscard]]
    constexpr Location get_line_width_location() const {
        return m_lineWidthLocation.get_location();
    }
    [[nodiscard]]
    constexpr Location get_dash_space_location() const {
        return m_dashSpaceLocation.get_location();
    }
    [[nodiscard]]
    constexpr Location get_dash_enabled_location() const {
        return m_dashEnabledLocation.get_location();
    }
    [[nodiscard]]
    constexpr Location get_dash_size_location() const {
        return m_dashSizeLocation.get_location();
    }
    [[nodiscard]]
    constexpr Location get_gap_size_location() const {
        return m_gapSizeLocation.get_location();
    }
    [[nodiscard]]
    constexpr Location get_dash_phase_location() const {
        return m_dashPhaseLocation.get_location();
    }
    [[nodiscard]]
    constexpr Location get_pick_mode_location() const {
        return m_pickModeLocation.get_location();
    }
    [[nodiscard]]
    constexpr Location get_pick_color_location() const {
        return m_pickColorLocation.get_location();
    }
    [[nodiscard]]
    constexpr Location get_corner_location() const {
        return m_cornerLocation.get_location();
    }
    [[nodiscard]]
    constexpr Location get_p0_location() const {
        return m_p0Location.get_location();
    }
    [[nodiscard]]
    constexpr Location get_p1_location() const {
        return m_p1Location.get_location();
    }
    [[nodiscard]]
    constexpr Location get_color0_location() const {
        return m_color0Location.get_location();
    }
    [[nodiscard]]
    constexpr Location get_color1_location() const {
        return m_color1Location.get_location();
    }

    void use() const;

  private:
    void move_from(LineProgram&& other) noexcept {
        m_program = std::move(other.m_program);
        m_viewProjectionLocation = std::move(other.m_viewProjectionLocation);
        m_modelMatrixLocation = std::move(other.m_modelMatrixLocation);
        m_viewportSizeLocation = std::move(other.m_viewportSizeLocation);
        m_lineWidthLocation = std::move(other.m_lineWidthLocation);
        m_dashSpaceLocation = std::move(other.m_dashSpaceLocation);
        m_dashEnabledLocation = std::move(other.m_dashEnabledLocation);
        m_dashSizeLocation = std::move(other.m_dashSizeLocation);
        m_gapSizeLocation = std::move(other.m_gapSizeLocation);
        m_dashPhaseLocation = std::move(other.m_dashPhaseLocation);
        m_pickModeLocation = std::move(other.m_pickModeLocation);
        m_pickColorLocation = std::move(other.m_pickColorLocation);
        m_cornerLocation = std::move(other.m_cornerLocation);
        m_p0Location = std::move(other.m_p0Location);
        m_p1Location = std::move(other.m_p1Location);
        m_color0Location = std::move(other.m_color0Location);
        m_color1Location = std::move(other.m_color1Location);
    }
};

OPENGL_EXPORT LineProgram make_line_program();

} // namespace opengl

#endif // OPENGL_LINEPROGRAM_HPP
