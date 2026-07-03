#ifndef EXAMPLES_GRID_HPP
#define EXAMPLES_GRID_HPP

#include "GeoQik/GeoQik.hpp"
#include <cassert>
#include <vector>

namespace geoqik::examples
{

struct Line
{
    double x1, y1, z1, x2, y2, z2;
};

struct GridPoint
{
    double x, y, z;
};

struct Grid
{
    std::vector<GridPoint> points;
    std::vector<Line> lines;
};

Grid
create_grid(double size, double step)
{
    assert(size > 0.0 && step > 0.0);

    Grid grid;

    for (double i = -size; i <= size; i += step) {
        grid.lines.emplace_back(i, -size, 0.0, i, size, 0.0);
        grid.lines.emplace_back(-size, i, 0.0, size, i, 0.0);
    }
    for (double x = -size; x <= size; x += step) {
        for (double y = -size; y <= size; y += step) {
            grid.points.push_back({x, y, 0.0});
        }
    }

    return grid;
}

struct GridGeometryIds
{
    geoqik_uuid_t pointsId;
    geoqik_uuid_t lineIds;
};

GridGeometryIds
add_grid(const Grid& grid, float lineWidth = 1.0F, float pointSize = 5.0F)
{
    assert(lineWidth > 0.0F);
    assert(pointSize > 0.0F);
    assert(pointSize > lineWidth); // Ensure points are visible over lines
    bool initialized;
    geoqik_is_api_initialized(&initialized);
    assert(initialized);

    assert(!grid.lines.empty());

    std::vector<double> lineCoords;
    lineCoords.reserve(grid.lines.size() * 6);
    for (const auto& line: grid.lines) {
        lineCoords.insert(lineCoords.end(), {line.x1, line.y1, line.z1, line.x2, line.y2, line.z2});
    }

    geoqik_set_line_width(lineWidth);
    geoqik_set_line_color(0.5F, 0.5F, 0.5F, 1.0F);
    geoqik_result_t lineRes = geoqik_add_lines_opts(lineCoords.data(), lineCoords.size(), NULL);
    assert(lineRes.err == GEOQIK_SUCCESS);

    geoqik_set_point_color(0.5F, 0.5F, 0.5F, 1.0F);
    geoqik_set_point_size(pointSize);
    std::vector<double> pointCoords;
    pointCoords.reserve(grid.points.size() * 3);
    assert(!grid.points.empty());
    for (const auto& point: grid.points) {
        // geoqik_add_point(point.x, point.y, point.z);
        pointCoords.insert(pointCoords.end(), {point.x, point.y, point.z});
    }

    geoqik_result_t pointRes = geoqik_add_points_opts(pointCoords.data(), pointCoords.size(), NULL);
    assert(pointRes.err == GEOQIK_SUCCESS);

    return GridGeometryIds{pointRes.geometryId, lineRes.geometryId};
}

GridGeometryIds
add_grid(double size, double step)
{
    auto grid = create_grid(size, step);
    return add_grid(grid);
}

} // namespace geoqik::examples

#endif // EXAMPLES_GRID_HPP
