#include <array>
#include <cstddef>
#include <cstdint>
#include <plinth/Renderer.hpp>
#include <plinth/Texture.hpp>
#include <plinth/WindowSettings.hpp>

int main() {
    renderer::WindowSettings settings;
    settings.title = "textured mesh example";
    auto renderer = renderer::Renderer::create(settings);
    if (!renderer) {
        return 1;
    }
    constexpr std::uint32_t textureSize = 64U;
    constexpr std::uint32_t checkerSize = 16U;
    std::array<std::uint8_t, textureSize * textureSize * 4U> checkerboard{};
    for (std::uint32_t y = 0; y < textureSize; ++y) {
        for (std::uint32_t x = 0; x < textureSize; ++x) {
            const auto shade = static_cast<std::uint8_t>(((x / checkerSize + y / checkerSize) % 2U == 0U) ? 255U : 32U);
            const auto pixel = static_cast<std::size_t>((y * textureSize + x) * 4U);
            checkerboard[pixel] = shade;
            checkerboard[pixel + 1U] = shade;
            checkerboard[pixel + 2U] = shade;
            checkerboard[pixel + 3U] = 255U;
        }
    }
    const auto texture = renderer->create_texture_2d(
        {textureSize, textureSize, checkerboard, renderer::TextureColorSpace::srgb, renderer::TextureFilter::nearest});
    const std::array<float, 12> vertices{-1.0F, -1.0F, 0.0F, 1.0F, -1.0F, 0.0F, 1.0F, 1.0F, 0.0F, -1.0F, 1.0F, 0.0F};
    const std::array<float, 12> normals{0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 1.0F};
    const std::array<float, 8> uvs{0.0F, 0.0F, 1.0F, 0.0F, 1.0F, 1.0F, 0.0F, 1.0F};
    const std::array<float, 16>
        colors{1.0F, 1.0F, 1.0F, 1.0F, 1.0F, 1.0F, 1.0F, 1.0F, 1.0F, 1.0F, 1.0F, 1.0F, 1.0F, 1.0F, 1.0F, 1.0F};
    const std::array<std::uint32_t, 6> indices{0, 1, 2, 0, 2, 3};
    renderer->add_textured_mesh_drawable(vertices, normals, uvs, colors, indices, texture);
    while (!renderer->should_close()) {
        renderer::Renderer::poll_events();
        renderer->begin_frame();
        renderer->draw();
        renderer->end_frame();
    }
}
