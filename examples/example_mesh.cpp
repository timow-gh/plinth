#include "plinth/LightingConfig.hpp"
#include "plinth/Renderer.hpp"
#include "plinth/Texture.hpp"
#include "plinth/WindowSettings.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <numeric>

namespace {
constexpr std::size_t kVertexCount = 4;
} // namespace

int main() {
    renderer::WindowSettings settings;
    settings.title = "textured mesh example";
    auto renderer = renderer::Renderer::create(settings);
    if (!renderer) {
        return 1;
    }

    const std::array<float, 4> colorGrey{0.5F, 0.5F, 0.5F, 1.0F};
    const std::array<float, 4> colorBlack{0.0F, 0.0F, 0.0F, 1.0F};
    const std::array<float, kVertexCount * 3> vertices{-1.0F, -1.0F, 0.0F, 1.0F, -1.0F, 0.0F, 1.0F, 1.0F, 0.0F, -1.0F, 1.0F, 0.0F};
    const std::array<std::uint32_t, 6> indices{0, 1, 2, 0, 2, 3};
    renderer->add_mesh_drawable(vertices, indices, colorGrey);
    const float pointSize = 5.0F;
    renderer->add_point_drawable(vertices, colorBlack, pointSize);

    renderer::LightingConfig lighting;

    while (!renderer->should_close()) {
        renderer::Renderer::poll_events();
        // Controls are consumed and cleared every frame, so re-register the
        // lighting panel each iteration. It edits `lighting` in place.
        renderer->imgui().add_lighting_controls(lighting);
        renderer->begin_frame();
        renderer->draw(lighting);
        renderer->end_frame();
    }
}
