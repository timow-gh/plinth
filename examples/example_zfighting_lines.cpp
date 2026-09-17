// Visual verification for the lines-on-paper z-fighting fix.
//
// Scenario: a flat "paper" quad with crease lines lying exactly on its surface (the
// coplanar lines-on-faces case), plus two crossing line strips at different depthLayers
// (the deterministic overlap-ordering case). Orbit with the mouse; grazing angles are
// where z-fighting is worst.
//
// Controls:
//   B     toggle the z-fighting fix on/off for the crease lines (see fixEnabled below)
//   1-7   preset views (front/back/left/right/top/bottom/iso)
//   Esc   quit
//
// With the fix ON the creases stay crisp on the paper at every angle. Toggle it OFF and the
// creases z-fight the paper. The bias is opt-in: depthLayer == 0 applies NO nudge (the true
// no-bias baseline that actually z-fights), and a positive depthLayer pushes the creases a
// per-layer step toward the camera (LineDrawable.cpp kDepthLayerStep), clearly in front.

#include "example_preset_views.hpp"

#include "plinth/Renderer.hpp"
#include "plinth/StrokeStyle.hpp"
#include "plinth/WindowSettings.hpp"

#include <array>
#include <cstdint>
#include <vector>

namespace {
constexpr std::uint32_t defaultWindowWidth = 1024;
constexpr std::uint32_t defaultWindowHeight = 768;
constexpr float creaseLineWidth = 3.0F;
constexpr float overlapLineWidth = 6.0F;

// "off" == 0 is the true no-bias baseline (creases z-fight the paper); "on" == a positive layer
// nudges the creases a per-layer step toward the camera so they sit clearly in front.
constexpr std::int32_t fixOffDepthLayer = 0;
constexpr std::int32_t fixOnDepthLayer = 4;

using example::add_preset_view_callback;
} // namespace

int main() {
    renderer::WindowSettings settings;
    settings.title = "z-fighting lines-on-paper example (B: toggle fix, 1-7: views, Esc: quit)";
    settings.width = defaultWindowWidth;
    settings.height = defaultWindowHeight;
    settings.overlay = renderer::OverlayKind::None;

    auto renderer = renderer::Renderer::create(settings);
    if (!renderer) {
        return 1;
    }

    // --- Paper: a flat quad in the z = 0 plane. ---
    const std::array<float, 12> paperVertices{
        -2.0F, -2.0F, 0.0F, 2.0F, -2.0F, 0.0F, 2.0F, 2.0F, 0.0F, -2.0F, 2.0F, 0.0F};
    const std::array<std::uint32_t, 6> paperIndices{0, 1, 2, 0, 2, 3};
    const std::array<float, 4> paperColor{0.90F, 0.87F, 0.78F, 1.0F}; // warm paper
    renderer->add_mesh_drawable(paperVertices, paperIndices, paperColor, renderer::MeshCullFaceMode::NONE);

    // --- Creases: line strips lying exactly on the paper plane (z = 0). ---
    // These are the coplanar lines-on-faces case; the toggle mutates their depthLayer.
    const std::array<float, 4> creaseColor{0.15F, 0.15F, 0.15F, 1.0F};

    // A few diagonal / mountain-valley style creases spanning the sheet.
    const std::array<float, 12> creaseA{-2.0F, -2.0F, 0.0F, -0.5F, 0.5F, 0.0F, 0.5F, -0.5F, 0.0F, 2.0F, 2.0F, 0.0F};
    const std::array<float, 12> creaseB{-2.0F, 2.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 2.0F, -2.0F, 0.0F};
    const std::array<float, 12> creaseC{-2.0F, 0.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F, -1.0F, 0.0F, 2.0F, 0.0F, 0.0F};

    renderer::StrokeStyle creaseStyle;
    creaseStyle.lineWidth = creaseLineWidth;
    creaseStyle.depthLayer = fixOnDepthLayer; // start with the fix ON

    std::vector<renderer::DrawableHandle> creaseHandles;
    for (const auto* verts: {&creaseA, &creaseB, &creaseC}) {
        creaseHandles.push_back(
            renderer->add_line_drawable(*verts, creaseColor, renderer::LineType::line_strip(), creaseStyle));
    }

    // --- Overlap set: two crossing strips at the same plane, different depthLayers. ---
    // The higher layer must stay consistently on top with no flicker while orbiting.
    const std::array<float, 6> overlapRed{-1.5F, -1.5F, 0.0F, 1.5F, 1.5F, 0.0F};
    const std::array<float, 6> overlapCyan{-1.5F, 1.5F, 0.0F, 1.5F, -1.5F, 0.0F};
    const std::array<float, 4> red{0.85F, 0.15F, 0.15F, 1.0F};
    const std::array<float, 4> cyan{0.10F, 0.65F, 0.75F, 1.0F};
    const std::array<std::uint32_t, 2> overlapIndices{0, 1};

    renderer::StrokeStyle overlapLow;
    overlapLow.lineWidth = overlapLineWidth;
    overlapLow.depthLayer = 2; // lower priority: still above the paper
    renderer->add_line_drawable(overlapRed, overlapIndices, red, renderer::LineType::line_strip(), overlapLow);

    renderer::StrokeStyle overlapHigh;
    overlapHigh.lineWidth = overlapLineWidth;
    overlapHigh.depthLayer = 6; // higher priority: sits on top of the red one
    renderer->add_line_drawable(overlapCyan, overlapIndices, cyan, renderer::LineType::line_strip(), overlapHigh);

    // --- B toggles the fix on the crease lines via the public stroke-style path. ---
    bool fixEnabled = true;
    const auto toggleFixSubscription = renderer->add_key_callback(
        [&renderer, &creaseHandles, &creaseStyle, &fixEnabled](renderer::Key key,
                                                               renderer::Scancode /*scancode*/,
                                                               renderer::Action action,
                                                               renderer::Mods /*mods*/) {
            if (key != renderer::Key::KEY_B || action != renderer::Action::PRESS) {
                return;
            }
            fixEnabled = !fixEnabled;
            creaseStyle.depthLayer = fixEnabled ? fixOnDepthLayer : fixOffDepthLayer;
            for (const renderer::DrawableHandle& handle: creaseHandles) {
                renderer->set_line_stroke_style(handle, creaseStyle);
            }
        });

    const auto presetViewSubscription = add_preset_view_callback(*renderer);

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
