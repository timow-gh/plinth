#include <GeoQik/GeoQik.hpp>

int main() {
    geoqik_init();

    geoqik_set_point_size(5.0f);
    geoqik_set_line_width(2.0f);

    geoqik_draw();

    geoqik_add_point_with_color(0.0, 0.0, 0.0, 1.0f, 0.0f, 0.0f, 1.0f);
    geoqik_add_point_with_color(1.0, 0.0, 0.0, 0.0f, 1.0f, 0.0f, 1.0f);
    geoqik_add_point_with_color(0.0, 1.0, 0.0, 0.0f, 0.0f, 1.0f, 1.0f);
    geoqik_add_point_with_color(0.0, 0.0, 1.0, 0.0f, 0.0f, 0.0f, 1.0f);

    geoqik_add_line_with_color(0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 1.0f, 0.0f, 0.0f, 1.0f);
    geoqik_add_line_with_color(0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0f, 1.0f, 0.0f, 1.0f);
    geoqik_add_line_with_color(0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0f, 0.0f, 1.0f, 1.0f);

    geoqik_wait_for_exit_and_cleanup();

    return 0;
}
