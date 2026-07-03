#ifndef RENDERER_PLANE_HPP
#define RENDERER_PLANE_HPP

#include <linal/linal.hpp>

namespace renderer {

class Plane {
    linal::double3 m_origin{0.0, 0.0, 0.0};
    linal::double3 m_normal{0.0, 0.0, 1.0};

  public:
    Plane() = default;
    Plane(const linal::double3& origin, const linal::double3& normal)
        : m_origin(origin)
        , m_normal(normal) {}

    [[nodiscard]]
    const linal::double3& get_origin() const {
        return m_origin;
    }
    [[nodiscard]]
    const linal::double3& get_normal() const {
        return m_normal;
    }
};

} // namespace renderer

#endif // RENDERER_PLANE_HPP
