#ifndef RENDERER_INPUTCAPTURESTATE_HPP
#define RENDERER_INPUTCAPTURESTATE_HPP

#include <Renderer/InputState.hpp>
#include <cstdint>

namespace renderer {

class InputCaptureState {
    static constexpr int maxTrackedMouseButtonCount = 32;

    std::uint32_t m_guiOwnedMouseButtons{0};
    std::uint32_t m_appOwnedMouseButtons{0};

  public:
    [[nodiscard]]
    bool should_forward_mouse_button(int button, Action action, bool guiWantsMouse) {
        const std::uint32_t mask = button_mask(button);
        if (mask == 0U) {
            return !guiWantsMouse;
        }

        if (action == Action::PRESS) {
            if (guiWantsMouse) {
                m_guiOwnedMouseButtons |= mask;
                return false;
            }

            m_appOwnedMouseButtons |= mask;
            return true;
        }

        if (action == Action::RELEASE) {
            const bool guiOwned = (m_guiOwnedMouseButtons & mask) != 0U;
            const bool appOwned = (m_appOwnedMouseButtons & mask) != 0U;
            m_guiOwnedMouseButtons &= ~mask;
            m_appOwnedMouseButtons &= ~mask;

            if (guiOwned) {
                return false;
            }
            if (appOwned) {
                return true;
            }
            return !guiWantsMouse;
        }

        return !guiWantsMouse;
    }

    [[nodiscard]]
    bool should_forward_cursor_position(bool guiWantsMouse) const {
        if (m_appOwnedMouseButtons != 0U) {
            return true;
        }
        if (m_guiOwnedMouseButtons != 0U) {
            return false;
        }
        return !guiWantsMouse;
    }

    [[nodiscard]]
    bool should_forward_scroll(bool guiWantsMouse) const {
        return m_guiOwnedMouseButtons == 0U && !guiWantsMouse;
    }

    [[nodiscard]]
    static bool should_forward_key(bool guiWantsKeyboard) {
        return !guiWantsKeyboard;
    }

  private:
    [[nodiscard]]
    static std::uint32_t button_mask(int button) {
        if (button < 0 || button >= maxTrackedMouseButtonCount) {
            return 0U;
        }
        return 1U << static_cast<std::uint32_t>(button);
    }
};

} // namespace renderer

#endif // RENDERER_INPUTCAPTURESTATE_HPP
