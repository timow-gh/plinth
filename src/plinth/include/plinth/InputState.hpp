#ifndef RENDERER_INPUTSTATE_HPP
#define RENDERER_INPUTSTATE_HPP

#include <plinth/Assert.hpp>
#include <plinth/RendererGlfw.hpp>
#include <compare>
#include <cstdint>
#include <functional>
#include <linal/linal.hpp>

namespace renderer {

enum class Action {
    UNDEFINED = 0,          //!< The action is undefined
    RELEASE = GLFW_RELEASE, //!< The key or mouse button was released
    PRESS = GLFW_PRESS,     //!< The key or mouse button was pressed
    REPEAT = GLFW_REPEAT    //!< The key is being held down
};

enum class Key {
    KEY_UNKNOWN = GLFW_KEY_UNKNOWN,
    KEY_SPACE = GLFW_KEY_SPACE,
    KEY_APOSTROPHE = GLFW_KEY_APOSTROPHE,
    KEY_COMMA = GLFW_KEY_COMMA,
    KEY_MINUS = GLFW_KEY_MINUS,
    KEY_PERIOD = GLFW_KEY_PERIOD,
    KEY_SLASH = GLFW_KEY_SLASH,
    KEY_0 = GLFW_KEY_0,
    KEY_1 = GLFW_KEY_1,
    KEY_2 = GLFW_KEY_2,
    KEY_3 = GLFW_KEY_3,
    KEY_4 = GLFW_KEY_4,
    KEY_5 = GLFW_KEY_5,
    KEY_6 = GLFW_KEY_6,
    KEY_7 = GLFW_KEY_7,
    KEY_8 = GLFW_KEY_8,
    KEY_9 = GLFW_KEY_9,
    KEY_SEMICOLON = GLFW_KEY_SEMICOLON,
    KEY_EQUAL = GLFW_KEY_EQUAL,
    KEY_A = GLFW_KEY_A,
    KEY_B = GLFW_KEY_B,
    KEY_C = GLFW_KEY_C,
    KEY_D = GLFW_KEY_D,
    KEY_E = GLFW_KEY_E,
    KEY_F = GLFW_KEY_F,
    KEY_G = GLFW_KEY_G,
    KEY_H = GLFW_KEY_H,
    KEY_I = GLFW_KEY_I,
    KEY_J = GLFW_KEY_J,
    KEY_K = GLFW_KEY_K,
    KEY_L = GLFW_KEY_L,
    KEY_M = GLFW_KEY_M,
    KEY_N = GLFW_KEY_N,
    KEY_O = GLFW_KEY_O,
    KEY_P = GLFW_KEY_P,
    KEY_Q = GLFW_KEY_Q,
    KEY_R = GLFW_KEY_R,
    KEY_S = GLFW_KEY_S,
    KEY_T = GLFW_KEY_T,
    KEY_U = GLFW_KEY_U,
    KEY_V = GLFW_KEY_V,
    KEY_W = GLFW_KEY_W,
    KEY_X = GLFW_KEY_X,
    KEY_Y = GLFW_KEY_Y,
    KEY_Z = GLFW_KEY_Z,
    KEY_LEFT_BRACKET = GLFW_KEY_LEFT_BRACKET,
    KEY_BACKSLASH = GLFW_KEY_BACKSLASH,
    KEY_RIGHT_BRACKET = GLFW_KEY_RIGHT_BRACKET,
    KEY_GRAVE_ACCENT = GLFW_KEY_GRAVE_ACCENT,
    KEY_WORLD_1 = GLFW_KEY_WORLD_1,
    KEY_WORLD_2 = GLFW_KEY_WORLD_2,
    KEY_ESCAPE = GLFW_KEY_ESCAPE,
    KEY_ENTER = GLFW_KEY_ENTER,
    KEY_TAB = GLFW_KEY_TAB,
    KEY_BACKSPACE = GLFW_KEY_BACKSPACE,
    KEY_INSERT = GLFW_KEY_INSERT,
    KEY_DELETE = GLFW_KEY_DELETE,
    KEY_RIGHT = GLFW_KEY_RIGHT,
    KEY_LEFT = GLFW_KEY_LEFT,
    KEY_DOWN = GLFW_KEY_DOWN,
    KEY_UP = GLFW_KEY_UP,
    KEY_PAGE_UP = GLFW_KEY_PAGE_UP,
    KEY_PAGE_DOWN = GLFW_KEY_PAGE_DOWN,
    KEY_HOME = GLFW_KEY_HOME,
    KEY_END = GLFW_KEY_END,
    KEY_CAPS_LOCK = GLFW_KEY_CAPS_LOCK,
    KEY_SCROLL_LOCK = GLFW_KEY_SCROLL_LOCK,
    KEY_NUM_LOCK = GLFW_KEY_NUM_LOCK,
    KEY_PRINT_SCREEN = GLFW_KEY_PRINT_SCREEN,
    KEY_PAUSE = GLFW_KEY_PAUSE,
    KEY_F1 = GLFW_KEY_F1,
    KEY_F2 = GLFW_KEY_F2,
    KEY_F3 = GLFW_KEY_F3,
    KEY_F4 = GLFW_KEY_F4,
    KEY_F5 = GLFW_KEY_F5,
    KEY_F6 = GLFW_KEY_F6,
    KEY_F7 = GLFW_KEY_F7,
    KEY_F8 = GLFW_KEY_F8,
    KEY_F9 = GLFW_KEY_F9,
    KEY_F10 = GLFW_KEY_F10,
    KEY_F11 = GLFW_KEY_F11,
    KEY_F12 = GLFW_KEY_F12,
    KEY_F13 = GLFW_KEY_F13,
    KEY_F14 = GLFW_KEY_F14,
    KEY_F15 = GLFW_KEY_F15,
    KEY_F16 = GLFW_KEY_F16,
    KEY_F17 = GLFW_KEY_F17,
    KEY_F18 = GLFW_KEY_F18,
    KEY_F19 = GLFW_KEY_F19,
    KEY_F20 = GLFW_KEY_F20,
    KEY_F21 = GLFW_KEY_F21,
    KEY_F22 = GLFW_KEY_F22,
    KEY_F23 = GLFW_KEY_F23,
    KEY_F24 = GLFW_KEY_F24,
    KEY_F25 = GLFW_KEY_F25,
    KEY_KP_0 = GLFW_KEY_KP_0,
    KEY_KP_1 = GLFW_KEY_KP_1,
    KEY_KP_2 = GLFW_KEY_KP_2,
    KEY_KP_3 = GLFW_KEY_KP_3,
    KEY_KP_4 = GLFW_KEY_KP_4,
    KEY_KP_5 = GLFW_KEY_KP_5,
    KEY_KP_6 = GLFW_KEY_KP_6,
    KEY_KP_7 = GLFW_KEY_KP_7,
    KEY_KP_8 = GLFW_KEY_KP_8,
    KEY_KP_9 = GLFW_KEY_KP_9,
    KEY_KP_DECIMAL = GLFW_KEY_KP_DECIMAL,
    KEY_KP_DIVIDE = GLFW_KEY_KP_DIVIDE,
    KEY_KP_MULTIPLY = GLFW_KEY_KP_MULTIPLY,
    KEY_KP_SUBTRACT = GLFW_KEY_KP_SUBTRACT,
    KEY_KP_ADD = GLFW_KEY_KP_ADD,
    KEY_KP_ENTER = GLFW_KEY_KP_ENTER,
    KEY_KP_EQUAL = GLFW_KEY_KP_EQUAL,
    KEY_LEFT_SHIFT = GLFW_KEY_LEFT_SHIFT,
    KEY_LEFT_CONTROL = GLFW_KEY_LEFT_CONTROL,
    KEY_LEFT_ALT = GLFW_KEY_LEFT_ALT,
    KEY_LEFT_SUPER = GLFW_KEY_LEFT_SUPER,
    KEY_RIGHT_SHIFT = GLFW_KEY_RIGHT_SHIFT,
    KEY_RIGHT_CONTROL = GLFW_KEY_RIGHT_CONTROL,
    KEY_RIGHT_ALT = GLFW_KEY_RIGHT_ALT,
    KEY_RIGHT_SUPER = GLFW_KEY_RIGHT_SUPER,
    KEY_MENU = GLFW_KEY_MENU
};

enum class Mods {
    NONE = 0,                       //!< No modifier keys pressed
    SHIFT = GLFW_MOD_SHIFT,         //!< Shift key pressed
    CONTROL = GLFW_MOD_CONTROL,     //!< Control key pressed
    ALT = GLFW_MOD_ALT,             //!< Alt key pressed
    SUPER = GLFW_MOD_SUPER,         //!< Super key pressed (Windows key on Windows, Command key on macOS)
    CAPS_LOCK = GLFW_MOD_CAPS_LOCK, //!< Caps Lock key pressed
    NUM_LOCK = GLFW_MOD_NUM_LOCK    //!< Num Lock key pressed
};

class Scancode {
    int m_scancode{-1}; // Default to -1 (unknown)

  public:
    Scancode() = default;
    explicit Scancode(int scancode)
        : m_scancode(scancode) {
        RENDERER_ASSERT(is_valid());
    }

    int get_value() const { return m_scancode; }

    bool is_valid() const { return m_scancode >= 0; }

    bool operator==(const Scancode& other) const { return m_scancode == other.m_scancode; }
    bool operator!=(const Scancode& other) const { return m_scancode != other.m_scancode; }

    std::strong_ordering operator<=>(const Scancode& other) const { return m_scancode <=> other.m_scancode; }
};

struct KeyState {
    Key key{Key::KEY_UNKNOWN};
    Scancode scancode;
    Action action{Action::UNDEFINED};
    Mods mods{Mods::NONE};
};

class Codepoint {
    std::uint32_t m_codepoint{0}; // Default to 0 (no codepoint)

  public:
    Codepoint() = default;
    explicit Codepoint(std::uint32_t codepoint)
        : m_codepoint(codepoint) {
        RENDERER_ASSERT(is_valid());
    }

    std::uint32_t get_value() const { return m_codepoint; }

    bool is_valid() const { return m_codepoint != 0; }

    bool operator==(const Codepoint& other) const { return m_codepoint == other.m_codepoint; }
    bool operator!=(const Codepoint& other) const { return m_codepoint != other.m_codepoint; }

    std::strong_ordering operator<=>(const Codepoint& other) const { return m_codepoint <=> other.m_codepoint; }
};

struct CharModsState {
    Codepoint codepoint;
    Mods mods{Mods::NONE};
};

struct MouseButtonState {
    int button{0};
    Action action{Action::UNDEFINED};
    Mods mods{Mods::NONE};
};

struct CursorPosState {
    double xpos{0.0};
    double ypos{0.0};
};

struct ScrollState {
    double xoffset{0.0};
    double yoffset{0.0};
};

struct DropState {
    int count{0};
    const char** paths{nullptr};
};

struct InputState {
    KeyState keyState;
    CharModsState charModsState;
    MouseButtonState mouseButtonState;
    CursorPosState cursorPosState;
    bool cursorEnteredState{false};
    ScrollState scrollState;
    DropState dropState;
};

using KeyCB = std::function<void(Key key, Scancode scancode, Action action, Mods mods)>;
using CharCB = std::function<void(std::uint32_t codepoint)>;
using CharModsCB = std::function<void(std::uint32_t codepoint, Mods mods)>;
using MouseBtnCB = std::function<void(int button, Action action, Mods mods)>;
using CursorPosCB = std::function<void(double xpos, double ypos)>;
using CursorEnterCB = std::function<void(bool entered)>;
using ScrollCB = std::function<void(double xoff, double yoff)>;
using DropCB = std::function<void(int count, const char** paths)>;

InputState* get_input_state(GLFWwindow* glfwWindow);

void set_key_callback(GLFWwindow* glfwWindow, KeyCB cb);
void set_char_callback(GLFWwindow* glfwWindow, CharCB cb);
void set_char_mods_callback(GLFWwindow* glfwWindow, CharModsCB cb);
void set_mouse_button_callback(GLFWwindow* glfwWindow, MouseBtnCB cb);
void set_cursor_pos_callback(GLFWwindow* glfwWindow, CursorPosCB cb);
void set_cursor_enter_callback(GLFWwindow* glfwWindow, CursorEnterCB cb);
void set_scroll_callback(GLFWwindow* glfwWindow, ScrollCB cb);
void set_drop_callback(GLFWwindow* glfwWindow, DropCB cb);

using FramebufferSizeCB = std::function<void(std::uint32_t width, std::uint32_t height)>;

void set_framebuffer_size_callback(GLFWwindow* glfwWindow, FramebufferSizeCB cb);

// Call before destroying the glfwWindow
void clear_callbacks(GLFWwindow* glfwWindow);

} // namespace renderer

#endif // RENDERER_INPUTSTATE_HPP
