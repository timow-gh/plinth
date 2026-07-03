#include <GeoQik/GeoQik.hpp>
#include <Renderer/WindowSettings.hpp>
#include <gtest/gtest.h>

using namespace renderer;

TEST(WindowSettingsTest, InternalDefaultsMatchExternalApiDefaults) {
    geoqik_window_settings_t externalSettings;
    geoqik_init_default_window_settings(&externalSettings);

    const WindowSettings internalSettings;

    EXPECT_STREQ(externalSettings.title, internalSettings.title);
    EXPECT_EQ(externalSettings.width, internalSettings.width);
    EXPECT_EQ(externalSettings.height, internalSettings.height);
    EXPECT_EQ(externalSettings.red_bits, internalSettings.red_bits);
    EXPECT_EQ(externalSettings.green_bits, internalSettings.green_bits);
    EXPECT_EQ(externalSettings.blue_bits, internalSettings.blue_bits);
    EXPECT_EQ(externalSettings.alpha_bits, internalSettings.alpha_bits);
    EXPECT_EQ(externalSettings.depth_bits, internalSettings.depth_bits);
    EXPECT_EQ(externalSettings.stencil_bits, internalSettings.stencil_bits);
    EXPECT_EQ(externalSettings.accum_red_bits, internalSettings.accum_red_bits);
    EXPECT_EQ(externalSettings.accum_green_bits, internalSettings.accum_green_bits);
    EXPECT_EQ(externalSettings.accum_blue_bits, internalSettings.accum_blue_bits);
    EXPECT_EQ(externalSettings.accum_alpha_bits, internalSettings.accum_alpha_bits);
    EXPECT_EQ(externalSettings.aux_buffers, internalSettings.aux_buffers);
    EXPECT_EQ(externalSettings.samples, internalSettings.samples);
    EXPECT_EQ(externalSettings.refresh_rate, internalSettings.refresh_rate);
    EXPECT_EQ(externalSettings.stereo != 0, internalSettings.stereo);
    EXPECT_EQ(externalSettings.srgb_capable != 0, internalSettings.srgb_capable);
    EXPECT_EQ(externalSettings.double_buffer != 0, internalSettings.double_buffer);
    EXPECT_EQ(externalSettings.resizable != 0, internalSettings.resizable);
    EXPECT_EQ(externalSettings.visible != 0, internalSettings.visible);
    EXPECT_EQ(externalSettings.decorated != 0, internalSettings.decorated);
    EXPECT_EQ(externalSettings.focused != 0, internalSettings.focused);
    EXPECT_EQ(externalSettings.auto_iconify != 0, internalSettings.auto_iconify);
    EXPECT_EQ(externalSettings.floating != 0, internalSettings.floating);
    EXPECT_EQ(externalSettings.maximized != 0, internalSettings.maximized);
    EXPECT_EQ(externalSettings.center_cursor != 0, internalSettings.center_cursor);
    EXPECT_EQ(externalSettings.transparent_framebuffer != 0, internalSettings.transparent_framebuffer);
    EXPECT_EQ(externalSettings.focus_on_show != 0, internalSettings.focus_on_show);
    EXPECT_EQ(externalSettings.scale_to_monitor != 0, internalSettings.scale_to_monitor);
}
