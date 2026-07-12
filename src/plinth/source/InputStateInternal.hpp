#ifndef RENDERER_INPUTSTATEINTERNAL_HPP
#define RENDERER_INPUTSTATEINTERNAL_HPP

#include <plinth/InputState.hpp>

struct GLFWwindow;

namespace renderer::detail {

InputState* get_input_state(GLFWwindow* window);

void set_key_callback(GLFWwindow* window, KeyCB cb);
void set_char_callback(GLFWwindow* window, CharCB cb);
void set_char_mods_callback(GLFWwindow* window, CharModsCB cb);
void set_mouse_button_callback(GLFWwindow* window, MouseBtnCB cb);
void set_cursor_pos_callback(GLFWwindow* window, CursorPosCB cb);
void set_cursor_enter_callback(GLFWwindow* window, CursorEnterCB cb);
void set_scroll_callback(GLFWwindow* window, ScrollCB cb);
void set_drop_callback(GLFWwindow* window, DropCB cb);

void set_framebuffer_size_callback(GLFWwindow* window, FramebufferSizeCB cb);

void clear_callbacks(GLFWwindow* window);

} // namespace renderer::detail

#endif // RENDERER_INPUTSTATEINTERNAL_HPP
