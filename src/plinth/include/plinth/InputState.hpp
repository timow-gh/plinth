#ifndef RENDERER_INPUTSTATE_HPP
#define RENDERER_INPUTSTATE_HPP

#include <plinth/Assert.hpp>
#include <compare>
#include <cstdint>
#include <functional>
#include <linal/linal.hpp>

namespace renderer {

enum class Action {
    UNDEFINED = 0, //!< The action is undefined
    RELEASE = 0,   //!< The key or mouse button was released
    PRESS = 1,     //!< The key or mouse button was pressed
    REPEAT = 2     //!< The key is being held down
};

enum class Key {
    KEY_UNKNOWN = -1,
    KEY_SPACE = 32,
    KEY_APOSTROPHE = 39,
    KEY_COMMA = 44,
    KEY_MINUS = 45,
    KEY_PERIOD = 46,
    KEY_SLASH = 47,
    KEY_0 = 48,
    KEY_1 = 49,
    KEY_2 = 50,
    KEY_3 = 51,
    KEY_4 = 52,
    KEY_5 = 53,
    KEY_6 = 54,
    KEY_7 = 55,
    KEY_8 = 56,
    KEY_9 = 57,
    KEY_SEMICOLON = 59,
    KEY_EQUAL = 61,
    KEY_A = 65,
    KEY_B = 66,
    KEY_C = 67,
    KEY_D = 68,
    KEY_E = 69,
    KEY_F = 70,
    KEY_G = 71,
    KEY_H = 72,
    KEY_I = 73,
    KEY_J = 74,
    KEY_K = 75,
    KEY_L = 76,
    KEY_M = 77,
    KEY_N = 78,
    KEY_O = 79,
    KEY_P = 80,
    KEY_Q = 81,
    KEY_R = 82,
    KEY_S = 83,
    KEY_T = 84,
    KEY_U = 85,
    KEY_V = 86,
    KEY_W = 87,
    KEY_X = 88,
    KEY_Y = 89,
    KEY_Z = 90,
    KEY_LEFT_BRACKET = 91,
    KEY_BACKSLASH = 92,
    KEY_RIGHT_BRACKET = 93,
    KEY_GRAVE_ACCENT = 96,
    KEY_WORLD_1 = 161,
    KEY_WORLD_2 = 162,
    KEY_ESCAPE = 256,
    KEY_ENTER = 257,
    KEY_TAB = 258,
    KEY_BACKSPACE = 259,
    KEY_INSERT = 260,
    KEY_DELETE = 261,
    KEY_RIGHT = 262,
    KEY_LEFT = 263,
    KEY_DOWN = 264,
    KEY_UP = 265,
    KEY_PAGE_UP = 266,
    KEY_PAGE_DOWN = 267,
    KEY_HOME = 268,
    KEY_END = 269,
    KEY_CAPS_LOCK = 280,
    KEY_SCROLL_LOCK = 281,
    KEY_NUM_LOCK = 282,
    KEY_PRINT_SCREEN = 283,
    KEY_PAUSE = 284,
    KEY_F1 = 290,
    KEY_F2 = 291,
    KEY_F3 = 292,
    KEY_F4 = 293,
    KEY_F5 = 294,
    KEY_F6 = 295,
    KEY_F7 = 296,
    KEY_F8 = 297,
    KEY_F9 = 298,
    KEY_F10 = 299,
    KEY_F11 = 300,
    KEY_F12 = 301,
    KEY_F13 = 302,
    KEY_F14 = 303,
    KEY_F15 = 304,
    KEY_F16 = 305,
    KEY_F17 = 306,
    KEY_F18 = 307,
    KEY_F19 = 308,
    KEY_F20 = 309,
    KEY_F21 = 310,
    KEY_F22 = 311,
    KEY_F23 = 312,
    KEY_F24 = 313,
    KEY_F25 = 314,
    KEY_KP_0 = 320,
    KEY_KP_1 = 321,
    KEY_KP_2 = 322,
    KEY_KP_3 = 323,
    KEY_KP_4 = 324,
    KEY_KP_5 = 325,
    KEY_KP_6 = 326,
    KEY_KP_7 = 327,
    KEY_KP_8 = 328,
    KEY_KP_9 = 329,
    KEY_KP_DECIMAL = 330,
    KEY_KP_DIVIDE = 331,
    KEY_KP_MULTIPLY = 332,
    KEY_KP_SUBTRACT = 333,
    KEY_KP_ADD = 334,
    KEY_KP_ENTER = 335,
    KEY_KP_EQUAL = 336,
    KEY_LEFT_SHIFT = 340,
    KEY_LEFT_CONTROL = 341,
    KEY_LEFT_ALT = 342,
    KEY_LEFT_SUPER = 343,
    KEY_RIGHT_SHIFT = 344,
    KEY_RIGHT_CONTROL = 345,
    KEY_RIGHT_ALT = 346,
    KEY_RIGHT_SUPER = 347,
    KEY_MENU = 348
};

enum class Mods {
    NONE = 0,               //!< No modifier keys pressed
    SHIFT = 0x0001,         //!< Shift key pressed
    CONTROL = 0x0002,       //!< Control key pressed
    ALT = 0x0004,           //!< Alt key pressed
    SUPER = 0x0008,         //!< Super key pressed (Windows key on Windows, Command key on macOS)
    CAPS_LOCK = 0x0010,     //!< Caps Lock key pressed
    NUM_LOCK = 0x0020       //!< Num Lock key pressed
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

using FramebufferSizeCB = std::function<void(std::uint32_t width, std::uint32_t height)>;

} // namespace renderer

#endif // RENDERER_INPUTSTATE_HPP
