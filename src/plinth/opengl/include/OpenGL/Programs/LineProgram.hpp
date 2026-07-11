#ifndef OPENGL_LINEPROGRAM_HPP
#define OPENGL_LINEPROGRAM_HPP

#include "OpenGL/Attribute.hpp"
#include "OpenGL/OpenGL.hpp"
#include "OpenGL/Programs/ProgramId.hpp"
#include "OpenGL/Uniform.hpp"
#include "OpenGL/opengl_export.h"
#include <plinth/Assert.hpp>

namespace opengl {

class OPENGL_EXPORT LineProgram {
    ProgramHandle m_program;
    Uniform m_mvpLocation;
    Uniform m_modelMatrixLocation;
    Attribute m_vertexLocation;
    Attribute m_colorLocation;

  public:
    LineProgram() noexcept = default;
    LineProgram(ProgramHandle program,
                Uniform mvpLocation,
                Uniform modelMatrixLocation,
                Attribute vertexLocation,
                Attribute colorLocation) noexcept;

    LineProgram(const LineProgram&) = delete;
    LineProgram& operator=(const LineProgram&) = delete;
    LineProgram(LineProgram&& other) noexcept {
        m_program = std::move(other.m_program);
        m_mvpLocation = std::move(other.m_mvpLocation);
        m_modelMatrixLocation = std::move(other.m_modelMatrixLocation);
        m_vertexLocation = std::move(other.m_vertexLocation);
        m_colorLocation = std::move(other.m_colorLocation);
    }
    LineProgram& operator=(LineProgram&& other) noexcept {
        if (this != &other) {
            m_program = std::move(other.m_program);
            m_mvpLocation = std::move(other.m_mvpLocation);
            m_modelMatrixLocation = std::move(other.m_modelMatrixLocation);
            m_vertexLocation = std::move(other.m_vertexLocation);
            m_colorLocation = std::move(other.m_colorLocation);
        }
        return *this;
    }
    ~LineProgram() = default;

    [[nodiscard]]
    bool is_valid() const noexcept {
        return m_program.is_valid();
    }

    [[nodiscard]]
    constexpr Location get_mvp_location() const {
        return m_mvpLocation.get_location();
    }
    [[nodiscard]]
    constexpr Location get_model_matrix_location() const {
        return m_modelMatrixLocation.get_location();
    }
    [[nodiscard]]
    constexpr Location get_vertex_location() const {
        return m_vertexLocation.get_location();
    }
    [[nodiscard]]
    constexpr Location get_color_location() const {
        return m_colorLocation.get_location();
    }

    void use() const;
};

OPENGL_EXPORT LineProgram make_line_program();

} // namespace opengl

#endif // OPENGL_LINEPROGRAM_HPP
