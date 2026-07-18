#include <GLFW/glfw3.h>
#include <OpenGL/OpenGL.hpp>
#include <algorithm>
#include <array>
#include <cstdint>
#include <gtest/gtest.h>
#include <memory>
#include <plinth/Renderer.hpp>
#include <plinth/WindowSettings.hpp>
#include <utility>

namespace {

class RendererPresentationTest : public ::testing::Test {
  protected:
    static std::unique_ptr<renderer::Renderer> create_renderer(int samples) {
        renderer::WindowSettings settings;
        settings.title = "plinth renderer presentation test";
        settings.width = 512;
        settings.height = 256;
        settings.visible = false;
        settings.resizable = true;
        settings.double_buffer = false;
        settings.srgb_capable = false;
        settings.samples = samples;
        return renderer::Renderer::create(settings);
    }

    static std::array<std::uint8_t, 4> read_front_pixel(int x, int y) {
        std::array<std::uint8_t, 4> pixel{};
        glReadBuffer(GL_FRONT);
        glReadPixels(x, y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel.data());
        return pixel;
    }

    static std::pair<int, int> scene_interior_coordinate(renderer::Renderer& renderer) {
        const auto imgui = renderer.get_imgui().lock();
        if (!imgui) {
            ADD_FAILURE() << "Renderer ImGui overlay is unavailable";
            return {0, 0};
        }

        const auto sceneViewport = renderer::Renderer::calculate_scene_viewport(
            renderer.window().get_window_size(),
            renderer.window().get_framebuffer_size(),
            static_cast<double>(imgui->get_reserved_control_panel_width()));
        return {sceneViewport.framebuffer.x + sceneViewport.framebuffer.width / 2,
                sceneViewport.framebuffer.y + sceneViewport.framebuffer.height / 2};
    }

    static void expect_pixel_near(const std::array<std::uint8_t, 4>& pixel,
                                  const std::array<int, 4>& expected) {
        constexpr int tolerance = 5;
        EXPECT_NEAR(expected[0], static_cast<int>(pixel[0]), tolerance);
        EXPECT_NEAR(expected[1], static_cast<int>(pixel[1]), tolerance);
        EXPECT_NEAR(expected[2], static_cast<int>(pixel[2]), tolerance);
        EXPECT_NEAR(expected[3], static_cast<int>(pixel[3]), tolerance);
    }
};

TEST_F(RendererPresentationTest, PresentsSingleSamplePointThroughPublicFrameLifecycle) {
    auto instance = create_renderer(1);
    ASSERT_NE(nullptr, instance);
    GLboolean doubleBuffered = GL_TRUE;
    glGetBooleanv(GL_DOUBLEBUFFER, &doubleBuffered);
    ASSERT_EQ(GL_FALSE, doubleBuffered) << "Front-buffer readback requires a single-buffered context";
    ASSERT_FALSE(instance->window().is_srgb_capable()) << "Pixel assertions require a non-sRGB default framebuffer";

    constexpr std::array<float, 3> vertices{0.0F, 0.0F, 0.0F};
    constexpr std::array<float, 4> colors{1.0F, 0.0F, 0.0F, 1.0F};
    constexpr std::array<std::uint32_t, 1> indices{0U};
    const renderer::DrawableHandle handle = instance->add_point_drawable(vertices, colors, indices, 32.0F);
    ASSERT_TRUE(handle.is_valid());

    instance->begin_frame({0.0F, 0.0F, 0.2F, 1.0F});
    GLint sampleBuffers = 0;
    GLint activeSamples = 0;
    glGetIntegerv(GL_SAMPLE_BUFFERS, &sampleBuffers);
    glGetIntegerv(GL_SAMPLES, &activeSamples);
    ASSERT_EQ(0, sampleBuffers) << "Single-sample scene framebuffer must not provide a multisample buffer";
    ASSERT_EQ(0, activeSamples) << "Single-sample scene framebuffer must report zero active samples";
    instance->draw();
    instance->end_frame();

    const auto [x, y] = scene_interior_coordinate(*instance);
    expect_pixel_near(read_front_pixel(x, y), {255, 0, 0, 255});
    EXPECT_EQ(GL_NO_ERROR, glGetError());
}

TEST_F(RendererPresentationTest, ResolvesMultisampledPointThroughPublicFrameLifecycle) {
    auto instance = create_renderer(4);
    ASSERT_NE(nullptr, instance);
    GLboolean doubleBuffered = GL_TRUE;
    glGetBooleanv(GL_DOUBLEBUFFER, &doubleBuffered);
    ASSERT_EQ(GL_FALSE, doubleBuffered) << "Front-buffer readback requires a single-buffered context";
    ASSERT_FALSE(instance->window().is_srgb_capable()) << "Pixel assertions require a non-sRGB default framebuffer";

    GLint maxSamples = 0;
    glGetIntegerv(GL_MAX_SAMPLES, &maxSamples);
    ASSERT_GE(maxSamples, 2) << "RendererPresentationTest requires a supported OpenGL 4.1 context with at least "
                                "2 samples";

    constexpr std::array<float, 3> vertices{0.0F, 0.0F, 0.0F};
    constexpr std::array<float, 4> colors{1.0F, 0.0F, 0.0F, 1.0F};
    constexpr std::array<std::uint32_t, 1> indices{0U};
    const renderer::DrawableHandle handle = instance->add_point_drawable(vertices, colors, indices, 32.0F);
    ASSERT_TRUE(handle.is_valid());

    instance->begin_frame({0.0F, 0.0F, 0.2F, 1.0F});
    GLint sampleBuffers = 0;
    GLint activeSamples = 0;
    glGetIntegerv(GL_SAMPLE_BUFFERS, &sampleBuffers);
    glGetIntegerv(GL_SAMPLES, &activeSamples);
    ASSERT_GE(sampleBuffers, 1) << "Renderer scene framebuffer must provide a multisample buffer";
    ASSERT_EQ(std::min(4, maxSamples), activeSamples)
        << "Renderer scene framebuffer must honor the requested sample count subject to the GL limit";
    instance->draw();
    instance->end_frame();

    const auto [x, y] = scene_interior_coordinate(*instance);
    expect_pixel_near(read_front_pixel(x, y), {255, 0, 0, 255});
    EXPECT_EQ(GL_NO_ERROR, glGetError());
}

TEST_F(RendererPresentationTest, PresentsFreshOutputAndGeometryAfterFramebufferResize) {
    auto instance = create_renderer(1);
    ASSERT_NE(nullptr, instance);
    GLboolean doubleBuffered = GL_TRUE;
    glGetBooleanv(GL_DOUBLEBUFFER, &doubleBuffered);
    ASSERT_EQ(GL_FALSE, doubleBuffered) << "Front-buffer readback requires a single-buffered context";
    ASSERT_FALSE(instance->window().is_srgb_capable()) << "Pixel assertions require a non-sRGB default framebuffer";

    instance->begin_frame({1.0F, 0.0F, 0.0F, 1.0F});
    instance->draw();
    instance->end_frame();

    auto [x, y] = scene_interior_coordinate(*instance);
    expect_pixel_near(read_front_pixel(x, y), {255, 0, 0, 255});

    auto* window = static_cast<GLFWwindow*>(instance->window().get_native_handle());
    ASSERT_NE(nullptr, window);
    const auto initialFramebufferSize = instance->window().get_framebuffer_size();
    auto resizedFramebufferSize = initialFramebufferSize;
    glfwSetWindowSize(window, 640, 320);
    for (int attempt = 0; attempt < 50 && resizedFramebufferSize == initialFramebufferSize; ++attempt) {
        glfwWaitEventsTimeout(0.02);
        resizedFramebufferSize = instance->window().get_framebuffer_size();
    }
    ASSERT_NE(initialFramebufferSize, resizedFramebufferSize)
        << "Framebuffer did not resize from " << initialFramebufferSize.first << 'x' << initialFramebufferSize.second;

    constexpr std::array<float, 3> vertices{0.0F, 0.0F, 0.0F};
    constexpr std::array<float, 4> colors{0.0F, 0.0F, 1.0F, 1.0F};
    constexpr std::array<std::uint32_t, 1> indices{0U};
    const renderer::DrawableHandle handle = instance->add_point_drawable(vertices, colors, indices, 64.0F);
    ASSERT_TRUE(handle.is_valid());

    instance->begin_frame({0.0F, 1.0F, 0.0F, 1.0F});
    instance->draw();
    instance->end_frame();

    // The origin reaches the new center only when Renderer replaces its scene target after resize.
    const auto [resizedX, resizedY] = scene_interior_coordinate(*instance);
    expect_pixel_near(read_front_pixel(resizedX, resizedY), {0, 0, 255, 255});
    EXPECT_EQ(GL_NO_ERROR, glGetError());
}

} // namespace
