#include "plinth/Renderer.hpp"

#include <gtest/gtest.h>

namespace {
// The scene occupies the window minus a left sidebar of the given logical width.
renderer::LogicalViewportRect scene_minus_left_sidebar(double windowWidth, double windowHeight, double sidebarWidth) {
    return renderer::LogicalViewportRect{sidebarWidth, 0.0, windowWidth - sidebarWidth, windowHeight};
}
} // namespace

TEST(RendererViewportTest, ReservesLeftSidebarInLogicalAndFramebufferSpace) {
    const renderer::SceneViewport viewport =
        renderer::Renderer::calculate_scene_viewport({1000, 500},
                                                     {2000, 1000},
                                                     scene_minus_left_sidebar(1000.0, 500.0, 328.0));

    EXPECT_DOUBLE_EQ(viewport.logical.x, 328.0);
    EXPECT_DOUBLE_EQ(viewport.logical.y, 0.0);
    EXPECT_DOUBLE_EQ(viewport.logical.width, 672.0);
    EXPECT_DOUBLE_EQ(viewport.logical.height, 500.0);

    EXPECT_EQ(viewport.framebuffer.x, 656);
    EXPECT_EQ(viewport.framebuffer.y, 0);
    EXPECT_EQ(viewport.framebuffer.width, 1344);
    EXPECT_EQ(viewport.framebuffer.height, 1000);
}

TEST(RendererViewportTest, TranslatesWindowCoordinatesToSceneFramebufferCoordinates) {
    const renderer::SceneViewport viewport =
        renderer::Renderer::calculate_scene_viewport({1000, 500},
                                                     {2000, 1000},
                                                     scene_minus_left_sidebar(1000.0, 500.0, 328.0));

    const auto leftEdge = renderer::Renderer::to_scene_framebuffer_coordinates(viewport, 328.0, 250.0);
    ASSERT_TRUE(leftEdge.has_value());
    EXPECT_DOUBLE_EQ(leftEdge->first, 0.0);
    EXPECT_DOUBLE_EQ(leftEdge->second, 500.0);

    const auto scenePoint = renderer::Renderer::to_scene_framebuffer_coordinates(viewport, 400.0, 100.0);
    ASSERT_TRUE(scenePoint.has_value());
    EXPECT_DOUBLE_EQ(scenePoint->first, 144.0);
    EXPECT_DOUBLE_EQ(scenePoint->second, 200.0);
}

TEST(RendererViewportTest, RejectsSidebarAndOutsideWindowCoordinates) {
    const renderer::SceneViewport viewport =
        renderer::Renderer::calculate_scene_viewport({1000, 500},
                                                     {2000, 1000},
                                                     scene_minus_left_sidebar(1000.0, 500.0, 328.0));

    EXPECT_FALSE(renderer::Renderer::to_scene_framebuffer_coordinates(viewport, 327.0, 250.0).has_value());
    EXPECT_FALSE(renderer::Renderer::to_scene_framebuffer_coordinates(viewport, 1000.0, 250.0).has_value());
    EXPECT_FALSE(renderer::Renderer::to_scene_framebuffer_coordinates(viewport, 400.0, 500.0).has_value());
}

TEST(RendererViewportTest, KeepsValidViewportWhenSidebarConsumesAlmostAllSpace) {
    // A sidebar wider than the window clamps the scene to a 1px-wide strip at the edge.
    const renderer::SceneViewport viewport =
        renderer::Renderer::calculate_scene_viewport({200, 100},
                                                     {400, 200},
                                                     renderer::LogicalViewportRect{500.0, 0.0, 200.0, 100.0});

    EXPECT_DOUBLE_EQ(viewport.logical.x, 199.0);
    EXPECT_DOUBLE_EQ(viewport.logical.width, 1.0);
    EXPECT_EQ(viewport.framebuffer.x, 398);
    EXPECT_EQ(viewport.framebuffer.width, 2);
    EXPECT_EQ(viewport.framebuffer.height, 200);
}

TEST(RendererViewportTest, FullWindowRectFillsFramebuffer) {
    const renderer::SceneViewport viewport =
        renderer::Renderer::calculate_scene_viewport({1000, 500},
                                                     {2000, 1000},
                                                     renderer::LogicalViewportRect{0.0, 0.0, 1000.0, 500.0});

    EXPECT_DOUBLE_EQ(viewport.logical.x, 0.0);
    EXPECT_DOUBLE_EQ(viewport.logical.width, 1000.0);
    EXPECT_EQ(viewport.framebuffer.x, 0);
    EXPECT_EQ(viewport.framebuffer.y, 0);
    EXPECT_EQ(viewport.framebuffer.width, 2000);
    EXPECT_EQ(viewport.framebuffer.height, 1000);
}

TEST(RendererViewportTest, MapsTopOffsetRectToFramebufferPixels) {
    // A rect inset from the top maps its y-offset and height into framebuffer pixels.
    const renderer::SceneViewport viewport =
        renderer::Renderer::calculate_scene_viewport({1000, 500},
                                                     {2000, 1000},
                                                     renderer::LogicalViewportRect{0.0, 100.0, 1000.0, 400.0});

    EXPECT_DOUBLE_EQ(viewport.logical.y, 100.0);
    EXPECT_DOUBLE_EQ(viewport.logical.height, 400.0);
    EXPECT_EQ(viewport.framebuffer.y, 200);
    EXPECT_EQ(viewport.framebuffer.height, 800);
}
