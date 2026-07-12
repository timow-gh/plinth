#include <GLFW/glfw3.h>
#include <plinth/InputState.hpp>
#include "../../src/plinth/source/InputStateInternal.hpp"
#include <gtest/gtest.h>

using namespace renderer;

namespace {

class GlfwWindowFixture : public ::testing::Test {
  protected:
    static void SetUpTestSuite() { ASSERT_EQ(GLFW_TRUE, glfwInit()); }

    static void TearDownTestSuite() { glfwTerminate(); }

    void SetUp() override {
        glfwDefaultWindowHints();
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
        m_window = glfwCreateWindow(64, 64, "plinth InputState test", nullptr, nullptr);
        ASSERT_NE(nullptr, m_window);
    }

    void TearDown() override {
        if (m_window != nullptr) {
            renderer::detail::clear_callbacks(m_window);
            glfwDestroyWindow(m_window);
            m_window = nullptr;
        }
    }

    GLFWwindow* m_window{nullptr};
};

} // namespace

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

TEST_F(GlfwWindowFixture, TrampolinesForwardRegisteredCallbacks) {
    renderer::Key receivedKey = renderer::Key::KEY_UNKNOWN;
    renderer::Scancode receivedScancode;
    renderer::Action receivedAction = renderer::Action::UNDEFINED;
    renderer::Mods receivedKeyMods = renderer::Mods::NONE;
    renderer::detail::set_key_callback(
        m_window,
        [&](renderer::Key key, renderer::Scancode scancode, renderer::Action action, renderer::Mods mods) {
            receivedKey = key;
            receivedScancode = scancode;
            receivedAction = action;
            receivedKeyMods = mods;
        });
    GLFWkeyfun keyCallback = glfwSetKeyCallback(m_window, nullptr);
    ASSERT_NE(nullptr, keyCallback);
    keyCallback(m_window, GLFW_KEY_A, 12, GLFW_PRESS, GLFW_MOD_SHIFT);
    EXPECT_EQ(renderer::Key::KEY_A, receivedKey);
    EXPECT_EQ(12, receivedScancode.get_value());
    EXPECT_EQ(renderer::Action::PRESS, receivedAction);
    EXPECT_EQ(renderer::Mods::SHIFT, receivedKeyMods);

    std::uint32_t receivedChar = 0;
    renderer::detail::set_char_callback(m_window, [&](std::uint32_t codepoint) { receivedChar = codepoint; });
    GLFWcharfun charCallback = glfwSetCharCallback(m_window, nullptr);
    ASSERT_NE(nullptr, charCallback);
    charCallback(m_window, 66);
    EXPECT_EQ(66U, receivedChar);

    std::uint32_t receivedCharModsCodepoint = 0;
    renderer::Mods receivedCharMods = renderer::Mods::NONE;
    renderer::detail::set_char_mods_callback(m_window, [&](std::uint32_t codepoint, renderer::Mods mods) {
        receivedCharModsCodepoint = codepoint;
        receivedCharMods = mods;
    });
    GLFWcharmodsfun charModsCallback = glfwSetCharModsCallback(m_window, nullptr);
    ASSERT_NE(nullptr, charModsCallback);
    charModsCallback(m_window, 67, GLFW_MOD_ALT);
    EXPECT_EQ(67U, receivedCharModsCodepoint);
    EXPECT_EQ(renderer::Mods::ALT, receivedCharMods);

    int receivedButton = -1;
    renderer::Action receivedButtonAction = renderer::Action::UNDEFINED;
    renderer::Mods receivedButtonMods = renderer::Mods::NONE;
    renderer::detail::set_mouse_button_callback(m_window, [&](int button, renderer::Action action, renderer::Mods mods) {
        receivedButton = button;
        receivedButtonAction = action;
        receivedButtonMods = mods;
    });
    GLFWmousebuttonfun mouseButtonCallback = glfwSetMouseButtonCallback(m_window, nullptr);
    ASSERT_NE(nullptr, mouseButtonCallback);
    mouseButtonCallback(m_window, GLFW_MOUSE_BUTTON_RIGHT, GLFW_RELEASE, GLFW_MOD_CONTROL);
    EXPECT_EQ(GLFW_MOUSE_BUTTON_RIGHT, receivedButton);
    EXPECT_EQ(renderer::Action::RELEASE, receivedButtonAction);
    EXPECT_EQ(renderer::Mods::CONTROL, receivedButtonMods);

    bool receivedCursorEnter = false;
    renderer::detail::set_cursor_enter_callback(m_window, [&](bool entered) { receivedCursorEnter = entered; });
    GLFWcursorenterfun cursorEnterCallback = glfwSetCursorEnterCallback(m_window, nullptr);
    ASSERT_NE(nullptr, cursorEnterCallback);
    cursorEnterCallback(m_window, GLFW_TRUE);
    EXPECT_TRUE(receivedCursorEnter);

    double receivedScrollX = 0.0;
    double receivedScrollY = 0.0;
    renderer::detail::set_scroll_callback(m_window, [&](double xoff, double yoff) {
        receivedScrollX = xoff;
        receivedScrollY = yoff;
    });
    GLFWscrollfun scrollCallback = glfwSetScrollCallback(m_window, nullptr);
    ASSERT_NE(nullptr, scrollCallback);
    scrollCallback(m_window, 1.5, -2.5);
    EXPECT_DOUBLE_EQ(1.5, receivedScrollX);
    EXPECT_DOUBLE_EQ(-2.5, receivedScrollY);

    int receivedDropCount = 0;
    const char** receivedDropPaths = nullptr;
    renderer::detail::set_drop_callback(m_window, [&](int count, const char** paths) {
        receivedDropCount = count;
        receivedDropPaths = paths;
    });
    GLFWdropfun dropCallback = glfwSetDropCallback(m_window, nullptr);
    ASSERT_NE(nullptr, dropCallback);
    const char* paths[] = {"first", "second"};
    dropCallback(m_window, 2, paths);
    EXPECT_EQ(2, receivedDropCount);
    EXPECT_EQ(paths, receivedDropPaths);

    std::uint32_t framebufferWidth = 0;
    std::uint32_t framebufferHeight = 0;
    renderer::detail::set_framebuffer_size_callback(m_window, [&](std::uint32_t width, std::uint32_t height) {
        framebufferWidth = width;
        framebufferHeight = height;
    });
    GLFWframebuffersizefun framebufferSizeCallback = glfwSetFramebufferSizeCallback(m_window, nullptr);
    ASSERT_NE(nullptr, framebufferSizeCallback);
    framebufferSizeCallback(m_window, 320, 240);
    EXPECT_EQ(320U, framebufferWidth);
    EXPECT_EQ(240U, framebufferHeight);
}

TEST_F(GlfwWindowFixture, InputStateStoragePersistsUntilCleared) {
    renderer::InputState* firstState = renderer::detail::get_input_state(m_window);
    ASSERT_NE(nullptr, firstState);
    firstState->cursorPosState = renderer::CursorPosState{10.0, 20.0};

    renderer::InputState* secondState = renderer::detail::get_input_state(m_window);
    EXPECT_EQ(firstState, secondState);
    EXPECT_DOUBLE_EQ(10.0, secondState->cursorPosState.xpos);
    EXPECT_DOUBLE_EQ(20.0, secondState->cursorPosState.ypos);

    renderer::detail::clear_callbacks(m_window);
    renderer::InputState* resetState = renderer::detail::get_input_state(m_window);
    ASSERT_NE(nullptr, resetState);
    EXPECT_DOUBLE_EQ(0.0, resetState->cursorPosState.xpos);
    EXPECT_DOUBLE_EQ(0.0, resetState->cursorPosState.ypos);
}
