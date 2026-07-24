// Showcase example: a single scene combining everything the renderer currently supports -
// a texture-mapped cube, colored line segments, and colored points - so the whole feature
// set can be seen at a glance in one window.
//
// The texture is generated procedurally (a checkerboard), so the example needs no external
// image asset. Left-drag orbits, scroll zooms; Auto Zoom keeps the scene framed.

#include <array>
#include <cstddef>
#include <cstdint>
#include <plinth/LineType.hpp>
#include <plinth/Renderer.hpp>
#include <plinth/Texture.hpp>
#include <plinth/WindowSettings.hpp>
#include <vector>

namespace {

constexpr std::uint32_t textureSize = 64U;
constexpr std::uint32_t checkerCell = 8U;
constexpr float cubeHalfExtent = 0.6F;
constexpr float axisPointSize = 12.0F;
constexpr float axisLineWidth = 2.0F;

// A procedural RGBA8 checkerboard so the example needs no external texture asset.
std::vector<std::uint8_t> make_checkerboard() {
    constexpr std::uint8_t lightShade = 235U;
    constexpr std::uint8_t darkShade = 40U;
    constexpr std::uint8_t opaqueAlpha = 255U;

    std::vector<std::uint8_t> pixels(static_cast<std::size_t>(textureSize) * textureSize * 4U);
    for (std::uint32_t y = 0; y < textureSize; ++y) {
        for (std::uint32_t x = 0; x < textureSize; ++x) {
            const bool light = ((x / checkerCell) + (y / checkerCell)) % 2U == 0U;
            const std::uint8_t shade = light ? lightShade : darkShade;
            const auto pixel = static_cast<std::size_t>((y * textureSize + x) * 4U);
            pixels[pixel + 0U] = shade;
            pixels[pixel + 1U] = shade;
            pixels[pixel + 2U] = shade;
            pixels[pixel + 3U] = opaqueAlpha;
        }
    }
    return pixels;
}

struct CubeMesh {
    std::vector<float> positions;
    std::vector<float> normals;
    std::vector<float> uvs;
    std::vector<float> colors;
    std::vector<std::uint32_t> indices;
};

// A cube with 24 vertices (4 per face) so each face gets its own normal and its own full
// 0..1 UV quad - i.e. the checkerboard is mapped once per face.
CubeMesh make_textured_cube(float h) {
    struct Face {
        std::array<float, 3> origin;
        std::array<float, 3> edgeU;
        std::array<float, 3> edgeV;
        std::array<float, 3> normal;
    };

    const std::array<Face, 6> faces{{
        {{h, -h, h}, {0, 0, -2 * h}, {0, 2 * h, 0}, {1, 0, 0}},   // +X
        {{-h, -h, -h}, {0, 0, 2 * h}, {0, 2 * h, 0}, {-1, 0, 0}}, // -X
        {{-h, h, h}, {2 * h, 0, 0}, {0, 0, -2 * h}, {0, 1, 0}},   // +Y
        {{-h, -h, -h}, {2 * h, 0, 0}, {0, 0, 2 * h}, {0, -1, 0}}, // -Y
        {{-h, -h, h}, {2 * h, 0, 0}, {0, 2 * h, 0}, {0, 0, 1}},   // +Z
        {{h, -h, -h}, {-2 * h, 0, 0}, {0, 2 * h, 0}, {0, 0, -1}}, // -Z
    }};

    CubeMesh mesh;
    std::uint32_t base = 0;
    for (const Face& face: faces) {
        const std::array<std::array<float, 2>, 4> corners{{{0, 0}, {1, 0}, {1, 1}, {0, 1}}};
        for (const auto& corner: corners) {
            for (int axis = 0; axis < 3; ++axis) {
                mesh.positions.push_back(face.origin[axis] + corner[0] * face.edgeU[axis] +
                                         corner[1] * face.edgeV[axis]);
            }
            for (int axis = 0; axis < 3; ++axis) {
                mesh.normals.push_back(face.normal[axis]);
            }
            mesh.uvs.push_back(corner[0]);
            mesh.uvs.push_back(corner[1]);
            // Opaque white vertex color so the texture shows through unmodulated.
            mesh.colors.insert(mesh.colors.end(), {1.0F, 1.0F, 1.0F, 1.0F});
        }
        mesh.indices.insert(mesh.indices.end(), {base, base + 1U, base + 2U, base, base + 2U, base + 3U});
        base += 4U;
    }
    return mesh;
}

} // namespace

int main() {
    renderer::WindowSettings settings;
    settings.title = "showcase scene example";
    auto renderer = renderer::Renderer::create(settings);
    if (!renderer) {
        return 1;
    }

    // --- Textured cube -------------------------------------------------------------------
    const std::vector<std::uint8_t> checkerboard = make_checkerboard();
    const auto texture = renderer->create_texture_2d(
        {textureSize, textureSize, checkerboard, renderer::TextureColorSpace::srgb, renderer::TextureFilter::nearest});
    const CubeMesh cube = make_textured_cube(cubeHalfExtent);
    renderer->add_textured_mesh_drawable(cube.positions, cube.normals, cube.uvs, cube.colors, cube.indices, texture);

    // --- Lines: three colored axes through the origin ------------------------------------
    const std::array<float, 18> lineVertices{
        -1.0F,
        0.0F,
        0.0F,
        1.0F,
        0.0F,
        0.0F, // X axis
        0.0F,
        -1.0F,
        0.0F,
        0.0F,
        1.0F,
        0.0F, // Y axis
        0.0F,
        0.0F,
        -1.0F,
        0.0F,
        0.0F,
        1.0F, // Z axis
    };
    const std::array<float, 24> lineColors{
        1.0F, 0.0F, 0.0F, 1.0F, 1.0F, 0.0F, 0.0F, 1.0F, // X: red
        0.0F, 1.0F, 0.0F, 1.0F, 0.0F, 1.0F, 0.0F, 1.0F, // Y: green
        0.0F, 0.0F, 1.0F, 1.0F, 0.0F, 0.0F, 1.0F, 1.0F, // Z: blue
    };
    const std::array<std::uint32_t, 6> lineIndices{0U, 1U, 2U, 3U, 4U, 5U};
    renderer->add_line_drawable(lineVertices, lineIndices, lineColors, renderer::LineType::lines(), axisLineWidth);

    // --- Points: yellow markers at the axis tips -----------------------------------------
    const std::array<float, 9> pointVertices{
        1.0F,
        0.0F,
        0.0F, //
        0.0F,
        1.0F,
        0.0F, //
        0.0F,
        0.0F,
        1.0F, //
    };
    const std::array<float, 12> pointColors{
        1.0F,
        1.0F,
        0.0F,
        1.0F, //
        1.0F,
        1.0F,
        0.0F,
        1.0F, //
        1.0F,
        1.0F,
        0.0F,
        1.0F, //
    };
    const std::array<std::uint32_t, 3> pointIndices{0U, 1U, 2U};
    renderer->add_point_drawable(pointVertices, pointColors, pointIndices, axisPointSize);

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
