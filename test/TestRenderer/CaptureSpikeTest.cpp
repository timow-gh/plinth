// Spike PoC for plan 007 (offscreen capture / visual-regression feasibility).
//
// This test is deliberately test-only, throwaway-quality: it proves that a hidden GLFW
// window's default framebuffer can be read back deterministically via glReadPixels,
// without adding any production capture API. See plans/notes/007-capture-design.md for
// the full design write-up this test exists to validate.
#include <GLFW/glfw3.h>
#include <OpenGL/Drawable/PointDrawable.hpp>
#include <OpenGL/FrameState.hpp>
#include <OpenGL/OpenGL.hpp>
#include <array>
#include <cstdint>
#include <gtest/gtest.h>
#include <linal/hmat.hpp>
#include <optional>
#include <vector>

namespace {

constexpr int captureWidth = 64;
constexpr int captureHeight = 64;

void* load_glfw_proc(const char* procName) {
    // GLAD v1 expects void* while GLFW returns an opaque function pointer.
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    return reinterpret_cast<void*>(glfwGetProcAddress(procName));
}

// Local, test-only read-back helper. Intentionally NOT part of the public API -
// see "Explicit non-goals" in plan 007. Returns top-down RGBA rows (flips the
// bottom-up rows glReadPixels produces), per the orientation decision in the
// design note (Question 3).
struct CapturedImage {
    std::uint32_t width{0};
    std::uint32_t height{0};
    std::vector<std::uint8_t> rgba;
};

CapturedImage read_back_rgba(int width, int height) {
    CapturedImage image;
    image.width = static_cast<std::uint32_t>(width);
    image.height = static_cast<std::uint32_t>(height);
    image.rgba.resize(static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4U);

    // Default framebuffer read-back after draw(), before swap_buffers() (Question 2):
    // with double buffering the back buffer holds the just-rendered frame, so
    // GL_BACK is the correct source (GL_FRONT would read the previously-presented frame).
    glReadBuffer(GL_BACK);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);

    std::vector<std::uint8_t> bottomUp(image.rgba.size());
    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, bottomUp.data());

    const std::size_t rowBytes = static_cast<std::size_t>(width) * 4U;
    for (int row = 0; row < height; ++row) {
        const std::size_t srcRow = static_cast<std::size_t>(height - 1 - row) * rowBytes;
        const std::size_t dstRow = static_cast<std::size_t>(row) * rowBytes;
        std::copy_n(bottomUp.data() + srcRow, rowBytes, image.rgba.data() + dstRow);
    }
    return image;
}

std::array<std::uint8_t, 4> pixel_at(const CapturedImage& image, std::uint32_t x, std::uint32_t y) {
    const std::size_t index = (static_cast<std::size_t>(y) * image.width + x) * 4U;
    return {image.rgba[index], image.rgba[index + 1], image.rgba[index + 2], image.rgba[index + 3]};
}

void expect_color_near(const std::array<std::uint8_t, 4>& actual,
                       const std::array<std::uint8_t, 4>& expected,
                       int tolerance) {
    for (std::size_t channel = 0; channel < 4; ++channel) {
        EXPECT_NEAR(static_cast<int>(actual[channel]), static_cast<int>(expected[channel]), tolerance)
            << "channel " << channel;
    }
}

class CaptureSpikeTest : public ::testing::Test {
  protected:
    static void SetUpTestSuite() { ASSERT_EQ(GLFW_TRUE, glfwInit()); }

    static void TearDownTestSuite() { glfwTerminate(); }

    void SetUp() override {
        glfwDefaultWindowHints();
        // Hidden window (WindowSettings::visible=false path) - Question 1 of the spike.
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        // No GLFW_SAMPLES hint set -> 0 samples / MSAA disabled, per the determinism
        // recommendation (Question 5): golden-pixel tests should not fight AA.

        m_window = glfwCreateWindow(captureWidth, captureHeight, "plinth capture spike test", nullptr, nullptr);
        ASSERT_NE(nullptr, m_window);
        glfwMakeContextCurrent(m_window);
        ASSERT_NE(0, gladLoadGLLoader(load_glfw_proc));
    }

    void TearDown() override {
        if (m_window != nullptr) {
            glfwDestroyWindow(m_window);
            m_window = nullptr;
        }
    }

    GLFWwindow* m_window{nullptr};
};

} // namespace

TEST_F(CaptureSpikeTest, HiddenWindowReadBackMatchesClearColorAndPrimitiveColor) {
    // Known clear color: opaque dark blue.
    const opengl::ClearColor clearColor{0.0F, 0.0F, 0.5F, 1.0F};
    const opengl::ViewportRect viewport{0, 0, captureWidth, captureHeight};
    opengl::begin_frame(clearColor, viewport);

    // One large point at the origin -> projects to the center of the viewport under
    // an identity MVP (mirrors the reasoning in DrawableTest.cpp / example_renderer_standalone.cpp).
    opengl::PointProgram program = opengl::make_point_program();
    const std::array<float, 3> vertices{0.0F, 0.0F, 0.0F};
    const std::array<float, 4> knownColor{1.0F, 0.0F, 0.0F, 1.0F}; // opaque red
    const std::array<std::uint32_t, 1> indices{0U};
    constexpr float largePointSize = 40.0F; // covers the center so read-back isn't 1-pixel-fragile.

    std::optional<opengl::PointDrawable> drawable = opengl::make_point_drawable(program,
                                                                                vertices,
                                                                                3,
                                                                                knownColor,
                                                                                4,
                                                                                indices,
                                                                                largePointSize,
                                                                                opengl::BufferAccessPattern::STATIC_DRAW);
    ASSERT_TRUE(drawable.has_value());

    linal::hmatf identity = linal::hmatf::identity();
    drawable->draw_opaque(identity);

    // Read-back happens after draw(), before swap_buffers() (Question 2) - there is
    // no swap here at all since the PoC never needs to present the hidden window.
    const CapturedImage image = read_back_rgba(captureWidth, captureHeight);
    ASSERT_EQ(static_cast<std::uint32_t>(captureWidth), image.width);
    ASSERT_EQ(static_cast<std::uint32_t>(captureHeight), image.height);

    constexpr int tolerance = 8; // absorbs point-sprite edge blending; see design note Question 5.

    // Corner pixel (top-left, in the flipped top-down image) should be the clear color.
    const std::array<std::uint8_t, 4> cornerPixel = pixel_at(image, 0, 0);
    expect_color_near(cornerPixel, {0U, 0U, 127U, 255U}, tolerance);

    // Center pixel should be the primitive's known color.
    const std::array<std::uint8_t, 4> centerPixel =
        pixel_at(image, static_cast<std::uint32_t>(captureWidth) / 2U, static_cast<std::uint32_t>(captureHeight) / 2U);
    expect_color_near(centerPixel, {255U, 0U, 0U, 255U}, tolerance);
}
