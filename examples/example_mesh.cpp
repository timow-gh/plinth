#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <numeric>
#include <plinth/LightingConfig.hpp>
#include <plinth/Renderer.hpp>
#include <plinth/Texture.hpp>
#include <plinth/WindowSettings.hpp>

namespace {
constexpr std::size_t kVertexCount = 4;
constexpr std::size_t kColorComponents = 4;
constexpr std::size_t kColorFloats = kVertexCount * kColorComponents;
} // namespace

template<typename TIter>
static void set_color(TIter begin, TIter end, const std::array<float, 4>& color) {
    assert(std::distance(begin, end) % 4 == 0);
    for (auto it = begin; it != end; ++it) {
        *it = color[static_cast<std::size_t>(it - begin) % 4U];
    }
}

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
    const std::array<float, kVertexCount * 3> normals{0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 1.0F};
    std::array<float, kColorFloats> meshColor{};
    set_color(meshColor.begin(), meshColor.end(), colorGrey);

    const std::array<std::uint32_t, 6> indices{0, 1, 2, 0, 2, 3};

    renderer->add_mesh_drawable(vertices, normals, meshColor, indices);
    const float pointSize = 5.0F;
    std::array<float, kColorFloats> pointColor{};
    set_color(pointColor.begin(), pointColor.end(), colorBlack);
    std::vector<std::uint32_t> pointIndices(kVertexCount);
    std::iota(pointIndices.begin(), pointIndices.end(), 0U);
    renderer->add_point_drawable(vertices, pointColor, pointIndices, pointSize);

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
