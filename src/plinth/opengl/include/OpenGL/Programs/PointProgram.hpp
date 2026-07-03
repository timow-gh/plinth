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

    Uniform m_mvpLocation;
    Attribute m_vertexLocation;
    Attribute m_colorLocation;

  public:
    PointProgram() noexcept = default;
    PointProgram(ProgramHandle program,
                 Uniform mvpLocation,
                 Attribute vertexLocation,
                 Attribute colorLocation) noexcept;

    PointProgram(const PointProgram&) = delete;
    PointProgram& operator=(const PointProgram&) = delete;
    PointProgram(PointProgram&& other) noexcept {
        m_program = std::move(other.m_program);
        m_mvpLocation = std::move(other.m_mvpLocation);
        m_vertexLocation = std::move(other.m_vertexLocation);
        m_colorLocation = std::move(other.m_colorLocation);
    }
    PointProgram& operator=(PointProgram&& other) noexcept {
        if (this != &other) {
            m_program = std::move(other.m_program);
            m_mvpLocation = std::move(other.m_mvpLocation);
            m_vertexLocation = std::move(other.m_vertexLocation);
            m_colorLocation = std::move(other.m_colorLocation);
        }
        return *this;
    }
    ~PointProgram() {
        if (m_program.is_valid()) {
            glDeleteProgram(m_program.get_id().get_value());
        }
    }

    [[nodiscard]]
    bool is_valid() const noexcept {
        return m_program.is_valid();
    }

    [[nodiscard]]
    constexpr Location get_mvp_location() const noexcept {
        return m_mvpLocation.get_location();
    }
    [[nodiscard]]
    constexpr Location get_pos_location() const noexcept {
        return m_vertexLocation.get_location();
    }
    [[nodiscard]]
    constexpr Location get_color_location() const noexcept {
        return m_colorLocation.get_location();
    }

    void use() const;
};

OPENGL_EXPORT PointProgram make_point_program();

} // namespace opengl

#endif // OPENGL_POINTPROGRAM_HPP
