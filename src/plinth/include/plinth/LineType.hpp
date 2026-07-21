#ifndef PLINTH_LINETYPE_HPP
#define PLINTH_LINETYPE_HPP

namespace renderer {

class LineType {
    enum class Type {
        Lines,
        LineStrip,
        LineLoop,
    };

  public:
    LineType()
        : m_type(Type::Lines) {}

    [[nodiscard]]
    static LineType lines() {
        return LineType(Type::Lines);
    }
    [[nodiscard]]
    static LineType line_strip() {
        return LineType(Type::LineStrip);
    }
    [[nodiscard]]
    static LineType line_loop() {
        return LineType(Type::LineLoop);
    }
    [[nodiscard]]
    bool is_lines() const {
        return m_type == Type::Lines;
    }
    [[nodiscard]]
    bool is_line_strip() const {
        return m_type == Type::LineStrip;
    }
    [[nodiscard]]
    bool is_line_loop() const {
        return m_type == Type::LineLoop;
    }

  private:
    LineType(Type type)
        : m_type(type) {}

    Type m_type;
};

} // namespace renderer

#endif // PLINTH_LINETYPE_HPP
