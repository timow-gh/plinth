#include <plinth/InputState.hpp>
#include <gtest/gtest.h>

using namespace renderer;

TEST(InputStateTest, ValueWrappersExposeValidityAndOrdering) {
    const Scancode defaultScancode;
    const Scancode scancode{7};

    EXPECT_FALSE(defaultScancode.is_valid());
    EXPECT_TRUE(scancode.is_valid());
    EXPECT_EQ(7, scancode.get_value());
    EXPECT_LT(defaultScancode, scancode);

    const renderer::Codepoint defaultCodepoint;
    const renderer::Codepoint codepoint{65};

    EXPECT_FALSE(defaultCodepoint.is_valid());
    EXPECT_TRUE(codepoint.is_valid());
    EXPECT_EQ(65U, codepoint.get_value());
    EXPECT_LT(defaultCodepoint, codepoint);
}
