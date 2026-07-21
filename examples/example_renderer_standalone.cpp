#include <array>
#include <cstdint>
#include <plinth/Renderer.hpp>
#include <plinth/WindowSettings.hpp>

namespace {
constexpr std::uint32_t defaultWindowWidth = 1024;
constexpr std::uint32_t defaultWindowHeight = 768;
constexpr float standalonePointSize = 12.0F;
constexpr float standaloneLineWidth = 2.0F;
} // namespace

int main() {
    renderer::WindowSettings settings;
    settings.title = "renderer standalone example";
    settings.width = defaultWindowWidth;
    settings.height = defaultWindowHeight;

    auto renderer = renderer::Renderer::create(settings);
    if (!renderer) {
        return 1;
    }

    // One point at the origin.
    const std::array<float, 3> pointVertices{0.0F, 0.0F, 0.0F};
    const std::array<float, 4> pointColors{1.0F, 1.0F, 0.0F, 1.0F}; // yellow
    const std::array<std::uint32_t, 1> pointIndices{0};
    renderer->add_point_drawable(pointVertices, pointColors, pointIndices, standalonePointSize);

    const std::array<float, 3> removablePointVertices{0.0F, 0.0F, 1.0F};
    const auto removablePoint =
        renderer->add_point_drawable(removablePointVertices, pointColors, pointIndices, standalonePointSize);
    renderer->remove_drawable(removablePoint);

    // A cross made of two line segments through the origin.
    const std::array<float, 12> lineVertices{-1.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 0.0F, -1.0F, 0.0F, 0.0F, 1.0F, 0.0F};
    const std::array<float, 16>
        lineColors{1.0F, 0.0F, 0.0F, 1.0F, 1.0F, 0.0F, 0.0F, 1.0F, 0.0F, 1.0F, 0.0F, 1.0F, 0.0F, 1.0F, 0.0F, 1.0F};
    const std::array<std::uint32_t, 4> lineIndices{0, 1, 2, 3};
    renderer->add_line_drawable(lineVertices, lineIndices, lineColors, renderer::LineType::lines(), standaloneLineWidth);

    // Tab toggles between orbit navigation and fly (WASD+QE) navigation, proving the new
    // CameraInteractor::NavigationStyle switch works end-to-end.
    const auto navigationStyleSubscription = renderer->add_key_callback([&renderer](renderer::Key key,
                                                                                     renderer::Scancode /*scancode*/,
                                                                                     renderer::Action action,
                                                                                     renderer::Mods /*mods*/) {
        if (key == renderer::Key::KEY_TAB && action == renderer::Action::PRESS) {
            auto camera = renderer->get_camera().lock();
            if (camera) {
                camera->set_navigation_style(camera->get_navigation_style() ==
                                                     renderer::CameraInteractor::NavigationStyle::ORBIT
                                                 ? renderer::CameraInteractor::NavigationStyle::FLY
                                                 : renderer::CameraInteractor::NavigationStyle::ORBIT);
            }
        }
    });

    // Number keys 1-7 jump to named preset views (front/back/left/right/top/bottom/iso), fitted
    // to whatever geometry currently exists in the scene. Routed through Renderer::go_to_preset_view
    // (not CameraInteractor::go_to_preset_view directly) so the jump actually frames current
    // geometry instead of just rotating around whatever pivot/distance the camera happened to have.
    const auto presetViewSubscription = renderer->add_key_callback([&renderer](renderer::Key key,
                                                                                renderer::Scancode /*scancode*/,
                                                                                renderer::Action action,
                                                                                renderer::Mods /*mods*/) {
        if (action != renderer::Action::PRESS) {
            return;
        }
        switch (key) {
        case renderer::Key::KEY_1: renderer->go_to_preset_view(renderer::PresetView::FRONT); break;
        case renderer::Key::KEY_2: renderer->go_to_preset_view(renderer::PresetView::BACK); break;
        case renderer::Key::KEY_3: renderer->go_to_preset_view(renderer::PresetView::LEFT); break;
        case renderer::Key::KEY_4: renderer->go_to_preset_view(renderer::PresetView::RIGHT); break;
        case renderer::Key::KEY_5: renderer->go_to_preset_view(renderer::PresetView::TOP); break;
        case renderer::Key::KEY_6: renderer->go_to_preset_view(renderer::PresetView::BOTTOM); break;
        case renderer::Key::KEY_7: renderer->go_to_preset_view(renderer::PresetView::ISO); break;
        default:                   break;
        }
    });

    while (!renderer->should_close()) {
        renderer::Renderer::poll_events();
        if (renderer->is_escape_pressed()) {
            break;
        }
        renderer->begin_frame();
        renderer->draw();
        // This overload retains the renderer-owned Auto Zoom state across frames.
        renderer->end_frame();
    }

    return 0;
}
