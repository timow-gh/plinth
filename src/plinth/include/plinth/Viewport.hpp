#ifndef RENDERER_VIEWPORT_HPP
#define RENDERER_VIEWPORT_HPP

#include <plinth/Assert.hpp>
#include <cstdint>

namespace renderer {

class Viewport {
    std::uint32_t m_position_x{0};
    std::uint32_t m_position_y{0};
    std::uint32_t m_width{800};
    std::uint32_t m_height{600};

  public:
    using value_type = std::uint32_t;

    Viewport() = default;

    Viewport(value_type positionX, value_type positionY, value_type width, value_type height)
        : m_position_x(positionX)
        , m_position_y(positionY)
        , m_width(width)
        , m_height(height) {
        RENDERER_ASSERT(width > 0 && height > 0);
    }

    [[nodiscard]]
    value_type get_xpos() const {
        return m_position_x;
    }
    [[nodiscard]]
    value_type get_ypos() const {
        return m_position_y;
    }
    [[nodiscard]]
    value_type get_width() const {
        return m_width;
    }
    [[nodiscard]]
    value_type get_height() const {
        return m_height;
    }

    // clang-format off
  void set_position_x(value_type positionX) { m_position_x = positionX; }
  void set_position_y(value_type positionY) { m_position_y = positionY; }
  void set_width(value_type width) { RENDERER_ASSERT(width > 0); m_width = width; }
  void set_height(value_type height) { RENDERER_ASSERT(height > 0); m_height = height; }
    // clang-format on

    [[nodiscard]]
    double get_aspect_ratio() const {
        RENDERER_ASSERT(m_height > 0);
        if (m_height == 0) {
            return 1.0;
        }
        return static_cast<double>(m_width) / static_cast<double>(m_height);
    }
};

} // namespace renderer

#endif // RENDERER_VIEWPORT_HPP
