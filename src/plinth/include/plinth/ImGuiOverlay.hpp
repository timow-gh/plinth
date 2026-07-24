#ifndef RENDERER_IMGUIOVERLAY_HPP
#define RENDERER_IMGUIOVERLAY_HPP

#include <plinth/CameraPivotMode.hpp>
#include <plinth/CameraProjectionType.hpp>
#include <plinth/InputCaptureState.hpp>
#include <plinth/InputState.hpp>
#include <plinth/PostProcessingEnums.hpp>
#include <array>
#include <cstdint>
#include <functional>
#include <vector>

namespace renderer {

class Renderer;

class ImGuiOverlay {
    void* m_window{nullptr};
    InputCaptureState m_inputCaptureState;

  public:
    explicit ImGuiOverlay(void* nativeWindow);
    ImGuiOverlay(const ImGuiOverlay&) = delete;
    ImGuiOverlay& operator=(const ImGuiOverlay&) = delete;
    ImGuiOverlay(ImGuiOverlay&&) = delete;
    ImGuiOverlay& operator=(ImGuiOverlay&&) = delete;
    ~ImGuiOverlay();

    void new_frame();
    void add_control(std::function<void()> controlFunc);
    void add_camera_controls(bool& autoZoomEnabled,
                             CameraProjectionType& projectionType,
                             CameraPivotMode& pivotMode,
                             bool& homeRequested);
    /// Full debug control surface. Reads current values from the renderer and
    /// routes every change back through its validated set_* methods.
    void add_post_processing_controls(Renderer& renderer);
    /// Minimal, game-like control surface: quality/AA preset, exposure, fog toggle.
    /// Also routes through the renderer's validated set_* methods.
    void add_release_post_processing_controls(Renderer& renderer);
    void build_controls();
    void render();
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
