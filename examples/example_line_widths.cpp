// Showcases instanced thick-line rendering: widths, caps, joins, dash patterns, and animated phase.
// Lines are expanded into screen-space quads so caps/joins and widths are driver-independent.

#include "plinth/Renderer.hpp"
#include "plinth/StrokeStyle.hpp"
#include "plinth/WindowSettings.hpp"

#include <array>
#include <chrono>
#include <cstdint>
#include <vector>

// Example code uses literal values for visual layout — suppressing magic-number checks.
// NOLINTBEGIN(readability-magic-numbers)

namespace {

constexpr std::uint32_t kWindowWidth = 1280;
constexpr std::uint32_t kWindowHeight = 900;

constexpr float kHalfLen = 3.5F;     // half-length of each demonstration line
constexpr float kRowSpacing = 0.55F; // vertical spacing between rows

// Flat xyz triplet for a horizontal segment at height y.
std::array<float, 6> hline(float y) {
    return {-kHalfLen, y, 0.0F, kHalfLen, y, 0.0F};
}

// Five-vertex zigzag strip (for join demonstrations): starts at left, zigzags to right.
std::vector<float> zigzag(float y, float amplitude = 0.18F) {
    return {
        -kHalfLen,
        y,
        0.0F,
        -kHalfLen * 0.5F,
        y + amplitude,
        0.0F,
        0.0F,
        y - amplitude,
        0.0F,
        kHalfLen * 0.5F,
        y + amplitude,
        0.0F,
        kHalfLen,
        y,
        0.0F,
    };
}

// A closed square loop for line_loop demonstration.
std::vector<float> square_loop(float cx, float cy, float side) {
    const float h = side * 0.5F;
    return {
        cx - h,
        cy - h,
        0.0F,
        cx + h,
        cy - h,
        0.0F,
        cx + h,
        cy + h,
        0.0F,
        cx - h,
        cy + h,
        0.0F,
    };
}

std::array<float, 4> rgba(float r, float g, float b, float a = 1.0F) {
    return {r, g, b, a};
}

} // namespace

int main() {
    renderer::WindowSettings settings;
    settings.title = "line widths, caps, joins, and dashing";
    settings.width = kWindowWidth;
    settings.height = kWindowHeight;

    auto renderer = renderer::Renderer::create(settings);
    if (!renderer) {
        return 1;
    }

    // Color palette
    const auto cyan = rgba(0.2F, 0.9F, 1.0F);
    const auto orange = rgba(1.0F, 0.6F, 0.1F);
    const auto green = rgba(0.3F, 1.0F, 0.4F);
    const auto pink = rgba(1.0F, 0.4F, 0.7F);
    const auto yellow = rgba(1.0F, 0.9F, 0.2F);
    const auto violet = rgba(0.7F, 0.4F, 1.0F);
    const auto white = rgba(0.9F, 0.9F, 0.9F);
    const auto teal = rgba(0.2F, 0.8F, 0.7F);
    const auto red = rgba(1.0F, 0.3F, 0.3F);

    const std::array<std::uint32_t, 2> seg2{0U, 1U};
    const std::array<std::uint32_t, 5> strip5{0U, 1U, 2U, 3U, 4U};
    const std::array<std::uint32_t, 4> loop4{0U, 1U, 2U, 3U};

    float row = 0.0F;

    // ── Row 0: Varying widths, butt cap (baseline) ──────────────────────────────
    constexpr std::array<float, 5> widths{1.0F, 2.0F, 4.0F, 8.0F, 16.0F};
    for (std::size_t i = 0; i < widths.size(); ++i) {
        const float y = row - (static_cast<float>(i) * 0.28F);
        const auto verts = hline(y);
        renderer::StrokeStyle s;
        s.lineWidth = widths[i];
        renderer->add_line_drawable(verts, seg2, cyan, renderer::LineType::lines(), s);
    }
    row -= static_cast<float>(widths.size()) * 0.28F + kRowSpacing;

    // ── Row 1: Round cap on isolated segments ────────────────────────────────────
    {
        const auto verts = hline(row);
        renderer::StrokeStyle s;
        s.lineWidth = 12.0F;
        s.cap = renderer::LineCap::Round;
        renderer->add_line_drawable(verts, seg2, green, renderer::LineType::lines(), s);
        row -= kRowSpacing;
    }

    // ── Row 2: Square cap on isolated segments ───────────────────────────────────
    {
        const auto verts = hline(row);
        renderer::StrokeStyle s;
        s.lineWidth = 12.0F;
        s.cap = renderer::LineCap::Square;
        renderer->add_line_drawable(verts, seg2, yellow, renderer::LineType::lines(), s);
        row -= kRowSpacing;
    }

    // ── Row 3: Round join on zigzag strip ────────────────────────────────────────
    {
        const auto verts = zigzag(row);
        renderer::StrokeStyle s;
        s.lineWidth = 10.0F;
        s.join = renderer::LineJoin::Round;
        s.cap = renderer::LineCap::Butt;
        renderer->add_line_drawable(verts, strip5, orange, renderer::LineType::line_strip(), s);
        row -= kRowSpacing;
    }

    // ── Row 4: Round cap + round join ────────────────────────────────────────────
    {
        const auto verts = zigzag(row);
        renderer::StrokeStyle s;
        s.lineWidth = 10.0F;
        s.cap = renderer::LineCap::Round;
        s.join = renderer::LineJoin::Round;
        renderer->add_line_drawable(verts, strip5, pink, renderer::LineType::line_strip(), s);
        row -= kRowSpacing;
    }

    // ── Row 5: Simple dash {10, 5}, butt cap, world space ────────────────────────
    {
        const auto verts = hline(row);
        renderer::StrokeStyle s;
        s.lineWidth = 8.0F;
        s.dashPattern = {0.4F, 0.2F};
        s.dashSpace = renderer::DashSpace::World;
        renderer->add_line_drawable(verts, seg2, white, renderer::LineType::lines(), s);
        row -= kRowSpacing;
    }

    // ── Row 6: Simple dash, round cap ────────────────────────────────────────────
    {
        const auto verts = hline(row);
        renderer::StrokeStyle s;
        s.lineWidth = 8.0F;
        s.cap = renderer::LineCap::Round;
        s.dashPattern = {0.3F, 0.25F};
        s.dashSpace = renderer::DashSpace::World;
        renderer->add_line_drawable(verts, seg2, teal, renderer::LineType::lines(), s);
        row -= kRowSpacing;
    }

    // ── Row 7: Dash–dot pattern {10, 3, 2, 3}, butt cap ─────────────────────────
    {
        const auto verts = hline(row);
        renderer::StrokeStyle s;
        s.lineWidth = 6.0F;
        s.dashPattern = {0.35F, 0.1F, 0.07F, 0.1F};
        s.dashSpace = renderer::DashSpace::World;
        renderer->add_line_drawable(verts, seg2, violet, renderer::LineType::lines(), s);
        row -= kRowSpacing;
    }

    // ── Row 8: Dash–dot pattern, round cap ───────────────────────────────────────
    {
        const auto verts = hline(row);
        renderer::StrokeStyle s;
        s.lineWidth = 6.0F;
        s.cap = renderer::LineCap::Round;
        s.dashPattern = {0.35F, 0.1F, 0.07F, 0.1F};
        s.dashSpace = renderer::DashSpace::World;
        renderer->add_line_drawable(verts, seg2, red, renderer::LineType::lines(), s);
        row -= kRowSpacing;
    }

    // ── Row 9: Simple dash, screen space ─────────────────────────────────────────
    {
        const auto verts = hline(row);
        renderer::StrokeStyle s;
        s.lineWidth = 8.0F;
        s.dashPattern = {20.0F, 10.0F};
        s.dashSpace = renderer::DashSpace::Screen;
        renderer->add_line_drawable(verts, seg2, yellow, renderer::LineType::lines(), s);
        row -= kRowSpacing;
    }

    // ── Row 10: Animated marching ants (handle retained) ─────────────────────────
    renderer::DrawableHandle antLine;
    {
        const auto verts = hline(row);
        renderer::StrokeStyle s;
        s.lineWidth = 8.0F;
        s.cap = renderer::LineCap::Round;
        s.dashPattern = {24.0F, 12.0F};
        s.dashSpace = renderer::DashSpace::Screen;
        antLine = renderer->add_line_drawable(verts, seg2, orange, renderer::LineType::lines(), s);
        row -= kRowSpacing;
    }

    // ── Row 11: Closed square loop, round join ────────────────────────────────────
    {
        const auto verts = square_loop(0.0F, row - 0.2F, 0.7F);
        renderer::StrokeStyle s;
        s.lineWidth = 8.0F;
        s.cap = renderer::LineCap::Round;
        s.join = renderer::LineJoin::Round;
        renderer->add_line_drawable(verts, loop4, green, renderer::LineType::line_loop(), s);
    }

    const auto startTime = std::chrono::steady_clock::now();

    while (!renderer->should_close()) {
        renderer::Renderer::poll_events();
        if (renderer->is_escape_pressed()) {
            break;
        }

        const float elapsed = std::chrono::duration<float>(std::chrono::steady_clock::now() - startTime).count();
        renderer->set_line_dash_phase(antLine, elapsed * 2.5F); // phase in pattern-periods (0.5/sec)

        renderer->begin_frame();
        renderer->draw();
        renderer->end_frame();
    }

    return 0;
}

// NOLINTEND(readability-magic-numbers)
