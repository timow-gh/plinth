#include <GLFW/glfw3.h>
#include <OpenGL/Framebuffer.hpp>
#include <OpenGL/OpenGL.hpp>
#include <OpenGL/PostProcessingPass.hpp>
#include <array>
#include <cmath>
#include <gtest/gtest.h>

namespace {

void* load_glfw_proc(const char* procName) {
    return reinterpret_cast<void*>(glfwGetProcAddress(procName));
}

class PostProcessingPassTest : public ::testing::Test {
  protected:
    static void SetUpTestSuite() { ASSERT_EQ(GLFW_TRUE, glfwInit()); }

    static void TearDownTestSuite() { glfwTerminate(); }

    void SetUp() override {
        glfwDefaultWindowHints();
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        m_window = glfwCreateWindow(64, 64, "plinth post-processing pass test", nullptr, nullptr);
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

TEST_F(PostProcessingPassTest, CreatesAndIsValid) {
    auto pass = opengl::PostProcessingPass::create();
    ASSERT_TRUE(pass.has_value());
    EXPECT_TRUE(pass->is_valid());
}

TEST_F(PostProcessingPassTest, ProcessesHdrToLdr) {
    opengl::Framebuffer::HdrConfig hdrConfig{16, 16, 1, true};
    auto hdrFb = opengl::Framebuffer::create_hdr(hdrConfig);
    ASSERT_TRUE(hdrFb.has_value());

    auto ldrFb = opengl::Framebuffer::create_ldr_intermediate(16, 16);
    ASSERT_TRUE(ldrFb.has_value());

    hdrFb->bind();
    glClearColor(1.0F, 0.0F, 0.0F, 1.0F);
    glClearDepth(1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    auto pass = opengl::PostProcessingPass::create();
    ASSERT_TRUE(pass.has_value());
    ASSERT_TRUE(pass->is_valid());

    float identity[16] = {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    };
    pass->set_inv_projection(identity);
    pass->set_fog_enabled(false);
    pass->set_exposure_stops(0.0f);
    pass->set_tone_map_mode(0);
    pass->set_visualization_mode(0);
    pass->set_hdr_display_max(10.0f);
    pass->set_grayscale(false);

    ldrFb->bind();
    glViewport(3, 4, 5, 6);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    pass->process(hdrFb->get_color_texture(), hdrFb->get_depth_texture(), 16, 16);

    std::array<GLint, 4> viewport{};
    glGetIntegerv(GL_VIEWPORT, viewport.data());
    EXPECT_EQ((std::array<GLint, 4>{3, 4, 5, 6}), viewport);
    EXPECT_TRUE(glIsEnabled(GL_DEPTH_TEST));
    EXPECT_TRUE(glIsEnabled(GL_CULL_FACE));
    EXPECT_TRUE(glIsEnabled(GL_BLEND));

    std::array<unsigned char, 4> pixel{0, 0, 0, 0};
    glReadPixels(8, 8, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel.data());

    EXPECT_NEAR(255, static_cast<int>(pixel[0]), 5);
    EXPECT_NEAR(0, static_cast<int>(pixel[1]), 5);
    EXPECT_EQ(255, pixel[3]);
    EXPECT_EQ(GL_NO_ERROR, glGetError());
}

TEST_F(PostProcessingPassTest, LinearLdrVisualizationBypassesSrgbEncoding) {
    opengl::Framebuffer::HdrConfig hdrConfig{16, 16, 1, true};
    auto hdrFb = opengl::Framebuffer::create_hdr(hdrConfig);
    ASSERT_TRUE(hdrFb.has_value());
    auto ldrFb = opengl::Framebuffer::create_ldr_intermediate(16, 16);
    ASSERT_TRUE(ldrFb.has_value());

    hdrFb->bind();
    glClearColor(0.25F, 0.25F, 0.25F, 1.0F);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    auto pass = opengl::PostProcessingPass::create();
    ASSERT_TRUE(pass.has_value());
    const float identity[16] = {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    };
    pass->set_inv_projection(identity);
    pass->set_fog_enabled(false);
    pass->set_exposure_stops(0.0F);
    pass->set_tone_map_mode(0);
    pass->set_hdr_display_max(10.0F);
    pass->set_grayscale(false);

    ldrFb->bind();
    pass->set_visualization_mode(0);
    pass->process(hdrFb->get_color_texture(), hdrFb->get_depth_texture(), 16, 16);
    std::array<unsigned char, 4> finalPixel{0, 0, 0, 0};
    glReadPixels(8, 8, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, finalPixel.data());

    ldrFb->bind();
    pass->set_visualization_mode(2);
    pass->process(hdrFb->get_color_texture(), hdrFb->get_depth_texture(), 16, 16);
    std::array<unsigned char, 4> linearPixel{0, 0, 0, 0};
    glReadPixels(8, 8, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, linearPixel.data());

    EXPECT_GT(static_cast<int>(finalPixel[0]), static_cast<int>(linearPixel[0]));
    EXPECT_NEAR(64, static_cast<int>(linearPixel[0]), 3);
    EXPECT_EQ(GL_NO_ERROR, glGetError());
}

TEST_F(PostProcessingPassTest, FogSettingsAffectVisibleGeometry) {
    opengl::Framebuffer::HdrConfig hdrConfig{16, 16, 1, true};
    auto hdrFb = opengl::Framebuffer::create_hdr(hdrConfig);
    ASSERT_TRUE(hdrFb.has_value());
    auto ldrFb = opengl::Framebuffer::create_ldr_intermediate(16, 16);
    ASSERT_TRUE(ldrFb.has_value());

    hdrFb->bind();
    glClearColor(1.0F, 0.0F, 0.0F, 1.0F);
    glClearDepth(0.5);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    auto pass = opengl::PostProcessingPass::create();
    ASSERT_TRUE(pass.has_value());
    const float identity[16] = {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    };
    pass->set_inv_projection(identity);
    pass->set_fog_enabled(true);
    pass->set_fog_mode(0);
    pass->set_fog_start(0.0F);
    pass->set_fog_end(0.01F);
    pass->set_fog_color(0.0F, 0.0F, 1.0F);
    pass->set_exposure_stops(0.0F);
    pass->set_tone_map_mode(0);
    pass->set_visualization_mode(0);
    pass->set_hdr_display_max(10.0F);
    pass->set_grayscale(false);

    ldrFb->bind();
    pass->process(hdrFb->get_color_texture(), hdrFb->get_depth_texture(), 16, 16);
    std::array<unsigned char, 4> pixel{0, 0, 0, 0};
    glReadPixels(8, 8, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel.data());

    EXPECT_NEAR(0, static_cast<int>(pixel[0]), 5);
    EXPECT_NEAR(0, static_cast<int>(pixel[1]), 5);
    EXPECT_NEAR(255, static_cast<int>(pixel[2]), 5);
    EXPECT_EQ(GL_NO_ERROR, glGetError());
}

TEST_F(PostProcessingPassTest, ExponentialFogMovesOutputTowardFogColor) {
    opengl::Framebuffer::HdrConfig hdrConfig{16, 16, 1, true};
    auto hdrFb = opengl::Framebuffer::create_hdr(hdrConfig);
    ASSERT_TRUE(hdrFb.has_value());
    auto ldrFb = opengl::Framebuffer::create_ldr_intermediate(16, 16);
    ASSERT_TRUE(ldrFb.has_value());

    hdrFb->bind();
    glClearColor(1.0F, 0.0F, 0.0F, 1.0F);
    glClearDepth(0.5);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    auto pass = opengl::PostProcessingPass::create();
    ASSERT_TRUE(pass.has_value());
    const float identity[16] = {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    };
    pass->set_inv_projection(identity);
    pass->set_exposure_stops(0.0F);
    pass->set_tone_map_mode(0);
    pass->set_visualization_mode(0);
    pass->set_hdr_display_max(10.0F);
    pass->set_grayscale(false);
    pass->set_fog_enabled(false);

    ldrFb->bind();
    pass->process(hdrFb->get_color_texture(), hdrFb->get_depth_texture(), 16, 16);
    std::array<unsigned char, 4> withoutFog{};
    glReadPixels(8, 8, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, withoutFog.data());

    pass->set_fog_enabled(true);
    pass->set_fog_mode(1);
    pass->set_fog_density(100.0F);
    pass->set_fog_color(0.0F, 0.0F, 1.0F);
    ldrFb->bind();
    pass->process(hdrFb->get_color_texture(), hdrFb->get_depth_texture(), 16, 16);
    std::array<unsigned char, 4> withFog{};
    glReadPixels(8, 8, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, withFog.data());

    EXPECT_GT(withoutFog[0], withFog[0]);
    EXPECT_LT(withoutFog[2], withFog[2]);
    EXPECT_NE(withoutFog, withFog);
    EXPECT_EQ(GL_NO_ERROR, glGetError());
}

TEST_F(PostProcessingPassTest, VisualizationModesProduceDeterministicOutput) {
    opengl::Framebuffer::HdrConfig hdrConfig{16, 16, 1, true};
    auto hdrFb = opengl::Framebuffer::create_hdr(hdrConfig);
    ASSERT_TRUE(hdrFb.has_value());
    auto ldrFb = opengl::Framebuffer::create_ldr_intermediate(16, 16);
    ASSERT_TRUE(ldrFb.has_value());
    auto pass = opengl::PostProcessingPass::create();
    ASSERT_TRUE(pass.has_value());
    const float identity[16] = {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    };
    pass->set_inv_projection(identity);
    pass->set_fog_enabled(false);
    pass->set_exposure_stops(0.0F);
    pass->set_tone_map_mode(0);
    pass->set_hdr_display_max(4.0F);
    pass->set_grayscale(false);

    const auto render = [&](int mode) {
        ldrFb->bind();
        pass->set_visualization_mode(mode);
        pass->process(hdrFb->get_color_texture(), hdrFb->get_depth_texture(), 16, 16);
        std::array<unsigned char, 4> pixel{};
        glReadPixels(8, 8, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel.data());
        return pixel;
    };

    hdrFb->bind();
    glClearColor(2.0F, 0.5F, 0.25F, 1.0F);
    glClearDepth(0.5);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    const auto rawHdr = render(1);
    EXPECT_NEAR(188, rawHdr[0], 4);
    EXPECT_NEAR(99, rawHdr[1], 4);
    EXPECT_NEAR(71, rawHdr[2], 4);
    const auto luminance = render(3);
    EXPECT_NEAR(124, luminance[0], 4);
    EXPECT_EQ(luminance[0], luminance[1]);
    EXPECT_EQ(luminance[1], luminance[2]);
    const auto logLuminance = render(4);
    EXPECT_NEAR(185, logLuminance[0], 4);
    EXPECT_EQ(logLuminance[0], logLuminance[1]);
    EXPECT_EQ(logLuminance[1], logLuminance[2]);
    const auto depth = render(5);
    EXPECT_NEAR(188, depth[0], 4);
    EXPECT_EQ(depth[0], depth[1]);
    EXPECT_EQ(depth[1], depth[2]);
    const auto overexposure = render(6);
    EXPECT_EQ((std::array<unsigned char, 4>{255, 0, 255, 255}), overexposure);
    const auto grayscale = render(9);
    EXPECT_NEAR(202, grayscale[0], 4);
    EXPECT_EQ(grayscale[0], grayscale[1]);
    EXPECT_EQ(grayscale[1], grayscale[2]);

    // Mesa llvmpipe/Xvfb normalizes invalid floats before this RGBA16F fixture
    // reaches the shader, so NaNAndInfinity has no deterministic input here.

    hdrFb->bind();
    glClearColor(0.001F, 0.001F, 0.001F, 1.0F);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    const auto underexposure = render(7);
    EXPECT_EQ((std::array<unsigned char, 4>{0, 255, 255, 255}), underexposure);
    EXPECT_EQ(GL_NO_ERROR, glGetError());
}

} // namespace
