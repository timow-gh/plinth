#include <GLFW/glfw3.h>
#include <OpenGL/ErrorReporting.hpp>
#include <OpenGL/OpenGL.hpp>
#include <gtest/gtest.h>

namespace {

void* load_glfw_proc(const char* procName) {
    // GLAD v1 expects void* while GLFW returns an opaque function pointer.
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    return reinterpret_cast<void*>(glfwGetProcAddress(procName));
}

// A GL 4.1 core, non-debug context (the project floor). install_debug_callback()
// must return false when the context is below 4.3 and true at 4.3+. The test queries
// the actual context version and asserts the correct outcome. GLFW version hints are
// minimums, so some drivers may give a higher version than requested.
class DebugCallbackTest : public ::testing::Test {
  protected:
    static void SetUpTestSuite() { ASSERT_EQ(GLFW_TRUE, glfwInit()); }

    static void TearDownTestSuite() { glfwTerminate(); }

    void SetUp() override {
        glfwDefaultWindowHints();
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        m_window = glfwCreateWindow(64, 64, "plinth debug callback test", nullptr, nullptr);
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

TEST_F(DebugCallbackTest, ReturnsCorrectValueForContextVersion) {
    GLint major = 0;
    GLint minor = 0;
    glGetIntegerv(GL_MAJOR_VERSION, &major);
    glGetIntegerv(GL_MINOR_VERSION, &minor);
    const bool expectsDebug = (major > 4 || (major == 4 && minor >= 3));
    EXPECT_EQ(expectsDebug, opengl::install_debug_callback());
}

} // namespace
