#include <GLFW/glfw3.h>
#include <OpenGL/GpuCapabilities.hpp>
#include <OpenGL/OpenGL.hpp>
#include <gtest/gtest.h>

namespace {

void* load_glfw_proc(const char* procName) {
    // GLAD v1 expects void* while GLFW returns an opaque function pointer.
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    return reinterpret_cast<void*>(glfwGetProcAddress(procName));
}

// A GL 4.1 core, non-debug context - the plinth default (see GlfwWindow::create). Mirrors
// DebugCallbackTest's fixture (ErrorReportingTest.cpp): a real GL context is required since
// query_gpu_capabilities() reads driver-reported state, not something that can be faked.
class GpuCapabilitiesTest : public ::testing::Test {
  protected:
    static void SetUpTestSuite() { ASSERT_EQ(GLFW_TRUE, glfwInit()); }

    static void TearDownTestSuite() { glfwTerminate(); }

    void SetUp() override {
        glfwDefaultWindowHints();
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        m_window = glfwCreateWindow(64, 64, "plinth gpu capabilities test", nullptr, nullptr);
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

TEST_F(GpuCapabilitiesTest, ReturnsPlausibleValuesOnRealContext) {
    const opengl::GpuCapabilities caps = opengl::query_gpu_capabilities();

    // Matches plan 004's floor - GlfwWindow::create's default requested context is GL 4.1 core.
    EXPECT_TRUE(caps.supports_version(4, 1));
    EXPECT_GT(caps.maxTextureSize, 0);
    EXPECT_GT(caps.maxColorAttachments, 0);
    EXPECT_FALSE(caps.glRenderer.empty());
    EXPECT_FALSE(caps.glVendor.empty());
    EXPECT_FALSE(caps.glslVersion.empty());
}

} // namespace
