#include "plinth/Renderer.hpp"
#include "plinth/WindowSettings.hpp"
#include "plinth/loader/MeshLoader.hpp"

#include <array>
#include <cstdint>
#include <iostream>
#include <string>
#include <string_view>

namespace {
constexpr std::uint32_t defaultWindowWidth = 1024;
constexpr std::uint32_t defaultWindowHeight = 768;

constexpr std::string_view cubeObj =
    "v -0.5 -0.5 -0.5\n"
    "v  0.5 -0.5 -0.5\n"
    "v  0.5  0.5 -0.5\n"
    "v -0.5  0.5 -0.5\n"
    "v -0.5 -0.5  0.5\n"
    "v  0.5 -0.5  0.5\n"
    "v  0.5  0.5  0.5\n"
    "v -0.5  0.5  0.5\n"
    "f 4 3 2 1\n"
    "f 6 7 8 5\n"
    "f 2 6 5 1\n"
    "f 8 7 3 4\n"
    "f 5 8 4 1\n"
    "f 3 7 6 2\n";
} // namespace

int main() {
    renderer::WindowSettings settings;
    settings.title = "in-memory mesh example";
    settings.width = defaultWindowWidth;
    settings.height = defaultWindowHeight;

    auto renderer = renderer::Renderer::create(settings);
    if (!renderer) {
        std::cerr << "Error: renderer initialization failed.\n";
        return 1;
    }

    const renderer::MeshLoadOptions options{.shading = renderer::ShadingMode::Flat};
    auto mesh = renderer::load_mesh(".obj", std::string{cubeObj}, options);
    if (!mesh) {
        std::cerr << "Error: failed to load the in-memory cube.\n";
        return 2;
    }

    const std::array<float, 4> lightBlue{0.2F, 0.4F, 0.8F, 1.0F};
    if (!renderer->add_mesh_drawable(*mesh, lightBlue).is_valid()) {
        std::cerr << "Error: failed to create the cube drawable.\n";
        return 3;
    }

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
