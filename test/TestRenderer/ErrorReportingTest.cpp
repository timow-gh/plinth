#include <GLFW/glfw3.h>
#include <OpenGL/ErrorReporting.hpp>
#include <OpenGL/OpenGL.hpp>
#include <gtest/gtest.h>
#include <string>
#include <vector>

namespace {

TEST(ErrorReportingTest, CustomSinkReceivesReportedMessages) {
    std::vector<std::string> received;
    opengl::set_error_sink([&received](opengl::ErrorSeverity severity, std::string_view message) {
        received.emplace_back(severity == opengl::ErrorSeverity::error ? "error: " : "warning: ");
        received.back() += message;
    });

    opengl::report_error("x");

    ASSERT_EQ(received.size(), 1U);
    EXPECT_EQ(received.front(), "error: x");

    opengl::set_error_sink(nullptr);
}

void* load_glfw_proc(const char* procName) {
    // GLAD v1 expects void* while GLFW returns an opaque function pointer.
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    return reinterpret_cast<void*>(glfwGetProcAddress(procName));
}

// A GL 4.1 core, non-debug context - the plinth default (see GlfwWindow::create). 4.1 < 4.3,
// so install_debug_callback() must deterministically report unavailability on it without
// needing a real 4.3 debug context in CI.
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

TEST_F(DebugCallbackTest, ReturnsFalseOnSub43Context) {
    EXPECT_FALSE(opengl::install_debug_callback());
}

} // namespace
