#include <OpenGL/LineType.hpp>
#include <plinth/Renderer.hpp>
#include <plinth/WindowSettings.hpp>
#include <array>
#include <cstdint>

namespace
{
constexpr std::uint32_t defaultWindowWidth = 1024;
constexpr std::uint32_t defaultWindowHeight = 768;
constexpr float standalonePointSize = 12.0F;
constexpr float standaloneLineWidth = 2.0F;
} // namespace

int
main()
{
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
    renderer->add_line_drawable(lineVertices, lineIndices, lineColors, opengl::LineType::lines(), standaloneLineWidth);

    while (!renderer->should_close()) {
        renderer::Renderer::poll_events();
        if (renderer->is_escape_pressed()) {
            break;
        }
        renderer->begin_frame();
        renderer->draw();
    }

    return 0;
}
