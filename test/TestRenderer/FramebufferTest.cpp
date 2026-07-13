#include <GLFW/glfw3.h>
#include <OpenGL/Framebuffer.hpp>
#include <OpenGL/OpenGL.hpp>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <gtest/gtest.h>
#include <optional>

namespace {

void* load_glfw_proc(const char* procName) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    return reinterpret_cast<void*>(glfwGetProcAddress(procName));
}

class FramebufferTest : public ::testing::Test {
  protected:
    static void SetUpTestSuite() { ASSERT_EQ(GLFW_TRUE, glfwInit()); }

    static void TearDownTestSuite() { glfwTerminate(); }

    void SetUp() override {
        glfwDefaultWindowHints();
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        m_window = glfwCreateWindow(64, 64, "plinth framebuffer test", nullptr, nullptr);
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

TEST_F(FramebufferTest, InvalidDimensionsReturnNullopt) {
    EXPECT_EQ(std::nullopt, opengl::Framebuffer::create(0, 64, 1, false));
    EXPECT_EQ(std::nullopt, opengl::Framebuffer::create(64, 0, 1, false));
    EXPECT_EQ(std::nullopt, opengl::Framebuffer::create(64, 64, 0, false));
    EXPECT_EQ(std::nullopt, opengl::Framebuffer::create(-1, 64, 1, false));
    EXPECT_EQ(std::nullopt, opengl::Framebuffer::create(64, -1, 1, false));
}

TEST_F(FramebufferTest, SingleSampleIsValid) {
    auto fb = opengl::Framebuffer::create(64, 64, 1, false);
    ASSERT_TRUE(fb.has_value());
    EXPECT_TRUE(fb->is_valid());
    EXPECT_NE(0u, fb->get_id());
    EXPECT_NE(0u, fb->get_color_texture());
    EXPECT_EQ(1, fb->get_samples());
    EXPECT_EQ(64, fb->get_width());
    EXPECT_EQ(64, fb->get_height());
}

TEST_F(FramebufferTest, SingleSampleSrgbUsesCorrectInternalFormat) {
    auto fb = opengl::Framebuffer::create(64, 64, 1, true);
    ASSERT_TRUE(fb.has_value());
    EXPECT_TRUE(fb->is_valid());
    EXPECT_EQ(1, fb->get_samples());

    const GLuint tex = fb->get_color_texture();
    ASSERT_NE(0u, tex);

    glBindTexture(GL_TEXTURE_2D, tex);
    GLint internalFormat = 0;
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_INTERNAL_FORMAT, &internalFormat);
    glBindTexture(GL_TEXTURE_2D, 0);

    EXPECT_EQ(GL_SRGB8_ALPHA8, internalFormat);
    EXPECT_EQ(GL_NO_ERROR, glGetError());
}

TEST_F(FramebufferTest, BindUnbindChangesBinding) {
    auto fb = opengl::Framebuffer::create(64, 64, 1, false);
    ASSERT_TRUE(fb.has_value());

    fb->bind();
    GLint currentBinding = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &currentBinding);
    EXPECT_EQ(static_cast<GLint>(fb->get_id()), currentBinding);

    opengl::Framebuffer::unbind();
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &currentBinding);
    EXPECT_EQ(0, currentBinding);
}

TEST_F(FramebufferTest, MultisampleIsValidAndHasNoTexture) {
    GLint maxSamples = 0;
    glGetIntegerv(GL_MAX_SAMPLES, &maxSamples);
    ASSERT_GT(maxSamples, 0);

    if (maxSamples < 2) {
        GTEST_SKIP() << "GL_MAX_SAMPLES < 2, skipping MSAA test";
    }

    const int samples = std::min(4, maxSamples);
    auto fb = opengl::Framebuffer::create(64, 64, samples, false);
    ASSERT_TRUE(fb.has_value());
    EXPECT_TRUE(fb->is_valid());
    EXPECT_EQ(samples, fb->get_samples());
    EXPECT_EQ(0u, fb->get_color_texture());
    EXPECT_EQ(64, fb->get_width());
    EXPECT_EQ(64, fb->get_height());
}

TEST_F(FramebufferTest, ResolveBlitsColor) {
    GLint maxSamples = 0;
    glGetIntegerv(GL_MAX_SAMPLES, &maxSamples);
    ASSERT_GT(maxSamples, 0);

    if (maxSamples < 2) {
        GTEST_SKIP() << "GL_MAX_SAMPLES < 2, skipping MSAA resolve test";
    }

    const int samples = std::min(4, maxSamples);
    auto src = opengl::Framebuffer::create(64, 64, samples, false);
    ASSERT_TRUE(src.has_value());

    auto dst = opengl::Framebuffer::create(64, 64, 1, false);
    ASSERT_TRUE(dst.has_value());

    src->bind();
    glClearColor(0.2f, 0.4f, 0.6f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    opengl::Framebuffer::unbind();

    const bool resolved = src->resolve_to(*dst);
    EXPECT_TRUE(resolved);

    dst->bind();
    std::array<unsigned char, 4> pixel{0, 0, 0, 0};
    glReadPixels(32, 32, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel.data());
    opengl::Framebuffer::unbind();

    EXPECT_EQ(GL_NO_ERROR, glGetError());

    const int rExpected = static_cast<int>(std::round(0.2f * 255.0f));
    const int gExpected = static_cast<int>(std::round(0.4f * 255.0f));
    const int bExpected = static_cast<int>(std::round(0.6f * 255.0f));

    EXPECT_NEAR(rExpected, static_cast<int>(pixel[0]), 1);
    EXPECT_NEAR(gExpected, static_cast<int>(pixel[1]), 1);
    EXPECT_NEAR(bExpected, static_cast<int>(pixel[2]), 1);
}

TEST_F(FramebufferTest, ResizeSucceedsAndUpdatesDimensions) {
    auto fb = opengl::Framebuffer::create(64, 64, 1, false);
    ASSERT_TRUE(fb.has_value());
    const GLuint originalId = fb->get_id();

    EXPECT_TRUE(fb->resize(32, 16));
    EXPECT_EQ(32, fb->get_width());
    EXPECT_EQ(16, fb->get_height());
    EXPECT_TRUE(fb->is_valid());
    EXPECT_NE(originalId, fb->get_id());
}

TEST_F(FramebufferTest, ResizePreservesStateOnFailure) {
    auto fb = opengl::Framebuffer::create(64, 64, 1, false);
    ASSERT_TRUE(fb.has_value());
    const GLuint originalId = fb->get_id();

    EXPECT_FALSE(fb->resize(0, 16));
    EXPECT_EQ(64, fb->get_width());
    EXPECT_EQ(64, fb->get_height());
    EXPECT_EQ(originalId, fb->get_id());
    EXPECT_TRUE(fb->is_valid());
}

TEST_F(FramebufferTest, MoveConstructionInvalidatesSource) {
    auto fb = opengl::Framebuffer::create(64, 64, 1, false);
    ASSERT_TRUE(fb.has_value());
    ASSERT_TRUE(fb->is_valid());
    const GLuint originalId = fb->get_id();

    opengl::Framebuffer moved{std::move(*fb)};
    EXPECT_TRUE(moved.is_valid());
    EXPECT_EQ(originalId, moved.get_id());
    EXPECT_EQ(64, moved.get_width());
    EXPECT_EQ(64, moved.get_height());

    EXPECT_FALSE(fb->is_valid());
    EXPECT_EQ(0u, fb->get_id());
    EXPECT_EQ(0, fb->get_width());
    EXPECT_EQ(0, fb->get_height());
}

TEST_F(FramebufferTest, MoveAssignmentInvalidatesSource) {
    auto fb = opengl::Framebuffer::create(64, 64, 1, false);
    ASSERT_TRUE(fb.has_value());
    ASSERT_TRUE(fb->is_valid());
    const GLuint originalId = fb->get_id();

    auto other = opengl::Framebuffer::create(32, 32, 1, false);
    ASSERT_TRUE(other.has_value());

    *other = std::move(*fb);
    EXPECT_TRUE(other->is_valid());
    EXPECT_EQ(originalId, other->get_id());
    EXPECT_EQ(64, other->get_width());
    EXPECT_EQ(64, other->get_height());

    EXPECT_FALSE(fb->is_valid());
    EXPECT_EQ(0u, fb->get_id());
    EXPECT_EQ(0, fb->get_width());
    EXPECT_EQ(0, fb->get_height());
}

} // namespace
