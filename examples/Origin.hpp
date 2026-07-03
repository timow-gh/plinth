#ifndef ORIGIN_HPP
#define ORIGIN_HPP

#include <GeoQik/GeoQik.hpp>
#include <cassert>
#include <linal/vec.hpp>

namespace geoqik::examples
{

inline void
draw_origin(double axisLength)
{
    bool initialized;
    geoqik_is_api_initialized(&initialized);
    assert(initialized);

    geoqik_add_point_with_color(0.0, 0.0, 0.0, 1.0f, 0.0f, 0.0f, 1.0f);
    geoqik_add_point_with_color(axisLength, 0.0, 0.0, 0.0f, 1.0f, 0.0f, 1.0f);
    geoqik_add_point_with_color(0.0, axisLength, 0.0, 0.0f, 0.0f, 1.0f, 1.0f);
    geoqik_add_point_with_color(0.0, 0.0, axisLength, 0.0f, 0.0f, 0.0f, 1.0f);

    geoqik_add_line_with_color(0.0, 0.0, 0.0, axisLength, 0.0, 0.0, 1.0f, 0.0f, 0.0f, 1.0f);
    geoqik_add_line_with_color(0.0, 0.0, 0.0, 0.0, axisLength, 0.0, 0.0f, 1.0f, 0.0f, 1.0f);
    geoqik_add_line_with_color(0.0, 0.0, 0.0, 0.0, 0.0, axisLength, 0.0f, 0.0f, 1.0f, 1.0f);
}

} // namespace geoqik::examples

#endif // ORIGIN_HPP
