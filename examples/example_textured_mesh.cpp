#include <array>
#include <cstdint>
#include <plinth/Renderer.hpp>
#include <plinth/Texture.hpp>
#include <plinth/WindowSettings.hpp>

int main() {
    renderer::WindowSettings settings;
    settings.title = "textured mesh example";
    auto renderer = renderer::Renderer::create(settings);
    if (!renderer) return 1;
    const std::array<std::uint8_t, 16> checkerboard{255, 255, 255, 255, 32, 32, 32, 255, 32, 32, 32, 255, 255, 255, 255, 255};
    const auto texture = renderer->create_texture_2d({2, 2, checkerboard});
    const std::array<float, 12> vertices{-1.0F, -1.0F, 0.0F, 1.0F, -1.0F, 0.0F, 1.0F, 1.0F, 0.0F, -1.0F, 1.0F, 0.0F};
    const std::array<float, 12> normals{0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 1.0F};
    const std::array<float, 8> uvs{0.0F, 0.0F, 1.0F, 0.0F, 1.0F, 1.0F, 0.0F, 1.0F};
    const std::array<float, 16> colors{1.0F, 1.0F, 1.0F, 1.0F, 1.0F, 1.0F, 1.0F, 1.0F, 1.0F, 1.0F, 1.0F, 1.0F, 1.0F, 1.0F, 1.0F, 1.0F};
    const std::array<std::uint32_t, 6> indices{0, 1, 2, 0, 2, 3};
    renderer->add_textured_mesh_drawable(vertices, normals, uvs, colors, indices, texture);
    while (!renderer->should_close()) {
        renderer::Renderer::poll_events();
        renderer->begin_frame();
        renderer->draw();
        renderer->end_frame();
    }
}
