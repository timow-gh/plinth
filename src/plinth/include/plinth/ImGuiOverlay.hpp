#ifndef RENDERER_IMGUIOVERLAY_HPP
#define RENDERER_IMGUIOVERLAY_HPP

#include "plinth/CameraProjectionType.hpp"
#include "plinth/IOverlay.hpp"
#include "plinth/InputCaptureState.hpp"
#include "plinth/InputState.hpp"
#include "plinth/LightingConfig.hpp"
#include "plinth/PostProcessingEnums.hpp"
#include "plinth/UiMode.hpp"

#include <array>
#include <cstdint>
#include <functional>
#include <vector>

namespace renderer {

class Renderer;

class ImGuiOverlay final : public IOverlay {
    void* m_window{nullptr};
    InputCaptureState m_inputCaptureState;

  public:
    explicit ImGuiOverlay(void* nativeWindow);
    ImGuiOverlay(const ImGuiOverlay&) = delete;
    ImGuiOverlay& operator=(const ImGuiOverlay&) = delete;
    ImGuiOverlay(ImGuiOverlay&&) = delete;
    ImGuiOverlay& operator=(ImGuiOverlay&&) = delete;
    ~ImGuiOverlay() override;

    void new_frame() override;
    /// Builds the built-in control panels (camera + post-processing) into the current
    /// ImGui frame from the given context, then reads user edits back into it. The panel
    /// set is chosen from the current UI mode (Debug vs Release).
    void build_controls(OverlayFrameContext& ctx) override;
    void render() override;
    void end_frame() override;

    /// Selects the built-in post-processing control surface (Debug vs Release).
    void set_ui_mode(UiMode mode) { m_uiMode = mode; }
    [[nodiscard]] UiMode ui_mode() const { return m_uiMode; }

    void add_control(std::function<void()> controlFunc);
    void add_camera_controls(bool& autoZoomEnabled, CameraProjectionType& projectionType, bool& homeRequested);
    /// Edits the caller-owned LightingConfig that is passed to draw(). Every
    /// widget mutates the referenced config in place; the config must outlive
    /// the frame in which the controls are built.
    void add_lighting_controls(LightingConfig& lighting);
    /// Full debug control surface. Reads current values from the renderer and
    /// routes every change back through its validated set_* methods.
    void add_post_processing_controls(Renderer& renderer);
    /// Minimal control surface: independent MSAA and FXAA controls plus exposure.
    /// Also routes through the renderer's validated set_* methods.
    void add_release_post_processing_controls(Renderer& renderer);

    [[nodiscard]] bool wants_mouse() const override;
    [[nodiscard]] bool wants_keyboard() const override;
    [[nodiscard]] bool handle_cursor_position(double xpos, double ypos) override;
    [[nodiscard]] bool handle_mouse_button(int button, Action action, Mods mods) override;
    [[nodiscard]] bool handle_scroll(double xoffset, double yoffset) override;
    [[nodiscard]] bool handle_key(Key key, Scancode scancode, Action action, Mods mods) override;
    void handle_char(std::uint32_t codepoint) override;

  private:
    /// Lays out and renders the accumulated controls into the ImGui frame, then clears
    /// them. Called from render().
    void layout_controls();

    float m_controlPanelWidth{320.0F};
    UiMode m_uiMode{UiMode::Release};
    std::vector<std::function<void()>> m_controls;
};

} // namespace renderer

#endif // RENDERER_IMGUIOVERLAY_HPP
