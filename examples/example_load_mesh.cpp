#include "plinth/Renderer.hpp"
#include "plinth/WindowSettings.hpp"
#include "plinth/loader/MeshLoader.hpp"
#include <array>
#include <cstdint>
#include <string>
#include <string_view>

namespace {
constexpr std::uint32_t defaultWindowWidth = 1024;
constexpr std::uint32_t defaultWindowHeight = 768;

// A unit cube in OBJ
constexpr std::string_view fallbackCubeObj =
    "v -0.5 -0.5 -0.5\n"
    "v  0.5 -0.5 -0.5\n"
    "v  0.5  0.5 -0.5\n"
    "v -0.5  0.5 -0.5\n"
    "v -0.5 -0.5  0.5\n"
    "v  0.5 -0.5  0.5\n"
    "v  0.5  0.5  0.5\n"
    "v -0.5  0.5  0.5\n"
    // Faces are wound counter-clockwise as seen from outside so they are
    // front-facing under the renderer's default GL_CCW / cull-back convention.
    "f 4 3 2 1\n"  // back
    "f 6 7 8 5\n"  // front
    "f 2 6 5 1\n"  // bottom
    "f 8 7 3 4\n"  // top
    "f 5 8 4 1\n"  // left
    "f 3 7 6 2\n"; // right
} // namespace

int main() {
    renderer::WindowSettings settings;
    settings.title = "load mesh example";
    settings.width = defaultWindowWidth;
    settings.height = defaultWindowHeight;

    auto renderer = renderer::Renderer::create(settings);
    if (!renderer) {
        return 1;
    }

    // OBJ files are Y-up by convention; load_mesh rotates them into the
    // renderer's Z-up space automatically (override with MeshLoadOptions::upAxis).
    const renderer::MeshLoadOptions options{.shading = renderer::ShadingMode::Flat};
    // auto mesh = renderer::load_mesh(
    //     std::filesystem::path("C:\\Users\\testUser\\dev\\repos\\plinth\\examples\\indoor plant_02.obj"),
    //     options);
    auto mesh = renderer::load_mesh(
        std::filesystem::path("C:\\Users\\testUser\\dev\\repos\\plinth\\examples\\Intergalactic_Spaceship.stl"),
        options);
    // auto mesh = renderer::load_mesh(".obj", std::string{fallbackCubeObj}, options);

    if (!mesh) {
        return 2;
    }

    const std::array<float, 4> lightBlue{0.2F, 0.4F, 0.8F, 1.0F};
    renderer->add_mesh_drawable(*mesh, lightBlue);

    while (!renderer->should_close()) {
        renderer::Renderer::poll_events();
        if (renderer->is_escape_pressed()) {
            break;
        }
        renderer->begin_frame();
        renderer->draw();
        renderer->end_frame();
    }

    return 0;
}
