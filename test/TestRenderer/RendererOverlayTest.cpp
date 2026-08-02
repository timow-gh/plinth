#include <GLFW/glfw3.h>
#include <gtest/gtest.h>
#include "plinth/ImGuiOverlay.hpp"
#include "plinth/Renderer.hpp"
#include "plinth/WindowSettings.hpp"
#include <memory>
#include <optional>

namespace {

class RendererOverlayTest : public ::testing::Test {
  protected:
    static void SetUpTestSuite() { ASSERT_EQ(GLFW_TRUE, glfwInit()); }
    static void TearDownTestSuite() { glfwTerminate(); }

    static renderer::WindowSettings base_settings() {
        renderer::WindowSettings settings;
        settings.title = "plinth overlay test";
        settings.width = 200;
        settings.height = 100;
        settings.visible = false;
        settings.resizable = false;
        settings.double_buffer = true;
        return settings;
    }
};

TEST_F(RendererOverlayTest, BuiltInOverlayRunsFrameLoopWithoutError) {
    auto settings = base_settings();
    settings.overlay = renderer::OverlayKind::BuiltInImGui;

    auto instance = renderer::Renderer::create(settings);
    ASSERT_NE(nullptr, instance);

    instance->begin_frame();
    instance->draw();
    instance->end_frame();
    SUCCEED();
}

TEST_F(RendererOverlayTest, InjectedOverlayParticipatesAndIsReleased) {
    auto settings = base_settings();
    settings.overlay = renderer::OverlayKind::None;

    auto instance = renderer::Renderer::create(settings);
    ASSERT_NE(nullptr, instance);

    auto overlay = std::make_shared<renderer::ImGuiOverlay>(instance->window().get_native_handle());
    const std::weak_ptr<renderer::ImGuiOverlay> view = overlay;
    instance->set_overlay(std::move(overlay));
    ASSERT_NE(nullptr, view.lock());

    // The injected overlay drives a frame without error.
    instance->begin_frame();
    instance->draw();
    instance->end_frame();

    instance.reset();
    EXPECT_EQ(nullptr, view.lock());
}

TEST_F(RendererOverlayTest, NoOverlayRunsFrameLoopWithoutError) {
    auto settings = base_settings();
    settings.overlay = renderer::OverlayKind::None;

    auto instance = renderer::Renderer::create(settings);
    ASSERT_NE(nullptr, instance);

    // The frame loop must be safe with no overlay present.
    instance->begin_frame();
    instance->draw();
    instance->end_frame();
    SUCCEED();
}

TEST_F(RendererOverlayTest, SceneViewportDefaultsToFullWindow) {
    auto settings = base_settings();
    settings.overlay = renderer::OverlayKind::None;

    auto instance = renderer::Renderer::create(settings);
    ASSERT_NE(nullptr, instance);

    const auto [fbWidth, fbHeight] = instance->window().get_framebuffer_size();
    const auto viewport = instance->scene_viewport();
    EXPECT_EQ(viewport.framebuffer.x, 0);
    EXPECT_EQ(viewport.framebuffer.y, 0);
    EXPECT_EQ(viewport.framebuffer.width, fbWidth);
    EXPECT_EQ(viewport.framebuffer.height, fbHeight);
}

TEST_F(RendererOverlayTest, UserDefinedSceneViewportIsApplied) {
    auto settings = base_settings();
    settings.overlay = renderer::OverlayKind::None;

    auto instance = renderer::Renderer::create(settings);
    ASSERT_NE(nullptr, instance);

    const auto [winWidth, winHeight] = instance->window().get_window_size();
    const auto [fbWidth, fbHeight] = instance->window().get_framebuffer_size();
    ASSERT_GT(winWidth, 0);
    ASSERT_GT(winHeight, 0);

    // Reserve a left strip; the scene occupies the remainder.
    constexpr double reserved = 40.0;
    instance->set_scene_viewport(renderer::LogicalViewportRect{
        reserved, 0.0, static_cast<double>(winWidth) - reserved, static_cast<double>(winHeight)});

    const auto viewport = instance->scene_viewport();
    EXPECT_DOUBLE_EQ(viewport.logical.x, reserved);
    EXPECT_DOUBLE_EQ(viewport.logical.width, static_cast<double>(winWidth) - reserved);

    // The framebuffer viewport starts inside the framebuffer and stays within bounds.
    EXPECT_GT(viewport.framebuffer.x, 0);
    EXPECT_LE(viewport.framebuffer.x + viewport.framebuffer.width, fbWidth);
    EXPECT_EQ(viewport.framebuffer.height, fbHeight);

    // A cursor inside the reserved strip maps to no scene coordinates; one to the right does.
    EXPECT_FALSE(
        renderer::Renderer::to_scene_framebuffer_coordinates(viewport, reserved - 1.0, 10.0).has_value());
    EXPECT_TRUE(
        renderer::Renderer::to_scene_framebuffer_coordinates(viewport, reserved + 1.0, 10.0).has_value());
}

TEST_F(RendererOverlayTest, BuiltInOverlayReservesSceneViewportBesidePanel) {
    auto settings = base_settings();
    settings.overlay = renderer::OverlayKind::BuiltInImGui;

    auto instance = renderer::Renderer::create(settings);
    ASSERT_NE(nullptr, instance);

    const auto [fbWidth, fbHeight] = instance->window().get_framebuffer_size();

    // The overlay reports its reserved region during end_frame(); it applies on the next
    // begin_frame(). Run a couple of frames so the reservation takes effect.
    for (int frame = 0; frame < 3; ++frame) {
        instance->begin_frame();
        instance->draw();
        instance->end_frame();
    }

    const auto viewport = instance->scene_viewport();
    // The scene now starts to the right of the panel and is narrower than the full window.
    EXPECT_GT(viewport.framebuffer.x, 0);
    EXPECT_LT(viewport.framebuffer.width, fbWidth);
    EXPECT_EQ(viewport.framebuffer.height, fbHeight);
}

TEST_F(RendererOverlayTest, ApplicationSceneViewportOverridesBuiltInOverlay) {
    auto settings = base_settings();
    settings.overlay = renderer::OverlayKind::BuiltInImGui;

    auto instance = renderer::Renderer::create(settings);
    ASSERT_NE(nullptr, instance);

    const auto [winWidth, winHeight] = instance->window().get_window_size();
    // The application pins a full-window scene viewport; the overlay's reservation must not win.
    instance->set_scene_viewport(renderer::LogicalViewportRect{
        0.0, 0.0, static_cast<double>(winWidth), static_cast<double>(winHeight)});

    for (int frame = 0; frame < 3; ++frame) {
        instance->begin_frame();
        instance->draw();
        instance->end_frame();
    }

    const auto [fbWidth, fbHeight] = instance->window().get_framebuffer_size();
    const auto viewport = instance->scene_viewport();
    EXPECT_EQ(viewport.framebuffer.x, 0);
    EXPECT_EQ(viewport.framebuffer.width, fbWidth);
    EXPECT_EQ(viewport.framebuffer.height, fbHeight);
}

TEST_F(RendererOverlayTest, ResetSceneViewportRestoresFullWindow) {
    auto settings = base_settings();
    auto instance = renderer::Renderer::create(settings);
    ASSERT_NE(nullptr, instance);

    const auto [winWidth, winHeight] = instance->window().get_window_size();
    instance->set_scene_viewport(renderer::LogicalViewportRect{
        20.0, 0.0, static_cast<double>(winWidth) - 20.0, static_cast<double>(winHeight)});
    ASSERT_DOUBLE_EQ(instance->scene_viewport().logical.x, 20.0);

    instance->set_scene_viewport(std::nullopt);
    const auto [fbWidth, fbHeight] = instance->window().get_framebuffer_size();
    const auto viewport = instance->scene_viewport();
    EXPECT_EQ(viewport.framebuffer.x, 0);
    EXPECT_EQ(viewport.framebuffer.width, fbWidth);
    EXPECT_EQ(viewport.framebuffer.height, fbHeight);
}

} // namespace
