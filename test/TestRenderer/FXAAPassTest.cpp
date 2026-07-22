#include <GLFW/glfw3.h>
#include <OpenGL/Framebuffer.hpp>
#include <OpenGL/FXAAPass.hpp>
#include <OpenGL/OpenGL.hpp>
#include <array>
#include <cmath>
#include <gtest/gtest.h>

namespace {

void* load_glfw_proc(const char* procName) {
    return reinterpret_cast<void*>(glfwGetProcAddress(procName));
}

class FXAAPassTest : public ::testing::Test {
  protected:
    static void SetUpTestSuite() { ASSERT_EQ(GLFW_TRUE, glfwInit()); }

    static void TearDownTestSuite() { glfwTerminate(); }

    void SetUp() override {
        glfwDefaultWindowHints();
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        m_window = glfwCreateWindow(64, 64, "plinth fxaa pass test", nullptr, nullptr);
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

TEST_F(FXAAPassTest, CreatesAndIsValid) {
    auto pass = opengl::FXAAPass::create();
    ASSERT_TRUE(pass.has_value());
    EXPECT_TRUE(pass->is_valid());
}

TEST_F(FXAAPassTest, PassThroughUniformColor) {
    auto source = opengl::Framebuffer::create_ldr_intermediate(16, 16);
    ASSERT_TRUE(source.has_value());

    source->bind();
    glClearColor(0.2F, 0.4F, 0.6F, 1.0F);
    glClear(GL_COLOR_BUFFER_BIT);

    auto pass = opengl::FXAAPass::create();
    ASSERT_TRUE(pass.has_value());
    ASSERT_TRUE(pass->is_valid());

    pass->set_enabled(true);
    pass->set_edge_threshold(0.166f);
    pass->set_edge_threshold_min(0.0833f);
    pass->set_subpixel_amount(0.75f);

    opengl::Framebuffer::unbind();
    glViewport(3, 4, 5, 6);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    GLuint texture0 = 0;
    glGenTextures(1, &texture0);
    ASSERT_NE(0U, texture0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, source->get_color_texture());
    pass->process(source->get_color_texture(), 16, 16);

    std::array<GLint, 4> viewport{};
    glGetIntegerv(GL_VIEWPORT, viewport.data());
    EXPECT_EQ((std::array<GLint, 4>{3, 4, 5, 6}), viewport);
    EXPECT_TRUE(glIsEnabled(GL_DEPTH_TEST));
    EXPECT_TRUE(glIsEnabled(GL_CULL_FACE));
    EXPECT_TRUE(glIsEnabled(GL_BLEND));
    GLint activeTexture = 0;
    GLint texture0Binding = 0;
    glGetIntegerv(GL_ACTIVE_TEXTURE, &activeTexture);
    glActiveTexture(GL_TEXTURE0);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &texture0Binding);
    EXPECT_EQ(GL_TEXTURE1, activeTexture);
    EXPECT_EQ(static_cast<GLint>(texture0), texture0Binding);
    glBindTexture(GL_TEXTURE_2D, 0);
    glDeleteTextures(1, &texture0);
    glActiveTexture(GL_TEXTURE1);

    std::array<unsigned char, 4> pixel{0, 0, 0, 0};
    glReadPixels(8, 8, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel.data());

    EXPECT_NEAR(static_cast<int>(std::round(0.2F * 255.0F)), static_cast<int>(pixel[0]), 2);
    EXPECT_NEAR(static_cast<int>(std::round(0.4F * 255.0F)), static_cast<int>(pixel[1]), 2);
    EXPECT_NEAR(static_cast<int>(std::round(0.6F * 255.0F)), static_cast<int>(pixel[2]), 2);
    EXPECT_EQ(255, pixel[3]);
    EXPECT_EQ(GL_NO_ERROR, glGetError());
}

TEST_F(FXAAPassTest, SmoothsHardEdgeWithoutChangingInterior) {
    constexpr int size = 16;
    auto source = opengl::Framebuffer::create_ldr_intermediate(size, size);
    ASSERT_TRUE(source.has_value());
    source->bind();
    glClearColor(0.0F, 0.0F, 0.0F, 1.0F);
    glClear(GL_COLOR_BUFFER_BIT);
    glEnable(GL_SCISSOR_TEST);
    glScissor(size / 2, 0, size / 2, size);
    glClearColor(1.0F, 1.0F, 1.0F, 1.0F);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_SCISSOR_TEST);

    auto pass = opengl::FXAAPass::create();
    ASSERT_TRUE(pass.has_value());
    pass->set_edge_threshold(0.166F);
    pass->set_edge_threshold_min(0.0833F);
    pass->set_subpixel_amount(0.75F);

    opengl::Framebuffer::unbind();
    pass->set_enabled(false);
    pass->process(source->get_color_texture(), size, size);
    std::array<unsigned char, 4> disabledEdge{};
    std::array<unsigned char, 4> disabledInterior{};
    glReadPixels(7, 8, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, disabledEdge.data());
    glReadPixels(4, 8, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, disabledInterior.data());

    pass->set_enabled(true);
    pass->process(source->get_color_texture(), size, size);
    std::array<unsigned char, 4> enabledEdge{};
    std::array<unsigned char, 4> enabledInterior{};
    glReadPixels(7, 8, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, enabledEdge.data());
    glReadPixels(4, 8, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, enabledInterior.data());

    EXPECT_EQ(0, disabledEdge[0]);
    EXPECT_EQ(0, disabledInterior[0]);
    EXPECT_GT(enabledEdge[0], 0);
    EXPECT_LT(enabledEdge[0], 255);
    EXPECT_EQ(disabledInterior, enabledInterior);
    EXPECT_EQ(GL_NO_ERROR, glGetError());
}

} // namespace
