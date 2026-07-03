#include <GeoQikClient/GeoQikClient.hpp>

int main() {
    geoqik_client_clear_last_error();

    geoqik_uuid_t id{};
    geoqik_result_t result{GEOQIK_SUCCESS, id};

    return result.err == GEOQIK_SUCCESS ? 0 : 1;
}
