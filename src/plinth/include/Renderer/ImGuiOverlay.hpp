#ifndef RENDERER_IMGUIOVERLAY_HPP
#define RENDERER_IMGUIOVERLAY_HPP

#include <Renderer/CameraProjectionType.hpp>
#include <Renderer/InputCaptureState.hpp>
#include <Renderer/InputState.hpp>
#include <cstdint>
#include <functional>
#include <vector>

struct GLFWwindow;

namespace renderer {

class ImGuiOverlay {
    GLFWwindow* m_window{nullptr};
    InputCaptureState m_inputCaptureState;

  public:
    explicit ImGuiOverlay(GLFWwindow* window);
    ImGuiOverlay(const ImGuiOverlay&) = delete;
    ImGuiOverlay& operator=(const ImGuiOverlay&) = delete;
    ImGuiOverlay(ImGuiOverlay&&) = delete;
    ImGuiOverlay& operator=(ImGuiOverlay&&) = delete;
    ~ImGuiOverlay();

    // \brief Starts a new ImGui frame. Must be called before adding any controls or rendering the overlay.
    void new_frame();
    // \brief Adds a custom control to the overlay. The provided function will be called every frame when rendering the
    // overlay, allowing the caller to define custom ImGui controls.
    void add_control(std::function<void()> controlFunc);
    // \brief Adds default camera settings to the overlay.
    void add_camera_controls(bool& autoZoomEnabled, CameraProjectionType& projectionType, bool& homeRequested);
    // \brief Renders the ImGui overlay by calling all registered control functions and issuing draw calls.
    void render();
    // \brief If no call to \ref render() occurs, call end_frame to ensure ImGui's internal state is updated correctly.
    void end_frame();

    [[nodiscard]]
    float get_reserved_control_panel_width() const;

    [[nodiscard]]
    bool wants_mouse() const;

    [[nodiscard]]
    bool wants_keyboard() const;
    [[nodiscard]]
    bool handle_cursor_position(double xpos, double ypos);
    [[nodiscard]]
    bool handle_mouse_button(int button, Action action, Mods mods);
    [[nodiscard]]
    bool handle_scroll(double xoffset, double yoffset);
    [[nodiscard]]
    bool handle_key(Key key, Scancode scancode, Action action, Mods mods);
    void handle_char(std::uint32_t codepoint);

  private:
    float m_controlPanelWidth{320.0F};
    std::vector<std::function<void()>> m_controls;
};

} // namespace renderer

#endif // RENDERER_IMGUIOVERLAY_HPP
