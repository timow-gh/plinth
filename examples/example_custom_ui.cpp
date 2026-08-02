// Demonstrates rolling your own UI on top of the plinth Renderer.
//
// The Renderer is created with OverlayKind::None, so no ImGui context or backends are
// created. The application:
//   * reserves screen space for its own UI by setting the scene viewport explicitly, and
//   * handles input itself through the renderer's callbacks (which, with no overlay, are
//     forwarded unfiltered) and/or the raw GLFW window handle.
//
// A real application would draw its panel here with whatever toolkit it prefers (a custom
// IOverlay reusing the renderer's ImGui, Qt, a web view, etc.). This example keeps the UI
// area empty to stay dependency-free and focuses on the wiring.

#include "plinth/Renderer.hpp"
#include "plinth/WindowSettings.hpp"
#include <array>
#include <cstdint>

namespace {
constexpr std::uint32_t defaultWindowWidth = 1024;
constexpr std::uint32_t defaultWindowHeight = 768;
constexpr float standalonePointSize = 12.0F;
constexpr float standaloneLineWidth = 3.0F;
constexpr double sidePanelLogicalWidth = 320.0;

// Reserve a fixed-width strip on the left of the window for the application's own UI and
// give the remaining area to the 3D scene, in logical (window) coordinates.
renderer::LogicalViewportRect scene_rect(std::pair<int, int> windowSize) {
    const double windowWidth = static_cast<double>(windowSize.first);
    const double windowHeight = static_cast<double>(windowSize.second);
    const double sceneX = sidePanelLogicalWidth < windowWidth ? sidePanelLogicalWidth : 0.0;
    return renderer::LogicalViewportRect{sceneX, 0.0, windowWidth - sceneX, windowHeight};
}
} // namespace

int main() {
    renderer::WindowSettings settings;
    settings.title = "custom UI example";
    settings.width = defaultWindowWidth;
    settings.height = defaultWindowHeight;
    // Do not create the built-in ImGui overlay: the application owns the UI.
    settings.overlay = renderer::OverlayKind::None;

    auto renderer = renderer::Renderer::create(settings);
    if (!renderer) {
        return 1;
    }

    // Give the 3D scene everything except the left strip we reserve for our own panel.
    renderer->set_scene_viewport(scene_rect(renderer->window().get_window_size()));

    const std::array<float, 9> pointVertices{0.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 0.0F, 1.0F, 0.0F};
    const std::array<float, 4> yellow{1.0F, 1.0F, 0.0F, 1.0F};
    renderer->add_point_drawable(pointVertices, yellow, standalonePointSize);

    const std::array<float, 12> lineVertices{-1.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 0.0F, -1.0F, 0.0F, 0.0F, 1.0F, 0.0F};
    const std::array<float, 4> red{1.0F, 0.0F, 0.0F, 1.0F};
    renderer->add_line_drawable(lineVertices, red, renderer::LineType::lines(), standaloneLineWidth);

    // With no overlay, mouse/key callbacks are forwarded unfiltered. Applications can also
    // read the raw window via renderer->window().get_native_handle() to install their own
    // GLFW callbacks for a bespoke toolkit.
    const auto clickSubscription = renderer->add_mouse_button_callback(
        [&renderer](int button, renderer::Action action, renderer::Mods /*mods*/) {
            if (button == 0 && action == renderer::Action::PRESS) {
                const auto ray = renderer->compute_pick_ray(0.0, 0.0);
                (void)ray;
            }
        });

    while (!renderer->should_close()) {
        renderer::Renderer::poll_events();
        if (renderer->is_escape_pressed()) {
            break;
        }
        renderer->set_scene_viewport(scene_rect(renderer->window().get_window_size()));

        renderer->begin_frame();
        renderer->draw();
        renderer->end_frame();
    }

    return 0;
}
