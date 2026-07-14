#include <GLFW/glfw3.h>
#include <OpenGL/OpenGL.hpp>
#include <OpenGL/FrameState.hpp>
#include <gtest/gtest.h>
#include <plinth/WindowSettings.hpp>

namespace {

void* load_glfw_proc(const char* procName) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    return reinterpret_cast<void*>(glfwGetProcAddress(procName));
}

class FrameStateBeginFrameTest : public ::testing::Test {
  protected:
    static void SetUpTestSuite() { ASSERT_EQ(GLFW_TRUE, glfwInit()); }

    static void TearDownTestSuite() { glfwTerminate(); }

    void SetUp() override {
        glfwDefaultWindowHints();
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        m_window = glfwCreateWindow(64, 64, "plinth frame state test", nullptr, nullptr);
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

TEST_F(FrameStateBeginFrameTest, BeginFrameEnablesSrgbFramebuffer) {
    const opengl::ClearColor clearColor{0.0F, 0.0F, 0.0F, 1.0F};
    const opengl::ViewportRect viewport{0, 0, 64, 64};

    opengl::begin_frame(clearColor, viewport);

    const GLenum err = glGetError();
    EXPECT_EQ(GL_NO_ERROR, err);
    EXPECT_EQ(GL_TRUE, glIsEnabled(GL_FRAMEBUFFER_SRGB));
}

TEST_F(FrameStateBeginFrameTest, DefaultWindowSettingsRequestsSrgbCapable) {
    const renderer::WindowSettings settings;
    EXPECT_TRUE(settings.srgb_capable);
}

TEST_F(FrameStateBeginFrameTest, BeginFrameSetsNonZeroViewportOrigin) {
    const opengl::ClearColor clearColor{0.2F, 0.4F, 0.6F, 1.0F};
    const opengl::ViewportRect viewport{16, 0, 48, 64};

    opengl::begin_frame(clearColor, viewport);

    GLint params[4]{0, 0, 0, 0};
    glGetIntegerv(GL_VIEWPORT, params);
    EXPECT_EQ(16, params[0]);
    EXPECT_EQ(0, params[1]);
    EXPECT_EQ(48, params[2]);
    EXPECT_EQ(64, params[3]);
    EXPECT_EQ(GL_NO_ERROR, glGetError());
}
