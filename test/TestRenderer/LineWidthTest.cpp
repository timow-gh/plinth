// Instanced thick-line width test: draws the same vertical line at several widths into a hidden,
// deterministic (zero-MSAA) capture window and verifies that a wider line covers strictly more
// horizontal pixels. This exercises the instanced quad-expansion path in LineDrawable, whose whole
// purpose is reliable thick lines in the core profile (where glLineWidth is unreliable).

#include "CaptureFixture.hpp"
#include "OpenGL/Drawable/LineDrawable.hpp"
#include "OpenGL/FrameState.hpp"
#include "OpenGL/OpenGL.hpp"
#include "OpenGL/Programs/LineProgram.hpp"

#include <linal/hmat.hpp>
#include <linal/vec.hpp>

#include <GLFW/glfw3.h>
#include <cstdint>
#include <gtest/gtest.h>
#include <memory>
#include <optional>
#include <vector>

namespace {

constexpr int captureWidth = 64;
constexpr int captureHeight = 64;

class LineWidthTest : public ::testing::Test {
  protected:
    static void SetUpTestSuite() { ASSERT_EQ(GLFW_TRUE, glfwInit()); }
    static void TearDownTestSuite() { glfwTerminate(); }

    void SetUp() override {
        m_captureWindow = std::make_unique<plinth_test::CaptureWindow>(captureWidth, captureHeight);
        ASSERT_TRUE(m_captureWindow->is_valid());
    }
    void TearDown() override { m_captureWindow.reset(); }

    std::unique_ptr<plinth_test::CaptureWindow> m_captureWindow;
};

// Counts how many horizontal pixels in the middle scanline are lit (clearly non-background). The
// scene is a red vertical line on a black background, so a lit pixel has a meaningful red channel.
std::size_t count_lit_columns_in_middle_row(const plinth_test::CapturedImage& image) {
    const std::uint32_t midY = image.height / 2U;
    const std::size_t rowBytes = static_cast<std::size_t>(image.width) * 4U;
    const std::size_t rowStart = static_cast<std::size_t>(midY) * rowBytes;
    std::size_t litColumns = 0;
    for (std::uint32_t x = 0; x < image.width; ++x) {
        const std::uint8_t red = image.rgba[rowStart + static_cast<std::size_t>(x) * 4U + 0U];
        if (red > 32U) {
            ++litColumns;
        }
    }
    return litColumns;
}

// Draws a red vertical line (from bottom to top through the center) at the given pixel width and
// returns the number of lit columns across the middle scanline.
std::size_t render_vertical_line_width(opengl::LineProgram& program, float lineWidth) {
    const opengl::ClearColor clearColor{0.0F, 0.0F, 0.0F, 1.0F};
    const opengl::ViewportRect viewport{0, 0, captureWidth, captureHeight};
    opengl::begin_frame(clearColor, viewport, false); // linear framebuffer -> pixel-exact read-back

    // A vertical segment through the center under identity MVP. Thickness spreads it horizontally,
    // so the middle-row lit-column count is a direct proxy for the rendered width.
    const std::vector<float> vertices = {0.0F, -0.9F, 0.0F, 0.0F, 0.9F, 0.0F};
    const std::vector<float> colors = {1.0F, 0.0F, 0.0F, 1.0F, 1.0F, 0.0F, 0.0F, 1.0F};
    const std::vector<std::uint32_t> indices = {0U, 1U};

    std::optional<opengl::LineDrawable> drawable = opengl::make_line_drawable(program,
                                                                              vertices,
                                                                              3,
                                                                              indices,
                                                                              colors,
                                                                              4,
                                                                              opengl::LineType::lines(),
                                                                              lineWidth,
                                                                              0.0F,
                                                                              opengl::BufferAccessPattern::Static);
    EXPECT_TRUE(drawable.has_value());

    const linal::hmatf identity = linal::hmatf::identity();
    const linal::float2 viewportSize{static_cast<float>(captureWidth), static_cast<float>(captureHeight)};
    drawable->draw_opaque(identity, identity, viewportSize);

    const plinth_test::CapturedImage image = plinth_test::read_back_rgba(captureWidth, captureHeight);
    return count_lit_columns_in_middle_row(image);
}

} // namespace

TEST_F(LineWidthTest, WiderLinesCoverMoreHorizontalPixels) {
    opengl::LineProgram program = opengl::make_line_program();
    ASSERT_TRUE(program.is_valid());

    const std::size_t thin = render_vertical_line_width(program, 2.0F);
    const std::size_t medium = render_vertical_line_width(program, 8.0F);
    const std::size_t thick = render_vertical_line_width(program, 20.0F);

    // Every width must render something.
    EXPECT_GT(thin, 0U);
    EXPECT_GT(medium, 0U);
    EXPECT_GT(thick, 0U);

    // Increasing the requested width must strictly increase the pixel footprint. This is the key
    // guarantee the instanced path provides that raw glLineWidth cannot in the core profile.
    EXPECT_LT(thin, medium);
    EXPECT_LT(medium, thick);

    // A 20px line, centered, should cover on the order of its width in columns. Allow generous
    // slack for coverage/anti-aliasing at the quad edges while still catching a broken width.
    EXPECT_GE(thick, 12U);
    EXPECT_LE(thick, 28U);
}
