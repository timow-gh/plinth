// Golden-image regression test: draws a known scene into a hidden, deterministic capture
// window (CaptureFixture.hpp) and compares the read-back pixels against a checked-in PNG.
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include "CaptureFixture.hpp"
#include <GLFW/glfw3.h>
#include <OpenGL/Drawable/PointDrawable.hpp>
#include <OpenGL/FrameState.hpp>
#include <OpenGL/OpenGL.hpp>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <gtest/gtest.h>
#include <linal/hmat.hpp>
#include <memory>
#include <optional>

namespace {

constexpr int captureWidth = 64;
constexpr int captureHeight = 64;

class GoldenImageTest : public ::testing::Test {
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

} // namespace

TEST_F(GoldenImageTest, PointSceneMatchesGolden) {
    // Known clear color: opaque dark blue.
    const opengl::ClearColor clearColor{0.0F, 0.0F, 0.5F, 1.0F};
    const opengl::ViewportRect viewport{0, 0, captureWidth, captureHeight};
    opengl::begin_frame(clearColor, viewport);

    // One large point at the origin -> projects to the center of the viewport under
    // an identity MVP.
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
    drawable->draw_opaque(identity, identity);

    // Read-back happens after draw(), before any swap.
    const plinth_test::CapturedImage image = plinth_test::read_back_rgba(captureWidth, captureHeight);
    ASSERT_EQ(static_cast<std::uint32_t>(captureWidth), image.width);
    ASSERT_EQ(static_cast<std::uint32_t>(captureHeight), image.height);

    const std::filesystem::path goldenPath = std::filesystem::path{PLINTH_GOLDEN_DIR} / "point_scene.png";

    const char* updateGoldens = std::getenv("PLINTH_UPDATE_GOLDENS");
    if (updateGoldens != nullptr && updateGoldens[0] != '\0') {
        const int writeResult = stbi_write_png(goldenPath.string().c_str(),
                                               static_cast<int>(image.width),
                                               static_cast<int>(image.height),
                                               4,
                                               image.rgba.data(),
                                               static_cast<int>(image.width) * 4);
        ASSERT_NE(0, writeResult) << "Failed to write golden image to " << goldenPath.string();
        GTEST_SKIP() << "PLINTH_UPDATE_GOLDENS set: (re)wrote golden image at " << goldenPath.string();
    }

    int width = 0;
    int height = 0;
    int channels = 0;
    unsigned char* decoded = stbi_load(goldenPath.string().c_str(), &width, &height, &channels, 4);
    ASSERT_NE(nullptr, decoded) << "Failed to load golden image at " << goldenPath.string()
                                << ". If it doesn't exist yet, rerun with PLINTH_UPDATE_GOLDENS=1 to generate it.";

    ASSERT_EQ(captureWidth, width);
    ASSERT_EQ(captureHeight, height);

    constexpr int tolerance = 8; // absorbs point-sprite edge blending.

    int maxDelta = 0;
    bool mismatchFound = false;
    std::uint32_t mismatchX = 0;
    std::uint32_t mismatchY = 0;

    const std::size_t rowBytes = static_cast<std::size_t>(width) * 4U;
    for (std::uint32_t y = 0; y < image.height; ++y) {
        for (std::uint32_t x = 0; x < image.width; ++x) {
            const std::size_t index = static_cast<std::size_t>(y) * rowBytes + static_cast<std::size_t>(x) * 4U;
            for (int channel = 0; channel < 4; ++channel) {
                const int actual = static_cast<int>(image.rgba[index + static_cast<std::size_t>(channel)]);
                const int expected = static_cast<int>(decoded[index + static_cast<std::size_t>(channel)]);
                const int delta = std::abs(actual - expected);
                if (delta > maxDelta) {
                    maxDelta = delta;
                }
                if (delta > tolerance && !mismatchFound) {
                    mismatchFound = true;
                    mismatchX = x;
                    mismatchY = y;
                }
            }
        }
    }

    stbi_image_free(decoded);

    EXPECT_FALSE(mismatchFound) << "Golden image mismatch: max per-channel delta = " << maxDelta
                                 << ", first differing pixel at (" << mismatchX << ", " << mismatchY << ")";
}
