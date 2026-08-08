#include "plinth/DashSpace.hpp"
#include "plinth/ImGuiOverlay.hpp"
#include "plinth/Renderer.hpp"
#include "plinth/UiMode.hpp"
#include "plinth/WindowSettings.hpp"
#include <array>
#include <chrono>
#include <cstdint>
#include <memory>

namespace {
constexpr std::uint32_t defaultWindowWidth = 1024;
constexpr std::uint32_t defaultWindowHeight = 768;
constexpr float standalonePointSize = 12.0F;
constexpr float standaloneLineWidth = 3.0F;
constexpr float rectangleVerticalOffset = 2.0F;

renderer::CallbackSubscription add_preset_view_callback(renderer::Renderer& renderer) {
    return renderer.add_key_callback([&renderer](renderer::Key key,
                                                 renderer::Scancode /*scancode*/,
                                                 renderer::Action action,
                                                 renderer::Mods /*mods*/) {
        if (action != renderer::Action::PRESS) {
            return;
        }
        renderer::CameraInteractor::CameraViewMode viewMode = renderer::CameraInteractor::CameraViewMode::FIX_ROTATE;
        switch (key) {
        case renderer::Key::KEY_1: renderer.go_to_preset_view(renderer::PresetView::FRONT); break;
        case renderer::Key::KEY_2: renderer.go_to_preset_view(renderer::PresetView::BACK); break;
        case renderer::Key::KEY_3: renderer.go_to_preset_view(renderer::PresetView::LEFT); break;
        case renderer::Key::KEY_4: renderer.go_to_preset_view(renderer::PresetView::RIGHT); break;
        case renderer::Key::KEY_5: renderer.go_to_preset_view(renderer::PresetView::TOP); break;
        case renderer::Key::KEY_6: renderer.go_to_preset_view(renderer::PresetView::BOTTOM); break;
        case renderer::Key::KEY_7:
            renderer.go_to_preset_view(renderer::PresetView::ISO);
            viewMode = renderer::CameraInteractor::CameraViewMode::NONE;
            break;
        default:                   return;
        }
        auto camera = renderer.get_camera().lock();
        if (camera) {
            camera->set_view_mode(viewMode);
        }
    });
}
} // namespace

int main() {
    renderer::WindowSettings settings;
    settings.title = "renderer standalone example";
    settings.width = defaultWindowWidth;
    settings.height = defaultWindowHeight;

    // Own the ImGui overlay so we can toggle its UiMode from the F1 key handler below.
    settings.overlay = renderer::OverlayKind::None;
    auto renderer = renderer::Renderer::create(settings);
    if (!renderer) {
        return 1;
    }

    auto overlay = std::make_shared<renderer::ImGuiOverlay>(renderer->window().get_native_handle());
    renderer::ImGuiOverlay& ui = *overlay;
    renderer->set_overlay(std::move(overlay));

    const std::array<float, 9> pointVertices{0.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 0.0F, 1.0F, 0.0F};
    const std::array<float, 4> yellow{1.0F, 1.0F, 0.0F, 1.0F};
    renderer->add_point_drawable(pointVertices, yellow, standalonePointSize);

    const std::array<float, 9> removablePointVertices{-1.0F, 0.0F, 0.0F, 0.0F, -1.0F, 0.0F, -1.0F, -1.0F, 1.0F};
    const std::array<float, 4> green{0.0F, 1.0F, 0.0F, 1.0F};
    const auto removablePoints = renderer->add_point_drawable(removablePointVertices, green, standalonePointSize);
    const auto deleteGreenPointsCallback =
        renderer->add_key_callback([&renderer, removablePoints](renderer::Key key,
                                                                renderer::Scancode /*scancode*/,
                                                                renderer::Action action,
                                                                renderer::Mods /*mods*/) {
            if (key == renderer::Key::KEY_DELETE && action == renderer::Action::PRESS) {
                renderer->remove_drawable(removablePoints);
            }
        });

    // A cross made of two line segments through the origin.
    const std::array<float, 12> lineVertices{-1.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 0.0F, -1.0F, 0.0F, 0.0F, 1.0F, 0.0F};
    const std::array<float, 16>
        lineColors{1.0F, 0.0F, 0.0F, 1.0F, 1.0F, 0.0F, 0.0F, 1.0F, 0.0F, 1.0F, 0.0F, 1.0F, 0.0F, 1.0F, 0.0F, 1.0F};
    const std::array<std::uint32_t, 4> lineIndices{0, 1, 2, 3};
    {
        renderer::StrokeStyle crossStyle;
        crossStyle.lineWidth = standaloneLineWidth;
        renderer->add_line_drawable(lineVertices, lineIndices, lineColors,
                                    renderer::LineType::lines(), crossStyle);
    }

    // A dashed cross above the solid one; the handle is kept so dashPhase can be animated.
    const std::array<float, 12> dashedLineVertices{
        -1.0F, 1.5F, 0.0F, 1.0F, 1.5F, 0.0F, 0.0F, 0.5F, 0.0F, 0.0F, 2.5F, 0.0F};
    const std::array<float, 4> magenta{1.0F, 0.0F, 1.0F, 1.0F};
    renderer::StrokeStyle dashedStyle;
    dashedStyle.lineWidth   = standaloneLineWidth;
    // NOLINTNEXTLINE(readability-magic-numbers)
    dashedStyle.dashPattern = {0.2F, 0.15F};
    dashedStyle.dashSpace   = renderer::DashSpace::World;
    const renderer::DrawableHandle dashedLines =
        renderer->add_line_drawable(dashedLineVertices, lineIndices, magenta,
                                    renderer::LineType::lines(), dashedStyle);

    // Add a rectangle
    const std::array<float, 4> darkBlue{0.0F, 0.0F, 0.5F, 1.0F};
    const std::array<float, 12>
        rectangleVertices{0.0F, 0.0F, 0.0F, 3.0F, 0.0F, 0.0F, 3.0F, 1.0F, 0.0F, 0.0F, 1.0F, 0.0F};
    renderer::StrokeStyle rectangleStyle;
    rectangleStyle.lineWidth = standaloneLineWidth;
    auto rectangleLines =
        renderer->add_line_drawable(rectangleVertices, darkBlue, renderer::LineType::line_loop(), rectangleStyle);

    const std::array<float, 4> lightBlue{0.0F, 0.5F, 1.0F, 1.0F};
    const std::array<std::uint32_t, 6> triangleIndices{0, 1, 2, 0, 2, 3};
    auto rectangleMesh =
        renderer->add_mesh_drawable(rectangleVertices, triangleIndices, lightBlue, renderer::MeshCullFaceMode::NONE);

    linal::hmatf transform = linal::hmatf::identity();
    transform.set_translation(linal::float3{0.0F, rectangleVerticalOffset, 0.0F});
    renderer->set_drawable_transform(rectangleLines, transform);
    renderer->set_drawable_transform(rectangleMesh, transform);

    // F1 toggles between the game-like Release control panel (the default) and the full
    // Debug panel exposing every post-processing and visualization control.
    const auto uiModeSubscription = renderer->add_key_callback([&ui](renderer::Key key,
                                                                      renderer::Scancode /*scancode*/,
                                                                      renderer::Action action,
                                                                      renderer::Mods /*mods*/) {
        if (key == renderer::Key::KEY_F1 && action == renderer::Action::PRESS) {
            ui.set_ui_mode(ui.ui_mode() == renderer::UiMode::Release ? renderer::UiMode::Debug
                                                                     : renderer::UiMode::Release);
        }
    });

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

    // Number keys 1-7 jump to named preset views (FRONT/BACK/left/right/top/bottom/iso), fitted
    // to whatever geometry currently exists in the scene.
    const auto presetViewSubscription = add_preset_view_callback(*renderer);

    const auto startTime = std::chrono::steady_clock::now();

    while (!renderer->should_close()) {
        renderer::Renderer::poll_events();
        if (renderer->is_escape_pressed()) {
            break;
        }

        // Advance the dash phase over time for a "marching ants" effect.
        const float elapsedSeconds =
            std::chrono::duration<float>(std::chrono::steady_clock::now() - startTime).count();
        renderer->set_line_dash_phase(dashedLines, elapsedSeconds * 0.5F); // NOLINT(readability-magic-numbers)

        renderer->begin_frame();
        renderer->draw();
        // This overload retains the renderer-owned Auto Zoom state across frames.
        renderer->end_frame();
    }

    return 0;
}
