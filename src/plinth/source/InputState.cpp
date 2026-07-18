#include "InputStateInternal.hpp"
#include <GLFW/glfw3.h>
#include <plinth/InputState.hpp>
#include <unordered_map>

namespace renderer {

namespace detail {

struct WindowCallbacks {
    KeyCB key;
    CharCB ch;
    CharModsCB chMods;
    MouseBtnCB mouseBtn;
    CursorPosCB cursorPos;
    CursorEnterCB cursorEnter;
    ScrollCB scroll;
    DropCB drop;

    InputState inputState;

    FramebufferSizeCB framebufferSizeCB;
};

static void set_window_callbacks(GLFWwindow* glfwWindow);

static std::unordered_map<GLFWwindow*, WindowCallbacks>& window_callbacks_storage() {
    static std::unordered_map<GLFWwindow*, WindowCallbacks>
        callbacks; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
    return callbacks;
}

static WindowCallbacks* get_window_callbacks(GLFWwindow* glfwWindow) {
    RENDERER_ASSERT(glfwWindow);
    auto [iterator, inserted] = window_callbacks_storage().try_emplace(glfwWindow);
    if (inserted) {
        set_window_callbacks(glfwWindow);
    }
    return &iterator->second;
}

} // namespace detail

static void key_callback_trampoline(GLFWwindow* glfwWindow, int key, int sc, int act, int mods) {
    if (auto* wCallbacks = detail::get_window_callbacks(glfwWindow); wCallbacks->key) {
        wCallbacks->key(static_cast<Key>(key), Scancode(sc), static_cast<Action>(act), static_cast<Mods>(mods));
    }
}
static void char_callback_trampoline(GLFWwindow* glfwWindow, unsigned int cp) {
    if (auto* wCallbacks = detail::get_window_callbacks(glfwWindow); wCallbacks->ch) {
        wCallbacks->ch(cp);
    }
}
static void char_mods_callback_trampoline(GLFWwindow* glfwWindow, unsigned int cp, int mods) {
    if (auto* wCallbacks = detail::get_window_callbacks(glfwWindow); wCallbacks->chMods) {
        wCallbacks->chMods(cp, static_cast<Mods>(mods));
    }
}
static void mouse_button_callback_trampoline(GLFWwindow* glfwWindow, int btn, int act, int mods) {
    if (auto* wCallbacks = detail::get_window_callbacks(glfwWindow); wCallbacks->mouseBtn) {
        wCallbacks->mouseBtn(btn, static_cast<Action>(act), static_cast<Mods>(mods));
    }
}
static void cursor_pos_callback_trampoline(GLFWwindow* glfwWindow, double x, double y) {
    if (auto* wCallbacks = detail::get_window_callbacks(glfwWindow); wCallbacks->cursorPos) {
        wCallbacks->cursorPos(x, y);
    }
}
static void cursor_enter_callback_trampoline(GLFWwindow* glfwWindow, int entered) {
    if (auto* wCallbacks = detail::get_window_callbacks(glfwWindow); wCallbacks->cursorEnter) {
        wCallbacks->cursorEnter(entered != 0);
    }
}
static void scroll_callback_trampoline(GLFWwindow* glfwWindow, double xo, double yo) {
    if (auto* wCallbacks = detail::get_window_callbacks(glfwWindow); wCallbacks->scroll) {
        wCallbacks->scroll(xo, yo);
    }
}
static void drop_callback_trampoline(GLFWwindow* glfwWindow, int cnt, const char** paths) {
    if (auto* wCallbacks = detail::get_window_callbacks(glfwWindow); wCallbacks->drop) {
        wCallbacks->drop(cnt, paths);
    }
}

static void framebuffer_size_callback_trampoline(GLFWwindow* glfwWindow, int width, int height) {
    if (auto* wCallbacks = detail::get_window_callbacks(glfwWindow); wCallbacks->framebufferSizeCB) {
        wCallbacks->framebufferSizeCB(static_cast<std::uint32_t>(width), static_cast<std::uint32_t>(height));
    }
}

namespace detail {

static void set_window_callbacks(GLFWwindow* glfwWindow) {
    RENDERER_ASSERT(glfwWindow);
    glfwSetKeyCallback(glfwWindow, key_callback_trampoline);
    glfwSetCharCallback(glfwWindow, char_callback_trampoline);
    glfwSetCharModsCallback(glfwWindow, char_mods_callback_trampoline);
    glfwSetMouseButtonCallback(glfwWindow, mouse_button_callback_trampoline);
    glfwSetCursorPosCallback(glfwWindow, cursor_pos_callback_trampoline);
    glfwSetCursorEnterCallback(glfwWindow, cursor_enter_callback_trampoline);
    glfwSetScrollCallback(glfwWindow, scroll_callback_trampoline);
    glfwSetDropCallback(glfwWindow, drop_callback_trampoline);

    glfwSetFramebufferSizeCallback(glfwWindow, framebuffer_size_callback_trampoline);
}

InputState* get_input_state(GLFWwindow* glfwWindow) {
    RENDERER_ASSERT(glfwWindow);
    return &get_window_callbacks(glfwWindow)->inputState;
}

void set_key_callback(GLFWwindow* glfwWindow, KeyCB cb) {
    RENDERER_ASSERT(glfwWindow);
    get_window_callbacks(glfwWindow)->key = std::move(cb);
}
void set_char_callback(GLFWwindow* glfwWindow, CharCB cb) {
    RENDERER_ASSERT(glfwWindow);
    get_window_callbacks(glfwWindow)->ch = std::move(cb);
}
void set_char_mods_callback(GLFWwindow* glfwWindow, CharModsCB cb) {
    RENDERER_ASSERT(glfwWindow);
    get_window_callbacks(glfwWindow)->chMods = std::move(cb);
}
void set_mouse_button_callback(GLFWwindow* glfwWindow, MouseBtnCB cb) {
    RENDERER_ASSERT(glfwWindow);
    get_window_callbacks(glfwWindow)->mouseBtn = std::move(cb);
}
void set_cursor_pos_callback(GLFWwindow* glfwWindow, CursorPosCB cb) {
    RENDERER_ASSERT(glfwWindow);
    get_window_callbacks(glfwWindow)->cursorPos = std::move(cb);
}
void set_cursor_enter_callback(GLFWwindow* glfwWindow, CursorEnterCB cb) {
    RENDERER_ASSERT(glfwWindow);
    get_window_callbacks(glfwWindow)->cursorEnter = std::move(cb);
}
void set_scroll_callback(GLFWwindow* glfwWindow, ScrollCB cb) {
    RENDERER_ASSERT(glfwWindow);
    get_window_callbacks(glfwWindow)->scroll = std::move(cb);
}
void set_drop_callback(GLFWwindow* glfwWindow, DropCB cb) {
    RENDERER_ASSERT(glfwWindow);
    get_window_callbacks(glfwWindow)->drop = std::move(cb);
}

void set_framebuffer_size_callback(GLFWwindow* glfwWindow, FramebufferSizeCB cb) {
    RENDERER_ASSERT(glfwWindow);
    get_window_callbacks(glfwWindow)->framebufferSizeCB = std::move(cb);
}

void clear_callbacks(GLFWwindow* glfwWindow) {
    RENDERER_ASSERT(glfwWindow);
    window_callbacks_storage().erase(glfwWindow);
}

} // namespace detail

} // namespace renderer
