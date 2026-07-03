#include <GLFW/glfw3.h>
#include <Renderer/InputCaptureState.hpp>
#include <gtest/gtest.h>

using namespace renderer;

TEST(InputCaptureStateTest, MousePressCapturedByGuiBlocksDragAndRelease) {
    InputCaptureState state;

    EXPECT_FALSE(state.should_forward_mouse_button(GLFW_MOUSE_BUTTON_RIGHT, Action::PRESS, true));
    EXPECT_FALSE(state.should_forward_cursor_position(false));
    EXPECT_FALSE(state.should_forward_mouse_button(GLFW_MOUSE_BUTTON_RIGHT, Action::RELEASE, false));
}

TEST(InputCaptureStateTest, MousePressOutsideGuiOwnsDragUntilRelease) {
    InputCaptureState state;

    EXPECT_TRUE(state.should_forward_mouse_button(GLFW_MOUSE_BUTTON_RIGHT, Action::PRESS, false));
    EXPECT_TRUE(state.should_forward_cursor_position(true));
    EXPECT_TRUE(state.should_forward_mouse_button(GLFW_MOUSE_BUTTON_RIGHT, Action::RELEASE, true));
    EXPECT_FALSE(state.should_forward_cursor_position(true));
}

TEST(InputCaptureStateTest, HoveredGuiBlocksMouseAndKeyboardCaptureBlocksKeys) {
    InputCaptureState state;

    EXPECT_FALSE(state.should_forward_cursor_position(true));
    EXPECT_FALSE(state.should_forward_scroll(true));
    EXPECT_TRUE(state.should_forward_scroll(false));
    EXPECT_FALSE(InputCaptureState::should_forward_key(true));
    EXPECT_TRUE(InputCaptureState::should_forward_key(false));
}
