#include "example_load_mesh_file.hpp"

int main() {
    const std::filesystem::path meshPath =
        std::filesystem::path{PLINTH_EXAMPLES_DIR} / "indoor_plant" / "indoor plant_02.obj";
    return mesh_example::run("plant mesh example", meshPath);
}
