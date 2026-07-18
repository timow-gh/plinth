#include <GLFW/glfw3.h>
#include <OpenGL/OpenGL.hpp>
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <gtest/gtest.h>
#include <memory>
#include <optional>
#include <plinth/Renderer.hpp>
#include <plinth/WindowSettings.hpp>
#include <sstream>
#include <string>
#include <utility>

namespace {

using SizePair = std::pair<int, int>;

struct SizeSnapshot {
    SizePair logical;
    SizePair framebuffer;
};

struct ResizeTransition {
    SizePair logical;
    SizePair framebuffer;
};

struct ResizeResult {
    bool stable{false};
    SizePair finalLogical;
    SizePair finalFramebuffer;
    std::array<ResizeTransition, 32> transitions{};
    std::size_t transitionCount{0};
    bool overflow{false};
};

struct ReadCoordinates {
    SizePair center;
    SizePair background;
    SizePair edge;
};

struct FrameResult {
    GLenum beforeBeginError{GL_NO_ERROR};
    GLenum beginError{GL_NO_ERROR};
    GLenum drawError{GL_NO_ERROR};
    GLenum endError{GL_NO_ERROR};
    GLenum finishError{GL_NO_ERROR};
    GLenum bindingError{GL_NO_ERROR};
    GLenum readError{GL_NO_ERROR};
    GLint sampleBuffers{0};
    GLint activeSamples{0};
    GLint readFramebufferBinding{-1};
    std::array<std::uint8_t, 4> centerPixel{};
    std::array<std::uint8_t, 4> backgroundPixel{};
    std::array<std::uint8_t, 4> edgePixel{};
    SizeSnapshot afterFinish;
    SizeSnapshot afterRead;
};

struct SteadyResult {
    bool stable{true};
    SizeSnapshot last;
};

SizeSnapshot get_size_snapshot(renderer::Renderer& renderer) {
    return {renderer.window().get_window_size(), renderer.window().get_framebuffer_size()};
}

bool same_transition(const ResizeTransition& lhs, const ResizeTransition& rhs) {
    return lhs.logical == rhs.logical && lhs.framebuffer == rhs.framebuffer;
}

void retain_unique_transition(ResizeResult& result, const SizeSnapshot& snapshot) {
    const ResizeTransition transition{snapshot.logical, snapshot.framebuffer};
    for (std::size_t index = 0; index < result.transitionCount; ++index) {
        if (same_transition(result.transitions[index], transition)) {
            return;
        }
    }
    if (result.transitionCount == result.transitions.size()) {
        result.overflow = true;
        return;
    }
    result.transitions[result.transitionCount++] = transition;
}

ResizeResult wait_for_stable_resize(renderer::Renderer& renderer,
                                    GLFWwindow* window,
                                    SizePair requestedLogical,
                                    const SizeSnapshot& preceding) {
    using Clock = std::chrono::steady_clock;
    constexpr auto timeout = std::chrono::seconds(2);
    constexpr auto quietPeriod = std::chrono::milliseconds(100);
    constexpr auto maximumWait = std::chrono::milliseconds(10);

    ResizeResult result;
    const auto deadline = Clock::now() + timeout;
    std::optional<SizeSnapshot> previous;
    std::optional<Clock::time_point> quietSince;
    glfwSetWindowSize(window, requestedLogical.first, requestedLogical.second);

    while (Clock::now() < deadline) {
        const auto beforeWait = Clock::now();
        const auto remaining = deadline - beforeWait;
        const auto waitDuration = std::min(remaining, Clock::duration(maximumWait));
        glfwWaitEventsTimeout(std::chrono::duration<double>(waitDuration).count());

        const auto sampledAt = Clock::now();
        const SizeSnapshot snapshot = get_size_snapshot(renderer);
        result.finalLogical = snapshot.logical;
        result.finalFramebuffer = snapshot.framebuffer;
        retain_unique_transition(result, snapshot);

        const bool candidate = snapshot.logical == requestedLogical && snapshot.framebuffer.first > 0 &&
                               snapshot.framebuffer.second > 0 &&
                               snapshot.framebuffer != preceding.framebuffer;
        const bool changed = !previous.has_value() || previous->logical != snapshot.logical ||
                             previous->framebuffer != snapshot.framebuffer;
        if (!candidate) {
            quietSince.reset();
        } else if (changed || !quietSince.has_value()) {
            quietSince = sampledAt;
        } else if (sampledAt - *quietSince >= quietPeriod) {
            result.stable = true;
            return result;
        }
        previous = snapshot;
    }
    return result;
}

std::string describe_sizes(const SizeSnapshot& snapshot) {
    std::ostringstream output;
    output << "logical=" << snapshot.logical.first << 'x' << snapshot.logical.second << ", framebuffer="
           << snapshot.framebuffer.first << 'x' << snapshot.framebuffer.second;
    return output.str();
}

std::string describe_resize(const ResizeResult& result,
                            SizePair requestedLogical,
                            const SizeSnapshot& preceding) {
    std::ostringstream output;
    output << "requested logical=" << requestedLogical.first << 'x' << requestedLogical.second
           << ", previous " << describe_sizes(preceding) << ", final logical=" << result.finalLogical.first << 'x'
           << result.finalLogical.second << ", final framebuffer=" << result.finalFramebuffer.first << 'x'
           << result.finalFramebuffer.second
           << ", transitions=[";
    for (std::size_t index = 0; index < result.transitionCount; ++index) {
        if (index != 0) {
            output << ", ";
        }
        const auto& transition = result.transitions[index];
        output << transition.logical.first << 'x' << transition.logical.second << '/'
               << transition.framebuffer.first << 'x' << transition.framebuffer.second;
    }
    output << ']';
    if (result.overflow) {
        output << " (more than " << result.transitions.size() << " unique transitions)";
    }
    return output.str();
}

ReadCoordinates calculate_read_coordinates(const SizeSnapshot& snapshot, double reservedLogicalWidth) {
    const auto viewport = renderer::Renderer::calculate_scene_viewport(
        snapshot.logical, snapshot.framebuffer, reservedLogicalWidth);
    const int edgeInset = std::max(16, snapshot.framebuffer.first / 64);
    return {{viewport.framebuffer.x + viewport.framebuffer.width / 2,
             viewport.framebuffer.y + viewport.framebuffer.height / 2},
            {viewport.framebuffer.x + viewport.framebuffer.width / 4,
             viewport.framebuffer.y + viewport.framebuffer.height / 4},
            {snapshot.framebuffer.first - edgeInset, snapshot.framebuffer.second / 4}};
}

testing::AssertionResult pixel_near(SizePair coordinate,
                                    const std::array<std::uint8_t, 4>& actual,
                                    const std::array<int, 4>& expected) {
    constexpr int tolerance = 5;
    for (std::size_t channel = 0; channel < actual.size(); ++channel) {
        if (std::abs(static_cast<int>(actual[channel]) - expected[channel]) > tolerance) {
            return testing::AssertionFailure()
                   << "pixel at (" << coordinate.first << ", " << coordinate.second << ") was RGBA("
                   << static_cast<int>(actual[0]) << ", " << static_cast<int>(actual[1]) << ", "
                   << static_cast<int>(actual[2]) << ", " << static_cast<int>(actual[3])
                   << "), expected RGBA(" << expected[0] << ", " << expected[1] << ", " << expected[2] << ", "
                   << expected[3] << ") within " << tolerance;
        }
    }
    return testing::AssertionSuccess();
}

bool coordinate_in_bounds(SizePair coordinate, SizePair framebuffer) {
    return coordinate.first >= 0 && coordinate.second >= 0 && coordinate.first < framebuffer.first &&
           coordinate.second < framebuffer.second;
}

SteadyResult pump_events_while_steady(renderer::Renderer& renderer, const SizeSnapshot& expected) {
    using Clock = std::chrono::steady_clock;
    constexpr auto duration = std::chrono::milliseconds(16);
    constexpr auto maximumWait = std::chrono::milliseconds(5);
    const auto deadline = Clock::now() + duration;

    SteadyResult result{true, get_size_snapshot(renderer)};
    if (result.last.logical != expected.logical || result.last.framebuffer != expected.framebuffer) {
        result.stable = false;
        return result;
    }
    while (Clock::now() < deadline) {
        const auto remaining = deadline - Clock::now();
        const auto waitDuration = std::min(remaining, Clock::duration(maximumWait));
        glfwWaitEventsTimeout(std::chrono::duration<double>(waitDuration).count());
        result.last = get_size_snapshot(renderer);
        if (result.last.logical != expected.logical || result.last.framebuffer != expected.framebuffer) {
            result.stable = false;
            return result;
        }
    }
    return result;
}

class RendererPresentationTest : public ::testing::Test {
  protected:
    static std::unique_ptr<renderer::Renderer> create_renderer(int samples, int width = 512, int height = 256) {
        renderer::WindowSettings settings;
        settings.title = "plinth renderer presentation test";
        settings.width = static_cast<std::uint32_t>(width);
        settings.height = static_cast<std::uint32_t>(height);
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

    static FrameResult render_and_read_frame(renderer::Renderer& renderer,
                                             const renderer::ClearColor& clearColor,
                                             const ReadCoordinates& coordinates) {
        FrameResult result;
        result.beforeBeginError = glGetError();
        renderer.begin_frame(clearColor);
        glGetIntegerv(GL_SAMPLE_BUFFERS, &result.sampleBuffers);
        glGetIntegerv(GL_SAMPLES, &result.activeSamples);
        result.beginError = glGetError();
        renderer.draw();
        result.drawError = glGetError();
        renderer.end_frame();
        result.endError = glGetError();
        glFinish();
        result.finishError = glGetError();
        result.afterFinish = get_size_snapshot(renderer);
        glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &result.readFramebufferBinding);
        result.bindingError = glGetError();
        result.centerPixel = read_front_pixel(coordinates.center.first, coordinates.center.second);
        result.backgroundPixel = read_front_pixel(coordinates.background.first, coordinates.background.second);
        result.edgePixel = read_front_pixel(coordinates.edge.first, coordinates.edge.second);
        result.readError = glGetError();
        result.afterRead = get_size_snapshot(renderer);
        return result;
    }

    static void verify_stable_resize_case(int samples) {
        constexpr SizePair maximumLogical{768, 384};
        auto instance = create_renderer(samples, maximumLogical.first, maximumLogical.second);
        ASSERT_NE(nullptr, instance);
        GLboolean doubleBuffered = GL_TRUE;
        glGetBooleanv(GL_DOUBLEBUFFER, &doubleBuffered);
        ASSERT_EQ(GL_FALSE, doubleBuffered) << "Front-buffer readback requires a single-buffered context";
        ASSERT_FALSE(instance->window().is_srgb_capable())
            << "Pixel assertions require a non-sRGB default framebuffer";

        GLint maxSamples = 0;
        glGetIntegerv(GL_MAX_SAMPLES, &maxSamples);
        if (samples > 1) {
            ASSERT_GE(maxSamples, 2) << "RendererPresentationTest requires a supported OpenGL 4.1 context with at "
                                        "least 2 samples";
        }

        constexpr std::array<float, 3> vertices{0.0F, 0.0F, 0.0F};
        constexpr std::array<float, 4> colors{0.0F, 0.0F, 1.0F, 1.0F};
        constexpr std::array<std::uint32_t, 1> indices{0U};
        const renderer::DrawableHandle handle = instance->add_point_drawable(vertices, colors, indices, 64.0F);
        ASSERT_TRUE(handle.is_valid());

        const auto imgui = instance->get_imgui().lock();
        ASSERT_NE(nullptr, imgui) << "Renderer ImGui overlay is unavailable";
        const double reservedLogicalWidth = static_cast<double>(imgui->get_reserved_control_panel_width());

        const auto verify_frame = [&](const SizeSnapshot& snapshot,
                                      const renderer::ClearColor& clearColor,
                                      const std::array<int, 4>& expectedBackground) {
            ASSERT_EQ(snapshot.logical, instance->window().get_window_size())
                << "logical size changed before begin_frame; expected " << describe_sizes(snapshot);
            ASSERT_EQ(snapshot.framebuffer, instance->window().get_framebuffer_size())
                << "framebuffer size changed before begin_frame; expected " << describe_sizes(snapshot);

            const ReadCoordinates coordinates = calculate_read_coordinates(snapshot, reservedLogicalWidth);
            ASSERT_TRUE(coordinate_in_bounds(coordinates.center, snapshot.framebuffer))
                << "center coordinate is outside " << describe_sizes(snapshot);
            ASSERT_TRUE(coordinate_in_bounds(coordinates.background, snapshot.framebuffer))
                << "background coordinate is outside " << describe_sizes(snapshot);
            ASSERT_TRUE(coordinate_in_bounds(coordinates.edge, snapshot.framebuffer))
                << "edge coordinate is outside " << describe_sizes(snapshot);
            ASSERT_TRUE(std::abs(coordinates.background.first - coordinates.center.first) > 32 ||
                        std::abs(coordinates.background.second - coordinates.center.second) > 32)
                << "background coordinate is not separated from center: center=(" << coordinates.center.first << ", "
                << coordinates.center.second << "), background=(" << coordinates.background.first << ", "
                << coordinates.background.second << ')';

            const FrameResult frame = render_and_read_frame(*instance, clearColor, coordinates);
            ASSERT_EQ(GL_NO_ERROR, frame.beforeBeginError) << "GL error before begin_frame";
            ASSERT_EQ(GL_NO_ERROR, frame.beginError) << "GL error during begin_frame/sample query";
            ASSERT_EQ(GL_NO_ERROR, frame.drawError) << "GL error during draw";
            ASSERT_EQ(GL_NO_ERROR, frame.endError) << "GL error during end_frame";
            ASSERT_EQ(GL_NO_ERROR, frame.finishError) << "GL error during glFinish";
            ASSERT_EQ(GL_NO_ERROR, frame.bindingError) << "GL error querying read framebuffer binding";
            ASSERT_EQ(GL_NO_ERROR, frame.readError) << "GL error during front-buffer reads";
            if (samples == 1) {
                ASSERT_EQ(0, frame.sampleBuffers)
                    << "Single-sample scene framebuffer must not provide a multisample buffer";
                ASSERT_EQ(0, frame.activeSamples)
                    << "Single-sample scene framebuffer must report zero active samples";
            } else {
                ASSERT_GE(frame.sampleBuffers, 1)
                    << "Renderer scene framebuffer must provide a multisample buffer";
                ASSERT_EQ(std::min(samples, maxSamples), frame.activeSamples)
                    << "Renderer scene framebuffer must honor the requested sample count subject to the GL limit";
            }
            ASSERT_EQ(0, frame.readFramebufferBinding) << "Front-buffer reads require the default read framebuffer";
            ASSERT_EQ(snapshot.logical, frame.afterFinish.logical)
                << "logical size changed after glFinish; expected " << describe_sizes(snapshot) << ", got "
                << describe_sizes(frame.afterFinish);
            ASSERT_EQ(snapshot.framebuffer, frame.afterFinish.framebuffer)
                << "framebuffer size changed after glFinish; expected " << describe_sizes(snapshot) << ", got "
                << describe_sizes(frame.afterFinish);
            ASSERT_EQ(snapshot.logical, frame.afterRead.logical)
                << "logical size changed after reads; expected " << describe_sizes(snapshot) << ", got "
                << describe_sizes(frame.afterRead);
            ASSERT_EQ(snapshot.framebuffer, frame.afterRead.framebuffer)
                << "framebuffer size changed after reads; expected " << describe_sizes(snapshot) << ", got "
                << describe_sizes(frame.afterRead);
            ASSERT_TRUE(pixel_near(coordinates.center, frame.centerPixel, {0, 0, 255, 255}));
            ASSERT_TRUE(pixel_near(coordinates.background, frame.backgroundPixel, expectedBackground));
            ASSERT_TRUE(pixel_near(coordinates.edge, frame.edgePixel, expectedBackground));
        };

        const SizeSnapshot initial = get_size_snapshot(*instance);
        ASSERT_EQ(maximumLogical, initial.logical)
            << "renderer was not created at the maximum logical extent; got " << describe_sizes(initial);
        verify_frame(initial, {1.0F, 0.0F, 0.0F, 1.0F}, {255, 0, 0, 255});
        ASSERT_FALSE(HasFatalFailure());

        auto* window = static_cast<GLFWwindow*>(instance->window().get_native_handle());
        ASSERT_NE(nullptr, window);
        constexpr SizePair shrunkenLogical{512, 256};
        const ResizeResult shrink = wait_for_stable_resize(*instance, window, shrunkenLogical, initial);
        ASSERT_TRUE(shrink.stable) << describe_resize(shrink, shrunkenLogical, initial);
        const SizeSnapshot shrunken{shrink.finalLogical, shrink.finalFramebuffer};

        // Realize llvmpipe's resized single-buffered default drawable before the verified renderer frame.
        glfwSwapBuffers(window);
        verify_frame(shrunken, {1.0F, 0.0F, 1.0F, 1.0F}, {255, 0, 255, 255});
        ASSERT_FALSE(HasFatalFailure());

        const SteadyResult steady = pump_events_while_steady(*instance, shrunken);
        ASSERT_TRUE(steady.stable) << "size changed during 16ms steady event pump; expected "
                                  << describe_sizes(shrunken) << ", got " << describe_sizes(steady.last);

        const ResizeResult regrow = wait_for_stable_resize(*instance, window, maximumLogical, shrunken);
        ASSERT_TRUE(regrow.stable) << describe_resize(regrow, maximumLogical, shrunken);
        const SizeSnapshot regrown{regrow.finalLogical, regrow.finalFramebuffer};

        glfwSwapBuffers(window);
        verify_frame(regrown, {1.0F, 1.0F, 0.0F, 1.0F}, {255, 255, 0, 255});
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

TEST_F(RendererPresentationTest, PresentsFreshSingleSampleOutputAfterStableFramebufferResize) {
    verify_stable_resize_case(1);
}

TEST_F(RendererPresentationTest, PresentsFreshMultisampledOutputAfterStableFramebufferResize) {
    verify_stable_resize_case(4);
}

} // namespace
