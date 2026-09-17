#ifndef OPENGL_POINTPROGRAM_HPP
#define OPENGL_POINTPROGRAM_HPP

#include "OpenGL/Attribute.hpp"
#include "OpenGL/OpenGL.hpp"
#include "OpenGL/Programs/ProgramId.hpp"
#include "OpenGL/Uniform.hpp"
#include "OpenGL/opengl_export.h"

namespace opengl {

class OPENGL_EXPORT PointProgram {
    ProgramHandle m_program;

    Uniform m_viewProjectionLocation;
    Uniform m_modelMatrixLocation;
    Attribute m_vertexLocation;
    Attribute m_colorLocation;
    Uniform m_pickModeLocation;
    Uniform m_pickColorLocation;
    Uniform m_depthBiasLocation;
    bool m_reversedDepth{false};

  public:
    PointProgram() noexcept = default;
    PointProgram(ProgramHandle program,
                 Uniform viewProjectionLocation,
                 Uniform modelMatrixLocation,
                 Attribute vertexLocation,
                 Attribute colorLocation,
                 Uniform pickModeLocation,
                 Uniform pickColorLocation,
                 Uniform depthBiasLocation) noexcept;

    PointProgram(const PointProgram&) = delete;
    PointProgram& operator=(const PointProgram&) = delete;
    PointProgram(PointProgram&& other) noexcept {
        m_program = std::move(other.m_program);
        m_viewProjectionLocation = std::move(other.m_viewProjectionLocation);
        m_modelMatrixLocation = std::move(other.m_modelMatrixLocation);
        m_vertexLocation = std::move(other.m_vertexLocation);
        m_colorLocation = std::move(other.m_colorLocation);
        m_pickModeLocation = std::move(other.m_pickModeLocation);
        m_pickColorLocation = std::move(other.m_pickColorLocation);
        m_depthBiasLocation = std::move(other.m_depthBiasLocation);
        m_reversedDepth = other.m_reversedDepth;
    }
    PointProgram& operator=(PointProgram&& other) noexcept {
        if (this != &other) {
            m_program = std::move(other.m_program);
            m_viewProjectionLocation = std::move(other.m_viewProjectionLocation);
            m_modelMatrixLocation = std::move(other.m_modelMatrixLocation);
            m_vertexLocation = std::move(other.m_vertexLocation);
            m_colorLocation = std::move(other.m_colorLocation);
            m_pickModeLocation = std::move(other.m_pickModeLocation);
            m_pickColorLocation = std::move(other.m_pickColorLocation);
            m_depthBiasLocation = std::move(other.m_depthBiasLocation);
            m_reversedDepth = other.m_reversedDepth;
        }
        return *this;
    }
    ~PointProgram() = default;

    [[nodiscard]] bool is_valid() const noexcept { return m_program.is_valid(); }

    [[nodiscard]] constexpr Location get_view_projection_location() const noexcept {
        return m_viewProjectionLocation.get_location();
    }
    [[nodiscard]] constexpr Location get_model_matrix_location() const noexcept {
        return m_modelMatrixLocation.get_location();
    }
    [[nodiscard]] constexpr Location get_pos_location() const noexcept { return m_vertexLocation.get_location(); }
    [[nodiscard]] constexpr Location get_color_location() const noexcept { return m_colorLocation.get_location(); }
    [[nodiscard]] constexpr Location get_pick_mode_location() const noexcept {
        return m_pickModeLocation.get_location();
    }
    [[nodiscard]] constexpr Location get_pick_color_location() const noexcept {
        return m_pickColorLocation.get_location();
    }
    [[nodiscard]] constexpr Location get_depth_bias_location() const noexcept {
        return m_depthBiasLocation.get_location();
    }

    /// Reversed-Z is a lifetime-constant GPU property; it selects the sign of the depth-bias baked
    /// into u_depthBias on the CPU so opted-in coplanar points are pushed toward the camera under
    /// both depth conventions. Mirrors LineProgram::set_reversed_depth.
    void set_reversed_depth(bool reversedDepth) noexcept { m_reversedDepth = reversedDepth; }
    [[nodiscard]] bool get_reversed_depth() const noexcept { return m_reversedDepth; }

    void use() const;
};

OPENGL_EXPORT PointProgram make_point_program();

} // namespace opengl

#endif // OPENGL_POINTPROGRAM_HPP
