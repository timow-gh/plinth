#include "OpenGL/OpenGL.hpp"
#include "plinth/CameraProjectionType.hpp"
#include "plinth/Renderer.hpp"
#include "plinth/WindowSettings.hpp"

#include <array>
#include <cstdint>
#include <gtest/gtest.h>
#include <memory>
#include <utility>

namespace {

using SizePair = std::pair<int, int>;

void expect_pick_ray_matches(const renderer::Renderer::PickRay& actual, const renderer::PickRay& expected) {
    for (int axis = 0; axis < 3; ++axis) {
        EXPECT_FLOAT_EQ(actual.origin[axis], static_cast<float>(expected.origin[axis]));
        EXPECT_FLOAT_EQ(actual.direction[axis], static_cast<float>(expected.direction[axis]));
    }
}

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
        const auto sceneViewport = renderer.scene_viewport();
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
    constexpr std::array<float, 12>
        vertices{-1.0F, -1.0F, 0.0F, 1.0F, -1.0F, 0.0F, 1.0F, 1.0F, 0.0F, -1.0F, 1.0F, 0.0F};
    constexpr std::array<std::uint32_t, 6> indices{0U, 1U, 2U, 0U, 2U, 3U};
    constexpr std::array<float, 4> color{1.0F, 0.5F, 0.25F, 1.0F};
    const renderer::DrawableHandle handle =
        instance->add_mesh_drawable(vertices, indices, color, renderer::MeshCullFaceMode::NONE);
    ASSERT_TRUE(handle.is_valid());

    render_one_frame(*instance);

    const auto [cx, cy] = scene_center(*instance);
    const renderer::Renderer::PickRay expectedRay = instance->compute_pick_ray(cx, cy);
    const auto results = instance->pick_drawables(cx, cy, 2.0);
    ASSERT_EQ(1U, results.size());
    EXPECT_EQ(handle.kind, results.front().handle.kind);
    EXPECT_EQ(handle.id, results.front().handle.id);
    EXPECT_EQ(handle.rendererInstance, results.front().handle.rendererInstance);
    EXPECT_TRUE(results.front().handle.is_valid());
    EXPECT_EQ(expectedRay.origin, results.front().ray.origin);
    EXPECT_EQ(expectedRay.direction, results.front().ray.direction);
    EXPECT_EQ(GL_NO_ERROR, glGetError());
}

TEST_F(RendererPickingTest, ComputesRayForActiveProjection) {
    auto instance = create_renderer();
    ASSERT_NE(nullptr, instance);
    const auto camera = instance->get_camera().lock();
    ASSERT_NE(nullptr, camera);

    const auto [cx, cy] = scene_center(*instance);
    const double xpos = static_cast<double>(cx) + 40.0;
    const double ypos = static_cast<double>(cy) - 20.0;

    camera->set_projection_type(renderer::CameraProjectionType::PERSPECTIVE);
    const renderer::Renderer::PickRay perspectiveRay = instance->compute_pick_ray(xpos, ypos);
    expect_pick_ray_matches(perspectiveRay, camera->get_pick_ray(xpos, ypos));
    EXPECT_NEAR(linal::length(perspectiveRay.direction), 1.0F, 1e-6F);

    camera->set_projection_type(renderer::CameraProjectionType::ORTHOGRAPHIC);
    const renderer::Renderer::PickRay orthographicRay = instance->compute_pick_ray(xpos, ypos);
    expect_pick_ray_matches(orthographicRay, camera->get_pick_ray(xpos, ypos));
    EXPECT_NEAR(linal::length(orthographicRay.direction), 1.0F, 1e-6F);

    EXPECT_NE(perspectiveRay.origin, orthographicRay.origin);
    EXPECT_NE(perspectiveRay.direction, orthographicRay.direction);
}

TEST_F(RendererPickingTest, ReturnsEmptyOverBackground) {
    auto instance = create_renderer();
    ASSERT_NE(nullptr, instance);

    // Small quad at the origin; a corner of the scene viewport should miss it entirely.
    constexpr std::array<float, 12>
        vertices{-0.2F, -0.2F, 0.0F, 0.2F, -0.2F, 0.0F, 0.2F, 0.2F, 0.0F, -0.2F, 0.2F, 0.0F};
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
