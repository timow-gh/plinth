#include "example_load_mesh_file.hpp"

int main() {
    const std::filesystem::path meshPath =
        std::filesystem::path{PLINTH_EXAMPLES_DIR} / "low_poly_tree" / "Lowpoly_tree_sample.obj";
    return mesh_example::run("low-poly tree mesh example", meshPath);
}
