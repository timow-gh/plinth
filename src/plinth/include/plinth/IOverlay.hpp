#ifndef RENDERER_IOVERLAY_HPP
#define RENDERER_IOVERLAY_HPP

#include "plinth/CameraProjectionType.hpp"
#include "plinth/InputState.hpp"
#include "plinth/LogicalViewportRect.hpp"

#include <cstdint>
#include <optional>

namespace renderer {

class Renderer;

/// Mutable per-frame view handed to the overlay's build_controls(). Fields are read by the
/// overlay to render current state and written back to request changes; the Renderer
/// applies the results after build_controls() returns.
///
/// New overlay-driven state is added here as new fields with sane defaults, WITHOUT
/// changing the IOverlay interface. Existing overlay implementations keep compiling and
/// simply ignore fields they do not use.
struct OverlayFrameContext {
    Renderer& renderer;

    bool autoFitEnabled{false};
    CameraProjectionType projectionType{CameraProjectionType::PERSPECTIVE};
    bool homeRequested{false};
    /// Out: the window-logical region the overlay leaves free for the 3D scene (i.e. the
    /// window minus whatever the UI occupies). The Renderer applies it as the scene viewport
    /// only when the application has not set an explicit viewport via set_scene_viewport().
    /// Overlays that do not reserve space leave this std::nullopt (scene fills the window).
    std::optional<LogicalViewportRect> sceneViewportHint{};
};

/// Contract the Renderer requires from an overlay. The built-in ImGuiOverlay is one
/// implementation; users may supply their own (reusing the renderer's ImGui context or a
/// different toolkit entirely), or none at all.
class IOverlay {
  public:
    IOverlay() = default;
    IOverlay(const IOverlay&) = delete;
    IOverlay& operator=(const IOverlay&) = delete;
    IOverlay(IOverlay&&) = delete;
    IOverlay& operator=(IOverlay&&) = delete;
    virtual ~IOverlay() = default;

    // --- Frame lifecycle (called by Renderer::end_frame, after present_scene()) ---
    virtual void new_frame() = 0;
    virtual void build_controls(OverlayFrameContext& ctx) = 0;
    virtual void render() = 0;
    virtual void end_frame() = 0;

    // --- Input arbitration ---
    // The handle_* methods return true when the event should be forwarded to the
    // camera/application callbacks, and false when the overlay consumed it.
    [[nodiscard]] virtual bool wants_mouse() const = 0;
    [[nodiscard]] virtual bool wants_keyboard() const = 0;
    [[nodiscard]] virtual bool handle_cursor_position(double xpos, double ypos) = 0;
    [[nodiscard]] virtual bool handle_mouse_button(int button, Action action, Mods mods) = 0;
    [[nodiscard]] virtual bool handle_scroll(double xoffset, double yoffset) = 0;
    [[nodiscard]] virtual bool handle_key(Key key, Scancode scancode, Action action, Mods mods) = 0;
    virtual void handle_char(std::uint32_t codepoint) = 0;
};

} // namespace renderer

#endif // RENDERER_IOVERLAY_HPP
