#include <plinth/Viewport.hpp>
#include <cmath>
#include <gtest/gtest.h>

namespace {

constexpr double tolerance = 1.0e-12;

} // namespace

TEST(ViewportTest, ConstructorStoresDimensions) {
    const renderer::Viewport vp{10, 20, 800, 600};
    EXPECT_EQ(vp.get_xpos(), 10u);
    EXPECT_EQ(vp.get_ypos(), 20u);
    EXPECT_EQ(vp.get_width(), 800u);
    EXPECT_EQ(vp.get_height(), 600u);
}

TEST(ViewportTest, DefaultViewportHasPositiveFiniteAspectRatio) {
    const renderer::Viewport vp; // 800x600 defaults
    const double aspect = vp.get_aspect_ratio();
    EXPECT_GT(aspect, 0.0);
    EXPECT_TRUE(std::isfinite(aspect));
    EXPECT_NEAR(aspect, 800.0 / 600.0, tolerance);
}

TEST(ViewportTest, SetWidthUpdatesAspectRatio) {
    renderer::Viewport vp{0, 0, 800, 600};
    vp.set_width(1024);
    EXPECT_EQ(vp.get_width(), 1024u);
    EXPECT_NEAR(vp.get_aspect_ratio(), 1024.0 / 600.0, tolerance);
}

TEST(ViewportTest, SetHeightUpdatesAspectRatio) {
    renderer::Viewport vp{0, 0, 800, 600};
    vp.set_height(400);
    EXPECT_EQ(vp.get_height(), 400u);
    EXPECT_NEAR(vp.get_aspect_ratio(), 800.0 / 400.0, tolerance);
}
