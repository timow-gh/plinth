// Showcases the instanced thick-line path: a stack of horizontal lines drawn at increasing widths,
// plus solid vs. dashed variants and animated "marching ants". Widths are reliable here because the
// lines are expanded into screen-space quads rather than relying on glLineWidth.

#include "plinth/DashSpace.hpp"
#include "plinth/Renderer.hpp"
#include "plinth/WindowSettings.hpp"
#include <array>
#include <chrono>
#include <cstdint>
#include <vector>

namespace {
constexpr std::uint32_t defaultWindowWidth = 1024;
constexpr std::uint32_t defaultWindowHeight = 768;

// Widths (in pixels) to showcase, drawn bottom-to-top with increasing thickness.
constexpr std::array<float, 6> showcaseWidths{1.0F, 2.0F, 4.0F, 8.0F, 16.0F, 24.0F};

constexpr float lineHalfLength = 3.0F;
constexpr float rowSpacing = 0.6F;

// A single horizontal segment centered on the Y axis at the given height.
std::array<float, 6> horizontal_segment(float y) {
    return {-lineHalfLength, y, 0.0F, lineHalfLength, y, 0.0F};
}
} // namespace

int main() {
    renderer::WindowSettings settings;
    settings.title = "line widths example";
    settings.width = defaultWindowWidth;
    settings.height = defaultWindowHeight;

    auto renderer = renderer::Renderer::create(settings);
    if (!renderer) {
        return 1;
    }

    const std::array<float, 4> cyan{0.2F, 0.9F, 1.0F, 1.0F};
    const std::array<float, 4> orange{1.0F, 0.6F, 0.1F, 1.0F};
    const std::array<std::uint32_t, 2> indices{0U, 1U};

    // A column of solid lines with increasing width. The lowest row is the thinnest.
    const auto rowCount = static_cast<float>(showcaseWidths.size());
    for (std::size_t i = 0; i < showcaseWidths.size(); ++i) {
        const float y = (static_cast<float>(i) - (rowCount - 1.0F) * 0.5F) * rowSpacing;
        const std::array<float, 6> verts = horizontal_segment(y);
        renderer->add_line_drawable(verts, indices, cyan, renderer::LineType::lines(), showcaseWidths[i]);
    }

    // A thick dashed line above the stack, kept as a handle so we can animate its dash phase.
    const std::array<float, 6> dashedVerts = horizontal_segment((rowCount * 0.5F + 0.5F) * rowSpacing);
    const renderer::DrawableHandle dashedLine = renderer->add_line_drawable(dashedVerts,
                                                                           indices,
                                                                           orange,
                                                                           renderer::LineType::lines(),
                                                                           16.0F,
                                                                           /*pointSize=*/0.0F,
                                                                           renderer::BufferAccessPattern::Static,
                                                                           /*dashEnabled=*/true,
                                                                           /*dashSize=*/0.4F,
                                                                           /*gapSize=*/0.25F,
                                                                           renderer::DashSpace::World);

    const auto startTime = std::chrono::steady_clock::now();

    while (!renderer->should_close()) {
        renderer::Renderer::poll_events();
        if (renderer->is_escape_pressed()) {
            break;
        }

        // Advance the dash phase over time for a "marching ants" effect.
        const float elapsedSeconds =
            std::chrono::duration<float>(std::chrono::steady_clock::now() - startTime).count();
        renderer->set_line_dash_phase(dashedLine, elapsedSeconds * 0.5F);

        renderer->begin_frame();
        renderer->draw();
        renderer->end_frame();
    }

    return 0;
}
