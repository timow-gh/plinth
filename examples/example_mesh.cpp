#include "plinth/ImGuiOverlay.hpp"
#include "plinth/LightingConfig.hpp"
#include "plinth/Renderer.hpp"
#include "plinth/Texture.hpp"
#include "plinth/WindowSettings.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <numeric>

namespace {
constexpr std::size_t kVertexCount = 4;
} // namespace

int main() {
    renderer::WindowSettings settings;
    settings.title = "Mesh Example";
    // Own the ImGui overlay so we can add our own panels to it. Create the renderer without
    // the built-in overlay, then inject ours.
    settings.overlay = renderer::OverlayKind::None;
    auto renderer = renderer::Renderer::create(settings);
    if (!renderer) {
        return 1;
    }

    auto overlay = std::make_shared<renderer::ImGuiOverlay>(renderer->window().get_native_handle());
    renderer::ImGuiOverlay& ui = *overlay;
    renderer->set_overlay(std::move(overlay));

    const std::array<float, 4> colorGrey{0.5F, 0.5F, 0.5F, 1.0F};
    const std::array<float, kVertexCount * 3>
        vertices{-1.0F, -1.0F, 0.0F, 1.0F, -1.0F, 0.0F, 1.0F, 1.0F, 0.0F, -1.0F, 1.0F, 0.0F};
    const float vertexRadius = 0.05F;
    const std::array<float, kVertexCount> vertexRadii{vertexRadius, vertexRadius, vertexRadius, vertexRadius};
    const std::array<std::uint32_t, 6> indices{0, 1, 2, 0, 2, 3};
    renderer->add_mesh_drawable(vertices, indices, colorGrey);
    renderer->add_sphere_point_drawable(vertices, vertexRadii, colorGrey);

    renderer::LightingConfig lighting;

    while (!renderer->should_close()) {
        renderer::Renderer::poll_events();
        ui.add_lighting_controls(lighting);
        renderer->begin_frame();
        renderer->draw(lighting);
        renderer->end_frame();
    }
}
