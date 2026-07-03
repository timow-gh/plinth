#ifndef LINETYPE_HPP
#define LINETYPE_HPP

#include <OpenGL/OpenGL.hpp>
#include <OpenGL/opengl_export.h>
#include <cstdint>
#include <stdexcept>

namespace opengl {

class LineType {
    enum class OPENGL_EXPORT Type : std::uint32_t {
        LINES = GL_LINES,
        LINE_STRIP = GL_LINE_STRIP,
        LINE_LOOP = GL_LINE_LOOP,
        TRIANGLES = GL_TRIANGLES,
        TRIANGLE_STRIP = GL_TRIANGLE_STRIP,
        TRIANGLE_FAN = GL_TRIANGLE_FAN
    };

  public:
    LineType()
        : m_type(Type::LINES) {}

    [[nodiscard]]
    static LineType lines() {
        return LineType(Type::LINES);
    }
    [[nodiscard]]
    static LineType line_strip() {
        return LineType(Type::LINE_STRIP);
    }
    [[nodiscard]]
    static LineType line_loop() {
        return LineType(Type::LINE_LOOP);
    }
    [[nodiscard]]
    static LineType triangles() {
        return LineType(Type::TRIANGLES);
    }
    [[nodiscard]]
    static LineType triangle_strip() {
        return LineType(Type::TRIANGLE_STRIP);
    }
    [[nodiscard]]
    static LineType triangle_fan() {
        return LineType(Type::TRIANGLE_FAN);
    }

    [[nodiscard]]
    static LineType from_gl_type(std::uint32_t gl_type) {
        switch (gl_type) {
        case GL_LINES:          return lines();
        case GL_LINE_STRIP:     return line_strip();
        case GL_LINE_LOOP:      return line_loop();
        case GL_TRIANGLES:      return triangles();
        case GL_TRIANGLE_STRIP: return triangle_strip();
        case GL_TRIANGLE_FAN:   return triangle_fan();
        default:                throw std::invalid_argument("Unknown OpenGL line type");
        }
    }

    [[nodiscard]]
    std::uint32_t get_gl_type() const {
        return static_cast<std::uint32_t>(m_type);
    }

  private:
    LineType(Type type)
        : m_type(type) {}

    Type m_type;
};

} // namespace opengl

#endif // LINETYPE_HPP
