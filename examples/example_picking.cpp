// Picking example: a scene combining every pickable drawable kind - cubes (meshes),
// colored axis lines, and point markers - wired up so a left-click selects whatever is
// under the cursor. The picked drawable is recolored to a bright highlight (only one at a
// time) and its label is printed to stdout, so the selection is visible both in the window
// and in the console.
//
// Picking uses Renderer::pick_drawables, which consumes scene-framebuffer coordinates. The
// cursor callback already reports coordinates in that space, so the latest cursor position
// is tracked and fed straight into the pick query on click.
//
// Left-drag orbits, scroll zooms; left-click picks; Esc quits.

#include "plinth/LineType.hpp"
#include "plinth/Renderer.hpp"
#include "plinth/WindowSettings.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace {

constexpr float cubeHalfExtent = 0.4F;
constexpr float markerPointSize = 14.0F;
constexpr float axisLineWidth = 4.0F;
constexpr double pickRadius = 4.0;
constexpr int mouseButtonLeft = 0; // GLFW_MOUSE_BUTTON_LEFT

using Color = std::array<float, 4>;

constexpr Color highlightColor{1.0F, 1.0F, 1.0F, 1.0F};

// One pickable scene entry. `rebuild` re-creates the drawable in the given color and
// returns the fresh handle; keeping it as a closure lets the pick handler recolor any kind
// of drawable without branching on kind.
struct SceneEntry {
    renderer::DrawableHandle handle;
    std::string label;
    Color originalColor;
    std::function<renderer::DrawableHandle(const Color&)> rebuild;
};

struct CubeMesh {
    std::vector<float> positions;
    std::vector<float> normals;
    std::vector<std::uint32_t> indices;
};

// A cube with 24 vertices (4 per face) so each face carries its own outward normal, which
// lets the lit mesh shader shade the faces distinctly.
CubeMesh make_cube(float h) {
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
            for (std::size_t axis = 0; axis < 3; ++axis) {
                mesh.positions.push_back(face.origin[axis] + (corner[0] * face.edgeU[axis]) +
                                         (corner[1] * face.edgeV[axis]));
            }
            for (std::size_t axis = 0; axis < 3; ++axis) {
                mesh.normals.push_back(face.normal[axis]);
            }
        }
        mesh.indices.insert(mesh.indices.end(), {base, base + 1U, base + 2U, base, base + 2U, base + 3U});
        base += 4U;
    }
    return mesh;
}

// Finds the entry whose current handle matches a pick result. Returns nullptr on a miss.
SceneEntry* find_entry(std::vector<SceneEntry>& scene, const renderer::DrawableHandle& handle) {
    for (SceneEntry& entry: scene) {
        if (entry.handle.kind == handle.kind && entry.handle.id == handle.id &&
            entry.handle.rendererInstance == handle.rendererInstance) {
            return &entry;
        }
    }
    return nullptr;
}

// Removes the entry's current drawable and rebuilds it in `color`, updating the stored
// handle. Used both to apply the highlight and to restore the original color.
void recolor(renderer::Renderer& renderer, SceneEntry& entry, const Color& color) {
    renderer.remove_drawable(entry.handle);
    entry.handle = entry.rebuild(color);
}

} // namespace

int main() {
    renderer::WindowSettings settings;
    settings.title = "picking example";
    auto renderer = renderer::Renderer::create(settings);
    if (!renderer) {
        return 1;
    }

    std::vector<SceneEntry> scene;

    // --- Meshes: two cubes offset along X so they sit apart ------------------------------
    const CubeMesh cube = make_cube(cubeHalfExtent);
    const std::array<std::pair<float, Color>, 2> cubeSpecs{{
        {-0.9F, Color{0.85F, 0.35F, 0.30F, 1.0F}}, // left cube: red
        {0.9F, Color{0.30F, 0.55F, 0.85F, 1.0F}},  // right cube: blue
    }};
    for (std::size_t i = 0; i < cubeSpecs.size(); ++i) {
        const auto [offsetX, color] = cubeSpecs[i];
        auto rebuild = [&renderer, &cube, offsetX](const Color& c) {
            const renderer::DrawableHandle handle =
                renderer->add_mesh_drawable(cube.positions, cube.indices, c, cube.normals);
            linal::hmatf transform = linal::hmatf::identity();
            // NOLINTNEXTLINE(clang-analyzer-core.NullDereference)
            transform.set_translation(linal::float3{offsetX, 0.0F, 0.0F});
            renderer->set_drawable_transform(handle, transform);
            return handle;
        };
        const renderer::DrawableHandle handle = rebuild(color);
        scene.push_back({handle, "cube " + std::to_string(i + 1), color, std::move(rebuild)});
    }

    // --- Lines: three colored axis segments through the origin ---------------------------
    const std::array<float, 18> lineVertices{
        -1.5F, 0.0F,  0.0F, 1.5F, 0.0F,  0.0F, // X axis
        0.0F,  -1.5F, 0.0F, 0.0F, 1.5F,  0.0F, // Y axis
        0.0F,  0.0F,  -1.5F, 0.0F, 0.0F, 1.5F, // Z axis
    };
    // Per-vertex colors are needed for the highlight to be uniform; each axis keeps a single
    // hue that the highlight overrides wholesale.
    const std::array<std::pair<Color, std::string>, 3> axisSpecs{{
        {Color{0.90F, 0.20F, 0.20F, 1.0F}, "X axis line"},
        {Color{0.20F, 0.80F, 0.20F, 1.0F}, "Y axis line"},
        {Color{0.30F, 0.45F, 0.95F, 1.0F}, "Z axis line"},
    }};
    for (std::size_t i = 0; i < axisSpecs.size(); ++i) {
        const auto& [color, label] = axisSpecs[i];
        const std::array<std::uint32_t, 2> segmentIndices{
            static_cast<std::uint32_t>(2U * i), static_cast<std::uint32_t>((2U * i) + 1U)};
        auto rebuild = [&renderer, lineVertices, segmentIndices](const Color& c) {
            const std::array<float, 24> colors{
                c[0], c[1], c[2], c[3],
                c[0], c[1], c[2], c[3],
                c[0], c[1], c[2], c[3],
                c[0], c[1], c[2], c[3],
                c[0], c[1], c[2], c[3],
                c[0], c[1], c[2], c[3],
            };
            renderer::StrokeStyle axisStyle;
            axisStyle.lineWidth = axisLineWidth;
            axisStyle.cap = renderer::LineCap::Round;
            axisStyle.join = renderer::LineJoin::Round;
            // NOLINTNEXTLINE(readability-magic-numbers)
            axisStyle.dashPattern = {0.2F, 0.1F}; // dash/gap lengths in world units
            return renderer->add_line_drawable(
                lineVertices, segmentIndices, colors, renderer::LineType::lines(), axisStyle);
        };
        const renderer::DrawableHandle handle = rebuild(color);
        scene.push_back({handle, label, color, std::move(rebuild)});
    }

    // --- Points: yellow markers at the axis tips -----------------------------------------
    const std::array<std::pair<std::array<float, 3>, std::string>, 3> pointSpecs{{
        {{1.5F, 0.0F, 0.0F}, "point marker +X"},
        {{0.0F, 1.5F, 0.0F}, "point marker +Y"},
        {{0.0F, 0.0F, 1.5F}, "point marker +Z"},
    }};
    const Color pointColor{1.0F, 0.85F, 0.10F, 1.0F};
    for (const auto& [position, label]: pointSpecs) {
        auto rebuild = [&renderer, position](const Color& c) {
            // NOLINTNEXTLINE(clang-analyzer-core.CallAndMessage)
            const std::array<float, 3> vertices{position[0], position[1], position[2]};
            return renderer->add_point_drawable(vertices, c, markerPointSize);
        };
        const renderer::DrawableHandle handle = rebuild(pointColor);
        scene.push_back({handle, label, pointColor, std::move(rebuild)});
    }

    // --- Picking: track the cursor, pick on left-click -----------------------------------
    std::pair<double, double> lastCursor{0.0, 0.0};
    const renderer::CallbackSubscription cursorSub =
        renderer->add_cursor_pos_callback([&lastCursor](double xpos, double ypos) {
            lastCursor = {xpos, ypos};
        });

    SceneEntry* highlighted = nullptr;
    const renderer::CallbackSubscription buttonSub = renderer->add_mouse_button_callback(
        [&renderer, &scene, &lastCursor, &highlighted](int button, renderer::Action action, renderer::Mods) {
            if (button != mouseButtonLeft || action != renderer::Action::PRESS) {
                return;
            }
            const auto results = renderer->pick_drawables(lastCursor.first, lastCursor.second, pickRadius);
            if (results.empty()) {
                std::cout << "nothing picked\n";
                return;
            }
            SceneEntry* picked = find_entry(scene, results.front().handle);
            if (picked == nullptr) {
                return; // Hit a drawable that is not tracked in the scene table.
            }
            std::cout << "picked: " << picked->label << '\n';
            if (highlighted == picked) {
                return; // Already highlighted; nothing to change.
            }
            if (highlighted != nullptr) {
                recolor(*renderer, *highlighted, highlighted->originalColor);
            }
            recolor(*renderer, *picked, highlightColor);
            highlighted = picked;
        });

    std::cout << "Left-click a cube, axis line, or point marker to pick it. Esc quits.\n";

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
