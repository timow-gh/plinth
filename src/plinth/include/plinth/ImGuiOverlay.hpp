#ifndef RENDERER_IMGUIOVERLAY_HPP
#define RENDERER_IMGUIOVERLAY_HPP

#include <plinth/CameraProjectionType.hpp>
#include <plinth/InputCaptureState.hpp>
#include <plinth/InputState.hpp>
#include <plinth/PostProcessingEnums.hpp>
#include <array>
#include <cstdint>
#include <functional>
#include <vector>

namespace renderer {

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
    void add_camera_controls(bool& autoZoomEnabled, CameraProjectionType& projectionType, bool& homeRequested);
    void add_post_processing_controls(
        float& exposureStops,
        renderer::ToneMapMode& toneMapMode,
        bool& fogEnabled,
        renderer::FogMode& fogMode,
        float& fogStart,
        float& fogEnd,
        float& fogDensity,
        std::array<float, 3>& fogColor,
        renderer::VisualizationMode& visualizationMode,
        float& hdrDisplayMax,
        bool& grayscale,
        bool& fxaaEnabled,
        float& fxaaEdgeThreshold,
        float& fxaaEdgeThresholdMin,
        float& fxaaSubpixelAmount);
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
