#include <array>
#include <cmath>
#include <gtest/gtest.h>
#include <plinth/Color.hpp>
#include <plinth/ColorConversion.hpp>
#include <vector>

namespace {

constexpr float tolerance = 1.0e-5F;

} // namespace

TEST(ColorConversionTest, ScalarFixedPoints) {
    // 0 and 1 are fixed points of the sRGB curve, which is why pure red (1,0,0)
    // is unaffected by linearization.
    EXPECT_NEAR(renderer::srgb_to_linear(0.0F), 0.0F, tolerance);
    EXPECT_NEAR(renderer::srgb_to_linear(1.0F), 1.0F, tolerance);
    EXPECT_NEAR(renderer::linear_to_srgb(0.0F), 0.0F, tolerance);
    EXPECT_NEAR(renderer::linear_to_srgb(1.0F), 1.0F, tolerance);
}

TEST(ColorConversionTest, ScalarKnownMidTone) {
    // Mid-grey sRGB 0.5 linearizes to ~0.214.
    EXPECT_NEAR(renderer::srgb_to_linear(0.5F), 0.21404F, 1.0e-4F);
}

TEST(ColorConversionTest, ScalarPiecewiseBoundaryContinuity) {
    // The two branches must agree at the sRGB->linear threshold (0.04045).
    constexpr float threshold = 0.04045F;
    const float below = renderer::srgb_to_linear(std::nextafter(threshold, 0.0F));
    const float above = renderer::srgb_to_linear(std::nextafter(threshold, 1.0F));
    EXPECT_NEAR(below, above, 1.0e-4F);
}

TEST(ColorConversionTest, ScalarRoundTrip) {
    for (const float x: {0.0F, 0.25F, 0.5F, 0.75F, 1.0F}) {
        EXPECT_NEAR(renderer::linear_to_srgb(renderer::srgb_to_linear(x)), x, tolerance);
    }
}

TEST(ColorConversionTest, ColorConvertsRgbAndPreservesAlpha) {
    const renderer::Color srgb{0.5F, 0.0F, 1.0F, 0.42F};
    const renderer::Color linear = renderer::srgb_to_linear(srgb);
    EXPECT_NEAR(linear[0], renderer::srgb_to_linear(0.5F), tolerance);
    EXPECT_NEAR(linear[1], 0.0F, tolerance);
    EXPECT_NEAR(linear[2], 1.0F, tolerance);
    EXPECT_EQ(linear[3], 0.42F); // alpha passed through unchanged
}

TEST(ColorConversionTest, PackedArrayLeavesAlphaUntouched) {
    std::vector<float> packed{0.5F, 0.0F, 1.0F, 0.25F, 1.0F, 0.5F, 0.0F, 0.9F};
    renderer::srgb_to_linear_inplace(packed);

    EXPECT_NEAR(packed[0], renderer::srgb_to_linear(0.5F), tolerance);
    EXPECT_NEAR(packed[1], 0.0F, tolerance);
    EXPECT_NEAR(packed[2], 1.0F, tolerance);
    EXPECT_EQ(packed[3], 0.25F); // alpha untouched

    EXPECT_NEAR(packed[4], 1.0F, tolerance);
    EXPECT_NEAR(packed[5], renderer::srgb_to_linear(0.5F), tolerance);
    EXPECT_NEAR(packed[6], 0.0F, tolerance);
    EXPECT_EQ(packed[7], 0.9F); // alpha untouched
}

TEST(ColorConversionTest, PackedArrayCopyMatchesInplace) {
    const std::array<float, 4> src{0.5F, 0.25F, 0.75F, 0.5F};
    const std::vector<float> copy = renderer::srgb_to_linear_copy(src);
    ASSERT_EQ(copy.size(), src.size());
    EXPECT_NEAR(copy[0], renderer::srgb_to_linear(0.5F), tolerance);
    EXPECT_NEAR(copy[1], renderer::srgb_to_linear(0.25F), tolerance);
    EXPECT_NEAR(copy[2], renderer::srgb_to_linear(0.75F), tolerance);
    EXPECT_EQ(copy[3], 0.5F); // alpha untouched
}
