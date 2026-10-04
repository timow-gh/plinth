// depthLayer bias regression tests at close zoom.
//
// The camera sits just above a large quad and looks across it at a grazing angle. Part of the quad
// is behind the camera, so the per-frame clip-plane fit drops the near plane to its 0.01 floor
// while the lines stay roughly 1-3 units away. This is the "zoomed in very close" case. A
// camera-ward bias that is a constant NDC offset grows as d^2/near in world units there, and lines
// hidden a hair behind a face bleed through it. A bias relative to view depth does not.

#include "plinth/Renderer.hpp"
#include "plinth/StrokeStyle.hpp"
#include "plinth/WindowSettings.hpp"

#include <GLFW/glfw3.h>
#include <array>
#include <cstddef>
#include <cstdint>
#include <gtest/gtest.h>
#include <memory>
#include <vector>

namespace {

constexpr int windowSize = 256;
constexpr float lineWidth = 4.0F;
constexpr float occluderHalfExtent = 4.0F;
// Gap between a hidden line and the face covering it. Along the grazing view rays this is a
// view-depth gap of roughly 0.003-0.01, far larger than any sensible per-layer bias.
constexpr float hiddenGap = 1.0e-3F;

constexpr std::array<float, 4> faceColor{0.5F, 0.5F, 0.5F, 1.0F};
constexpr std::array<float, 4> magenta{1.0F, 0.0F, 1.0F, 1.0F};
constexpr std::array<float, 4> cyan{0.0F, 1.0F, 1.0F, 1.0F};

struct PixelCounts {
    std::size_t magenta{0};
    std::size_t cyan{0};
};

class LineDepthBiasTest : public ::testing::Test {
  protected:
    static void SetUpTestSuite() { ASSERT_EQ(GLFW_TRUE, glfwInit()); }

    static void TearDownTestSuite() { glfwTerminate(); }

    void SetUp() override {
        renderer::WindowSettings settings;
        settings.title = "plinth line depth bias test";
        settings.width = windowSize;
        settings.height = windowSize;
        settings.visible = false;
        settings.resizable = false;
        settings.double_buffer = true;
        settings.overlay = renderer::OverlayKind::None;

        m_renderer = renderer::Renderer::create(settings);
        ASSERT_NE(nullptr, m_renderer);
        m_renderer->set_msaa_samples(1);
        m_renderer->set_fxaa_enabled(false);

        renderer::CameraAutoFitSettings autoFit = m_renderer->get_camera_auto_fit_settings();
        autoFit.enabled = false;
        m_renderer->set_camera_auto_fit_settings(autoFit);

        const std::shared_ptr<renderer::CameraInteractor> camera = m_renderer->get_camera().lock();
        ASSERT_NE(nullptr, camera);
        camera->look_at(linal::double3{0.0, -0.5, 0.3}, linal::double3{0.0, 1.5, 0.0}, linal::double3{0.0, 0.0, 1.0});
    }

    void TearDown() override { m_renderer.reset(); }

    void add_occluder() {
        const std::array<float, 12> vertices{-occluderHalfExtent,
                                             -occluderHalfExtent,
                                             0.0F,
                                             occluderHalfExtent,
                                             -occluderHalfExtent,
                                             0.0F,
                                             occluderHalfExtent,
                                             occluderHalfExtent,
                                             0.0F,
                                             -occluderHalfExtent,
                                             occluderHalfExtent,
                                             0.0F};
        const std::array<std::uint32_t, 6> indices{0, 1, 2, 0, 2, 3};
        ASSERT_TRUE(
            m_renderer->add_mesh_drawable(vertices, indices, faceColor, renderer::MeshCullFaceMode::NONE).is_valid());
    }

    // A point behind the camera: without occluding anything it holds the near plane at its floor,
    // as the off-screen part of the quad does when it is in the scene.
    void add_near_plane_anchor() {
        const std::array<float, 3> vertex{0.0F, -2.0F, 0.3F};
        ASSERT_TRUE(m_renderer->add_point_drawable(vertex, faceColor).is_valid());
    }

    // A line running away from the camera across the quad, at height z.
    // It passes through the camera target, so it crosses the center of the screen.
    renderer::DrawableHandle add_line(float z, std::array<float, 4> color, std::int32_t depthLayer) {
        const std::array<float, 6> vertices{0.0F, 0.4F, z, 0.0F, 2.5F, z};
        renderer::StrokeStyle style;
        style.lineWidth = lineWidth;
        style.depthLayer = depthLayer;
        const renderer::DrawableHandle handle =
            m_renderer->add_line_drawable(vertices, color, renderer::LineType::lines(), style);
        EXPECT_TRUE(handle.is_valid());
        return handle;
    }

    PixelCounts render_and_count() {
        // Two frames: the first lets the per-frame clip-plane fit settle on this scene.
        for (int frame = 0; frame < 2; ++frame) {
            m_renderer->begin_frame();
            m_renderer->draw();
            m_renderer->end_frame();
        }

        std::vector<std::uint8_t> pixels;
        int width = 0;
        int height = 0;
        EXPECT_TRUE(m_renderer->read_scene_pixels(pixels, width, height));

        PixelCounts counts;
        for (std::size_t i = 0; i + 2 < pixels.size(); i += 3) {
            const std::uint8_t r = pixels[i];
            const std::uint8_t g = pixels[i + 1];
            const std::uint8_t b = pixels[i + 2];
            if (r > 100U && b > 100U && g < 60U) {
                ++counts.magenta;
            }
            if (g > 100U && b > 100U && r < 60U) {
                ++counts.cyan;
            }
        }
        return counts;
    }

    [[nodiscard]] double near_plane() const { return m_renderer->get_camera().lock()->get_near_plane(); }

    std::unique_ptr<renderer::Renderer> m_renderer;
};

} // namespace

// Guards the scenario itself: geometry behind the camera must force the minimum near plane, which
// is where a constant NDC bias is at its largest in world units.
TEST_F(LineDepthBiasTest, CloseZoomSceneUsesMinimumNearPlane) {
    add_occluder();
    add_line(-hiddenGap, magenta, 1);
    render_and_count();
    EXPECT_LE(near_plane(), 0.0101);
}

// A reference line with nothing in front of it, so the hidden-line assertions are known to be
// looking at pixels that would otherwise show the line.
TEST_F(LineDepthBiasTest, UnoccludedLineIsVisible) {
    add_line(-hiddenGap, magenta, 1);
    add_near_plane_anchor();
    EXPECT_GT(render_and_count().magenta, 100U);
}

class LineDepthBiasHiddenTest
    : public LineDepthBiasTest
    , public ::testing::WithParamInterface<std::int32_t> {};

// The reported bug: a line just behind a face shows through when zoomed in close.
TEST_P(LineDepthBiasHiddenTest, LineJustBehindFaceStaysHidden) {
    add_occluder();
    add_line(-hiddenGap, magenta, GetParam());
    EXPECT_EQ(0U, render_and_count().magenta) << "depthLayer " << GetParam();
}

INSTANTIATE_TEST_SUITE_P(DepthLayers, LineDepthBiasHiddenTest, ::testing::Values(0, 1, 2, 6));

class LineDepthBiasCoplanarTest
    : public LineDepthBiasTest
    , public ::testing::WithParamInterface<std::int32_t> {};

// The guarantee depthLayer exists for: a line lying exactly on a face wins the depth test.
TEST_P(LineDepthBiasCoplanarTest, CoplanarLineBeatsFace) {
    add_line(0.0F, magenta, GetParam());
    add_near_plane_anchor();
    const std::size_t reference = render_and_count().magenta;
    ASSERT_GT(reference, 100U);

    m_renderer->clear_drawables();
    add_occluder();
    add_line(0.0F, magenta, GetParam());
    EXPECT_GE(render_and_count().magenta * 100U, reference * 98U) << "depthLayer " << GetParam();
}

INSTANTIATE_TEST_SUITE_P(DepthLayers, LineDepthBiasCoplanarTest, ::testing::Values(1, 2, 6));

// Points share the line path's depthLayer bias, so a point just behind a face must stay hidden too.
// One-pixel points keep this about the bias: a larger sprite has a single flat depth, and at this
// grazing angle the face's depth changes by more than the gap across a few pixels.
TEST_P(LineDepthBiasHiddenTest, PointsJustBehindFaceStayHidden) {
    const auto add_points = [this] {
        const std::array<float, 9> vertices{0.0F, 0.6F, -hiddenGap, 0.0F, 1.2F, -hiddenGap, 0.0F, 2.4F, -hiddenGap};
        ASSERT_TRUE(
            m_renderer->add_point_drawable(vertices, magenta, 1.0F, renderer::BufferAccessPattern::Static, GetParam())
                .is_valid());
    };
    add_points();
    add_near_plane_anchor();
    ASSERT_GT(render_and_count().magenta, 0U);

    m_renderer->clear_drawables();
    add_occluder();
    add_points();
    EXPECT_EQ(0U, render_and_count().magenta) << "depthLayer " << GetParam();
}

// A higher depthLayer stacks in front of a lower one on the same face, whatever the draw order.
TEST_F(LineDepthBiasTest, HigherLayerWinsOverLowerLayerOnFace) {
    add_occluder();
    add_line(0.0F, magenta, 1);
    add_line(0.0F, cyan, 2);
    const PixelCounts counts = render_and_count();
    EXPECT_GT(counts.cyan, 100U);
    EXPECT_LE(counts.magenta * 50U, counts.cyan) << "magenta " << counts.magenta << " cyan " << counts.cyan;
}

// Picking renders into a 24-bit depth target, where the close-zoom depth step is smallest.
TEST_F(LineDepthBiasTest, CoplanarLineOnFaceIsPicked) {
    add_occluder();
    const renderer::DrawableHandle line = add_line(0.0F, magenta, 1);
    render_and_count();

    const auto sceneViewport = m_renderer->scene_viewport();
    const auto results = m_renderer->pick_drawables(sceneViewport.framebuffer.width / 2.0,
                                                    sceneViewport.framebuffer.height / 2.0,
                                                    0.0);
    ASSERT_EQ(1U, results.size());
    EXPECT_EQ(line.id, results.front().handle.id);
    EXPECT_EQ(line.kind, results.front().handle.kind);
}
