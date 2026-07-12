#ifndef PLINTH_LINETYPE_HPP
#define PLINTH_LINETYPE_HPP

#include <cstdint>
#include <stdexcept>

namespace renderer {

class LineType {
    enum class Type : std::uint32_t {
        LINES = 0x0001,
        LINE_STRIP = 0x0003,
        LINE_LOOP = 0x0002,
        TRIANGLES = 0x0004,
        TRIANGLE_STRIP = 0x0005,
        TRIANGLE_FAN = 0x0006
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
        case 0x0001: return lines();
        case 0x0003: return line_strip();
        case 0x0002: return line_loop();
        case 0x0004: return triangles();
        case 0x0005: return triangle_strip();
        case 0x0006: return triangle_fan();
        default:      throw std::invalid_argument("Unknown OpenGL line type");
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

} // namespace renderer

#endif // PLINTH_LINETYPE_HPP
