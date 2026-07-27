#include <array>
#include <cstddef>
#include <cstdint>
#include <plinth/Renderer.hpp>
#include <plinth/Texture.hpp>
#include <plinth/WindowSettings.hpp>

template<typename TIter>
void set_color(TIter begin, TIter end, const std::array<float, 4>& color) {
    assert(std::distance(begin, end) % 4 == 0);
    for (auto it = begin; it != end; ++it) {
        *it = color[(it - begin) % 4];
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

    const std::array<float, 12> vertices{-1.0F, -1.0F, 0.0F, 1.0F, -1.0F, 0.0F, 1.0F, 1.0F, 0.0F, -1.0F, 1.0F, 0.0F};
    const std::array<float, 12> normals{0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 1.0F};
    std::array<float, 16> meshColor;
    set_color(meshColor.begin(), meshColor.end(), colorGrey);

    const std::array<std::uint32_t, 6> indices{0, 1, 2, 0, 2, 3};
    
    renderer::DrawableHandle meshDrawableHandle = renderer->add_mesh_drawable(vertices, normals, meshColor, indices);
    const float pointSize = 5.0F;
    std::array<float, 16> pointColor;
    set_color(pointColor.begin(), pointColor.end(), colorBlack);
    const std::size_t vertexCount = 4;
    std::vector<std::uint32_t> pointIndices(vertexCount);
    std::iota(pointIndices.begin(), pointIndices.end(), 0u);
    renderer::DrawableHandle vertexMeshDrawableHandle = renderer->add_point_drawable(vertices, pointColor, pointIndices, pointSize);

    while (!renderer->should_close()) {
        renderer::Renderer::poll_events();
        renderer->begin_frame();
        renderer->draw();
        renderer->end_frame();
    }
}
