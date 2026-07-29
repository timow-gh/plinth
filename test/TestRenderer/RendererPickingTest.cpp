#include "OpenGL/OpenGL.hpp"
#include "plinth/Renderer.hpp"
#include "plinth/WindowSettings.hpp"
#include <array>
#include <cstdint>
#include <gtest/gtest.h>
#include <memory>
#include <utility>

namespace {

using SizePair = std::pair<int, int>;

class RendererPickingTest : public ::testing::Test {
  protected:
    static std::unique_ptr<renderer::Renderer> create_renderer(int width = 512, int height = 256) {
        renderer::WindowSettings settings;
        settings.title = "plinth renderer picking test";
        settings.width = static_cast<std::uint32_t>(width);
        settings.height = static_cast<std::uint32_t>(height);
        settings.visible = false;
        settings.resizable = false;
        settings.double_buffer = false;
        settings.srgb_capable = false;
        settings.samples = 1;
        return renderer::Renderer::create(settings);
    }

    // The center of the scene viewport, expressed in scene-framebuffer coordinates (the space
    // pick_drawables consumes: local to the scene viewport, top-left origin).
    static SizePair scene_center(renderer::Renderer& renderer) {
        const auto imgui = renderer.get_imgui().lock();
        if (!imgui) {
            ADD_FAILURE() << "Renderer ImGui overlay is unavailable";
            return {0, 0};
        }
        const auto sceneViewport = renderer::Renderer::calculate_scene_viewport(
            renderer.window().get_window_size(),
            renderer.window().get_framebuffer_size(),
            static_cast<double>(imgui->get_reserved_control_panel_width()));
        return {sceneViewport.framebuffer.width / 2, sceneViewport.framebuffer.height / 2};
    }

    static void render_one_frame(renderer::Renderer& renderer) {
        renderer.begin_frame({0.0F, 0.0F, 0.0F, 1.0F});
        renderer.draw();
        renderer.end_frame();
        glFinish();
    }
};

TEST_F(RendererPickingTest, PicksMeshDrawableUnderCursorCenter) {
    auto instance = create_renderer();
    ASSERT_NE(nullptr, instance);

    // A quad on the z=0 plane centered at the origin, facing +z (CCW winding) toward the camera.
    constexpr std::array<float, 12> vertices{-1.0F, -1.0F, 0.0F, 1.0F, -1.0F, 0.0F,
                                             1.0F,  1.0F,  0.0F, -1.0F, 1.0F, 0.0F};
    constexpr std::array<std::uint32_t, 6> indices{0U, 1U, 2U, 0U, 2U, 3U};
    constexpr std::array<float, 4> color{1.0F, 0.5F, 0.25F, 1.0F};
    const renderer::DrawableHandle handle =
        instance->add_mesh_drawable(vertices, indices, color, renderer::MeshCullFaceMode::NONE);
    ASSERT_TRUE(handle.is_valid());

    render_one_frame(*instance);

    const auto [cx, cy] = scene_center(*instance);
    const auto results = instance->pick_drawables(cx, cy, 2.0);
    ASSERT_EQ(1U, results.size());
    EXPECT_EQ(handle.kind, results.front().handle.kind);
    EXPECT_EQ(handle.id, results.front().handle.id);
    EXPECT_EQ(handle.rendererInstance, results.front().handle.rendererInstance);
    EXPECT_TRUE(results.front().handle.is_valid());
    EXPECT_EQ(GL_NO_ERROR, glGetError());
}

TEST_F(RendererPickingTest, ReturnsEmptyOverBackground) {
    auto instance = create_renderer();
    ASSERT_NE(nullptr, instance);

    // Small quad at the origin; a corner of the scene viewport should miss it entirely.
    constexpr std::array<float, 12> vertices{-0.2F, -0.2F, 0.0F, 0.2F, -0.2F, 0.0F,
                                             0.2F,  0.2F,  0.0F, -0.2F, 0.2F, 0.0F};
    constexpr std::array<std::uint32_t, 6> indices{0U, 1U, 2U, 0U, 2U, 3U};
    constexpr std::array<float, 4> color{0.2F, 0.8F, 0.4F, 1.0F};
    const renderer::DrawableHandle handle =
        instance->add_mesh_drawable(vertices, indices, color, renderer::MeshCullFaceMode::NONE);
    ASSERT_TRUE(handle.is_valid());

    render_one_frame(*instance);

    const auto results = instance->pick_drawables(1.0, 1.0, 1.0);
    EXPECT_TRUE(results.empty());
    EXPECT_EQ(GL_NO_ERROR, glGetError());
}

TEST_F(RendererPickingTest, ReturnsEmptyWhenNoDrawables) {
    auto instance = create_renderer();
    ASSERT_NE(nullptr, instance);
    render_one_frame(*instance);

    const auto [cx, cy] = scene_center(*instance);
    EXPECT_TRUE(instance->pick_drawables(cx, cy, 4.0).empty());
    EXPECT_EQ(GL_NO_ERROR, glGetError());
}

} // namespace
