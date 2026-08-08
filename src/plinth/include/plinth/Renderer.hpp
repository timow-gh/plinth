#ifndef RENDERER_RENDERER_HPP
#define RENDERER_RENDERER_HPP

#include "linal/vec.hpp"
#include "plinth/BufferAccessPattern.hpp"
#include "plinth/CameraAutoFit.hpp"
#include "plinth/CameraInteractor.hpp"
#include "plinth/DashSpace.hpp"
#include "plinth/FrameState.hpp"
#include "plinth/GlfwWindow.hpp"
#include "plinth/IOverlay.hpp"
#include "plinth/InputState.hpp"
#include "plinth/LightingConfig.hpp"
#include "plinth/LineType.hpp"
#include "plinth/LogicalViewportRect.hpp"
#include "plinth/MeshCullFaceMode.hpp"
#include "plinth/PostProcessingEnums.hpp"
#include "plinth/StrokeStyle.hpp"
#include "plinth/Texture.hpp"
#include "plinth/WindowSettings.hpp"
#include "plinth/loader/MeshData.hpp"

#include <array>
#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <utility>
#include <vector>

namespace opengl {
class DrawablesManager;
class Framebuffer;
class PostProcessingPass;
class FXAAPass;
} // namespace opengl

namespace renderer {

enum class DrawableKind {
    invalid,
    point,
    line,
    mesh,
};

struct DrawableHandle {
    DrawableKind kind{DrawableKind::invalid};
    std::uint64_t id{0U};
    /// Opaque identity of the Renderer that created this handle. Handles are
    /// renderer-specific; invalid, foreign, removed, and stale handles are rejected.
    std::uint64_t rendererInstance{0U};

    [[nodiscard]] constexpr bool is_valid() const {
        return kind != DrawableKind::invalid && id != 0U && rendererInstance != 0U;
    }
};

struct CallbackConnection;

class CallbackSubscription {
  public:
    CallbackSubscription() = default;
    CallbackSubscription(const CallbackSubscription&) = delete;
    CallbackSubscription& operator=(const CallbackSubscription&) = delete;
    CallbackSubscription(CallbackSubscription&& other) noexcept;
    CallbackSubscription& operator=(CallbackSubscription&& other) noexcept;
    ~CallbackSubscription();

    /// Disconnecting is idempotent. A disconnected callback is skipped even if
    /// dispatch is in progress.
    void disconnect() noexcept;
    [[nodiscard]] bool is_connected() const noexcept;

  private:
    friend class Renderer;
    explicit CallbackSubscription(std::shared_ptr<CallbackConnection> connection);

    std::shared_ptr<CallbackConnection> m_connection;
};

struct SceneViewport {
    LogicalViewportRect logical;
    renderer::ViewportRect framebuffer;
};

class Renderer {
  public:
    Renderer() = delete;
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;
    Renderer(Renderer&&) = delete;
    Renderer& operator=(Renderer&&) = delete;
    ~Renderer();

    /// Only one live Renderer is supported at a time. A second call to create()
    /// while another Renderer is alive returns nullptr. A new instance may be
    /// created after the previous owner is destroyed.
    [[nodiscard]] static std::unique_ptr<Renderer> create(const WindowSettings& settings);
    /// Computes the scene viewport layout from window and framebuffer dimensions.
    /// The given logical rect (in window coordinates) defines the area the scene
    /// occupies; the framebuffer viewport is that rect mapped to pixel coordinates.
    [[nodiscard]] static SceneViewport calculate_scene_viewport(std::pair<int, int> windowSize,
                                                                std::pair<int, int> framebufferSize,
                                                                const LogicalViewportRect& logicalRect);
    /// Converts window cursor coordinates to the scene framebuffer coordinate
    /// system. Returns std::nullopt when the position lies outside the scene
    /// logical viewport.
    [[nodiscard]] static std::optional<std::pair<double, double>>
    to_scene_framebuffer_coordinates(const SceneViewport& sceneViewport, double xpos, double ypos);

    /// Sets the scene viewport in logical (window) coordinates. Passing std::nullopt (the
    /// default) makes the scene fill the whole window. The renderer maps this rect to
    /// framebuffer pixels and keeps camera aspect and input picking consistent. Applies on
    /// the next begin_frame().
    void set_scene_viewport(std::optional<LogicalViewportRect> logicalRect);
    /// The scene viewport currently in effect (framebuffer + logical rects).
    [[nodiscard]] SceneViewport scene_viewport() const { return m_sceneViewport; }

    DrawableHandle add_point_drawable(std::span<const float> vertices,
                                      std::array<float, 4> color,
                                      float pointSize = 2.0F,
                                      BufferAccessPattern accessPattern = BufferAccessPattern::Static);

    DrawableHandle add_point_drawable(std::span<const float> vertices,
                                      std::span<const float> colors,
                                      float pointSize = 2.0F,
                                      BufferAccessPattern accessPattern = BufferAccessPattern::Static);

    DrawableHandle add_point_drawable(std::span<const float> vertices,
                                      std::span<const std::uint32_t> indices,
                                      std::span<const float> colors,
                                      float pointSize = 2.0F,
                                      BufferAccessPattern accessPattern = BufferAccessPattern::Static);

    DrawableHandle add_line_drawable(std::span<const float> vertices,
                                     std::span<const std::uint32_t> indices,
                                     std::span<const float> colors,
                                     renderer::LineType lineType,
                                     const renderer::StrokeStyle& style = {},
                                     float pointSize = 0.0F,
                                     BufferAccessPattern accessPattern = BufferAccessPattern::Static,
                                     std::span<const std::uint8_t> perVertexDashFlags = {});

    DrawableHandle add_line_drawable(std::span<const float> vertices,
                                     std::array<float, 4> color,
                                     renderer::LineType lineType,
                                     const renderer::StrokeStyle& style = {},
                                     float pointSize = 0.0F,
                                     BufferAccessPattern accessPattern = BufferAccessPattern::Static);

    DrawableHandle add_line_drawable(std::span<const float> vertices,
                                     std::span<const float> colors,
                                     renderer::LineType lineType,
                                     const renderer::StrokeStyle& style = {},
                                     float pointSize = 0.0F,
                                     BufferAccessPattern accessPattern = BufferAccessPattern::Static);

    DrawableHandle add_mesh_drawable(std::span<const float> vertices,
                                     std::span<const std::uint32_t> triangleIndices,
                                     std::array<float, 4> color,
                                     MeshCullFaceMode cullMode = MeshCullFaceMode::BACK,
                                     BufferAccessPattern accessPattern = BufferAccessPattern::Static);

    DrawableHandle add_mesh_drawable(std::span<const float> vertices,
                                     std::span<const std::uint32_t> triangleIndices,
                                     std::span<const float> colors,
                                     MeshCullFaceMode cullMode = MeshCullFaceMode::BACK,
                                     BufferAccessPattern accessPattern = BufferAccessPattern::Static);

    DrawableHandle add_mesh_drawable(std::span<const float> vertices,
                                     std::span<const std::uint32_t> triangleIndices,
                                     std::array<float, 4> color,
                                     std::span<const float> normals,
                                     MeshCullFaceMode cullMode = MeshCullFaceMode::BACK,
                                     BufferAccessPattern accessPattern = BufferAccessPattern::Static);

    DrawableHandle add_mesh_drawable(std::span<const float> vertices,
                                     std::span<const std::uint32_t> triangleIndices,
                                     std::span<const float> normals,
                                     std::span<const float> colors,
                                     MeshCullFaceMode cullMode = MeshCullFaceMode::BACK,
                                     BufferAccessPattern accessPattern = BufferAccessPattern::Static);

    DrawableHandle add_mesh_drawable(const renderer::MeshData& mesh,
                                     std::array<float, 4> color = {0.8F, 0.8F, 0.8F, 1.0F},
                                     MeshCullFaceMode cullMode = MeshCullFaceMode::BACK,
                                     BufferAccessPattern accessPattern = BufferAccessPattern::Static);

    /// TextureData::rgba8 is copied.
    TextureHandle create_texture_2d(TextureData data);
    bool remove_texture(TextureHandle texture);
    /// Returns an invalid handle when creation fails. The texture must outlive
    /// this drawable or be re-registered before removal.
    DrawableHandle add_textured_mesh_drawable(std::span<const float> vertices,
                                              std::span<const float> normals,
                                              std::span<const float> textureCoordinates,
                                              std::span<const float> colors,
                                              std::span<const std::uint32_t> triangleIndices,
                                              TextureHandle texture,
                                              BufferAccessPattern accessPattern = BufferAccessPattern::Static);

    /// Invalid, foreign, removed, and stale handles leave state unchanged.
    void set_mesh_drawable_cull_mode(DrawableHandle handle, MeshCullFaceMode mode);

    /// Returns false for invalid, foreign, removed, or stale handles.
    bool remove_drawable(DrawableHandle handle);

    /// Transform operations return false (or std::nullopt) for invalid, foreign,
    /// removed, and stale handles; reset restores the identity transform.
    bool set_drawable_transform(DrawableHandle handle, const linal::hmatf& transform);
    [[nodiscard]] std::optional<linal::hmatf> get_drawable_transform(DrawableHandle handle) const;
    bool reset_drawable_transform(DrawableHandle handle);

    /// Line style controls. Return false for invalid, foreign, removed, non-line, or stale handles.
    bool set_line_cap(DrawableHandle handle, renderer::LineCap cap);
    bool set_line_join(DrawableHandle handle, renderer::LineJoin join);
    /// Replaces the full stroke style (width, cap, join, dash pattern, phase, space).
    bool set_line_stroke_style(DrawableHandle handle, const renderer::StrokeStyle& style);

    /// Dash controls. dashPattern uses SVG stroke-dasharray semantics: alternating on/off lengths.
    /// Empty pattern = solid. The phase animates "marching ants" when advanced over time.
    bool set_line_dash_pattern(DrawableHandle handle, std::span<const float> pattern);
    bool set_line_dash_phase(DrawableHandle handle, float phase);
    bool set_line_dash_space(DrawableHandle handle, renderer::DashSpace space);

    /// Convenience shims for backward compatibility.
    bool set_line_dash_enabled(DrawableHandle handle, bool enabled);
    bool set_line_dash(DrawableHandle handle, float dashSize, float gapSize);

    /// Updates affect the most recently added drawable of that kind. If none
    /// exists, the call is ignored. Input spans are copied during the call.
    void update_last_point_drawable(std::span<const float> vertices,
                                    std::span<const float> colors,
                                    std::span<const std::uint32_t> indices,
                                    BufferAccessPattern accessPattern);

    void update_last_line_drawable(std::span<const float> vertices,
                                   std::span<const float> colors,
                                   std::span<const std::uint32_t> indices,
                                   BufferAccessPattern accessPattern);

    /// Removes all drawables of the given kind. Handles previously returned for
    /// those drawables become invalid and are rejected by subsequent operations.
    void clear_point_drawables();
    void clear_line_drawables();
    void clear_mesh_drawables();
    /// Removes all drawables regardless of kind. All previously-returned handles
    /// become invalid.
    void clear_drawables();

    [[nodiscard]] bool has_point_drawables() const;
    [[nodiscard]] bool has_line_drawables() const;
    [[nodiscard]] bool has_mesh_drawables() const;

    struct PickRay {
        linal::float3 origin;
        linal::float3 direction;
    };
    struct PickResult {
        DrawableHandle handle;
        PickRay ray;
    };

    [[nodiscard]] std::vector<PickResult> pick_drawables(double xpos, double ypos, double radius) const;

    [[nodiscard]] PickRay compute_pick_ray(double xpos, double ypos) const;

    /// Post-processing controls require finite numeric values. HDR display max
    /// must be positive, fog density must be non-negative, and linear fog needs
    /// end > start. FXAA edge threshold, minimum edge contrast, and subpixel
    /// amount are constrained to [0, 0.5], [0, 0.25], and [0, 1]. Invalid input
    /// is rejected without changing state and reported through the error sink.
    void set_exposure_stops(float stops);
    void set_tone_map_mode(renderer::ToneMapMode mode);
    void set_fog_enabled(bool enabled);
    void set_fog_mode(renderer::FogMode mode);
    void set_fog_start(float start);
    void set_fog_end(float end);
    void set_fog_density(float density);
    void set_fog_color(float r, float g, float b);
    void set_visualization_mode(renderer::VisualizationMode mode);
    void set_hdr_display_max(float maxVal);
    void set_grayscale(bool enabled);
    void set_fxaa_enabled(bool enabled);
    void set_fxaa_edge_threshold(float threshold);
    void set_fxaa_edge_threshold_min(float threshold);
    void set_fxaa_subpixel_amount(float amount);

    /// Current post-processing state. These mirror the values applied by the
    /// pipeline and are used by the ImGui overlay to render its controls.
    [[nodiscard]] float get_exposure_stops() const { return m_exposureStops; }
    [[nodiscard]] renderer::ToneMapMode get_tone_map_mode() const { return m_toneMapMode; }
    [[nodiscard]] bool get_fog_enabled() const { return m_fogEnabled; }
    [[nodiscard]] renderer::FogMode get_fog_mode() const { return m_fogMode; }
    [[nodiscard]] float get_fog_start() const { return m_fogStart; }
    [[nodiscard]] float get_fog_end() const { return m_fogEnd; }
    [[nodiscard]] float get_fog_density() const { return m_fogDensity; }
    [[nodiscard]] std::array<float, 3> get_fog_color() const { return {m_fogColorR, m_fogColorG, m_fogColorB}; }
    [[nodiscard]] renderer::VisualizationMode get_visualization_mode() const { return m_visualizationMode; }
    [[nodiscard]] float get_hdr_display_max() const { return m_hdrDisplayMax; }
    [[nodiscard]] bool get_grayscale() const { return m_grayscale; }
    [[nodiscard]] bool get_fxaa_enabled() const { return m_fxaaEnabled; }
    [[nodiscard]] float get_fxaa_edge_threshold() const { return m_fxaaEdgeThreshold; }
    [[nodiscard]] float get_fxaa_edge_threshold_min() const { return m_fxaaEdgeThresholdMin; }
    [[nodiscard]] float get_fxaa_subpixel_amount() const { return m_fxaaSubpixelAmount; }

    /// Replaces the active overlay. Passing nullptr removes any overlay (the frame loop and
    /// input routing then run with no UI). The caller may retain a co-owning handle to the
    /// overlay to drive its concrete features (e.g. the built-in ImGuiOverlay's UiMode); the
    /// Renderer itself drives it only through the IOverlay interface.
    void set_overlay(std::shared_ptr<IOverlay> overlay);

    /// Renderer owns one GLFW/OpenGL context. All methods that access the window,
    /// renderer state, or GL must be called on its creating thread. Frame methods
    /// make that context current before GL work. Per frame, call poll_events(),
    /// begin_frame(), draw(), then end_frame(); repeated draw() calls add work to
    /// the same frame, while end_frame() presents and swaps buffers once per call.
    static void poll_events();
    [[nodiscard]]
    /// Returns true when the GLFW window has received a close request.
    bool should_close() const;
    [[nodiscard]]
    /// Returns true when the Escape key was pressed this frame.
    bool is_escape_pressed() const;

    void begin_frame(const renderer::ClearColor& clearColor = defaultClearColor);
    void draw();
    void draw(const renderer::LightingConfig& lighting);
    void end_frame();
    // Caller owns auto-fit state for these overloads. A Home request is consumed
    // before return. The no-argument overload retains renderer-owned auto-fit state.
    void end_frame(bool& autoFitEnabled);
    void end_frame(bool& autoFitEnabled, bool& homeRequested);

    /// Makes the renderer-owned context current on the calling thread.
    void make_context_current() const;

    // --- Camera navigation (geometry-fit aware) ---
    /// Schedules one geometry-aware camera fit for the next eligible
    /// begin_frame(). Requests coalesce and remain pending while automatic
    /// fitting is disabled or a camera view transition is active.
    void request_auto_fit() noexcept;
    /// Replaces renderer-owned automatic-fit configuration and schedules a
    /// fit. Automatic fitting remains disabled by default for a new Renderer.
    void set_camera_auto_fit_settings(const CameraAutoFitSettings& settings);
    [[nodiscard]] CameraAutoFitSettings get_camera_auto_fit_settings() const noexcept {
        return m_cameraAutoFitSettings;
    }
    /// Controls the scene-radius padding used for fitted far clip planes and
    /// schedules a fit. Callers are responsible for providing a valid value.
    void set_camera_far_plane_multiplier(double multiplier);
    [[nodiscard]] double get_camera_far_plane_multiplier() const noexcept { return m_cameraFarPlaneMultiplier; }
    /// Animates the camera to a preset view orientation.
    void go_to_preset_view(PresetView view);
    /// Animates the camera to the home (initial) view.
    void go_to_home_view();
    /// Refits all geometry into view along the camera's CURRENT viewing direction, animating to the
    /// fitted pose immediately. Unlike go_to_home_view (which resets to the default direction), this
    /// preserves the current orientation - e.g. a front/top/iso preset stays put and is only
    /// re-framed. Unlike enabling auto-fit, it moves the camera unconditionally, independent of the
    /// auto-fit-enabled setting and the post-interaction suppression window. No-op if there is no
    /// geometry to fit.
    void refit_current_view();

    // --- Callback extension points ---
    /// Keep the returned subscription alive for the callback lifetime. Destroying
    /// or disconnecting it removes the callback. During dispatch, disconnected
    /// callbacks are skipped and callbacks added during dispatch wait until later.
    /// Cursor callbacks receive scene framebuffer coordinates and are suppressed
    /// outside the scene or when ImGui captures the event. Scroll, mouse, and key
    /// callbacks are likewise suppressed while ImGui captures their input.
    [[nodiscard]] CallbackSubscription add_cursor_pos_callback(CursorPosCB cb);
    [[nodiscard]] CallbackSubscription add_scroll_callback(ScrollCB cb);
    [[nodiscard]] CallbackSubscription add_mouse_button_callback(MouseBtnCB cb);
    [[nodiscard]] CallbackSubscription add_key_callback(KeyCB cb);

    // --- Accessors ---
    [[nodiscard]] [[nodiscard]] const GlfwWindow& window() const { return m_window; }
    [[nodiscard]] std::weak_ptr<CameraInteractor> get_camera() { return m_camera; }
    [[nodiscard]] std::weak_ptr<const CameraInteractor> get_camera() const { return m_camera; }
    [[nodiscard]] bool is_auto_fit_enabled() const noexcept { return m_cameraAutoFitSettings.enabled; }
    /// True when GL_ARB_clip_control or OpenGL 4.5 support allowed the renderer
    /// to select a floating-point reversed-Z pipeline.
    [[nodiscard]] bool uses_reversed_depth() const noexcept { return m_reversedDepth; }
    static constexpr renderer::ClearColor defaultClearColor{0.05F, 0.05F, 0.05F, 1.0F};

  private:
    Renderer(GlfwWindow window,
             std::unique_ptr<opengl::DrawablesManager> drawablesManager,
             std::shared_ptr<CameraInteractor> camera,
             std::shared_ptr<IOverlay> overlay,
             std::unique_ptr<opengl::Framebuffer> sceneFramebuffer,
             std::unique_ptr<opengl::Framebuffer> hdrResolveFramebuffer,
             std::unique_ptr<opengl::Framebuffer> ldrIntermediate,
             std::unique_ptr<opengl::PostProcessingPass> postProcessingPass,
             std::unique_ptr<opengl::FXAAPass> fxaaPass,
             int sceneSamples,
             int maxTextureSize,
             int maxAnisotropy,
             bool reversedDepth,
             std::uint64_t rendererInstance);

    void wire_callbacks();
    void update_scene_viewport();
    void present_scene();
    void on_cursor_pos(double xpos, double ypos);
    void on_scroll(double xoff, double yoff);
    void on_mouse_button(int button, Action action, Mods mods);
    [[nodiscard]] std::optional<std::pair<double, double>> current_scene_framebuffer_coordinates() const;

    [[nodiscard]] CameraAutoFitResult compute_fit_destination(const linal::double3& direction,
                                                              const linal::double3& up,
                                                              const linal::double3& targetHint,
                                                              double currentDistance,
                                                              bool suppressZoomIn = false) const;
    void maybe_update_auto_fit(std::chrono::steady_clock::time_point now);
    void apply_fit_result(const CameraAutoFitResult& result);
    // Refits near/far clip planes to the current geometry at the camera's
    // current pose. Runs every frame and never moves the camera.
    void update_clip_planes_from_bounds();
    // Applies near/far to the camera in an order that keeps near < far valid
    // through the intermediate setter call.
    void apply_clip_planes(double nearPlane, double farPlane);

    template <typename Callback>
    struct CallbackEntry {
        Callback callback;
        std::shared_ptr<CallbackConnection> connection;
    };

    template <typename Callback>
    CallbackSubscription add_callback(std::vector<CallbackEntry<Callback>>& callbacks, Callback callback);
    template <typename Callback, typename... Args>
    void dispatch_callbacks(std::vector<CallbackEntry<Callback>>& callbacks, Args&&... args);

    GlfwWindow m_window;
    std::unique_ptr<opengl::DrawablesManager> m_drawablesManager;
    std::shared_ptr<CameraInteractor> m_camera;
    /// Active overlay (may be null). The Renderer drives it only through the IOverlay
    /// interface and holds no knowledge of the concrete overlay type.
    std::shared_ptr<IOverlay> m_overlay;
    std::unique_ptr<opengl::Framebuffer> m_sceneFramebuffer;
    std::unique_ptr<opengl::Framebuffer> m_hdrResolveFramebuffer;
    std::unique_ptr<opengl::Framebuffer> m_ldrIntermediate;
    /// Lazily-created single-sample color target for GPU color-ID picking. Created/resized on the
    /// first pick_drawables call and reused thereafter. Mutable because pick_drawables is const.
    mutable std::unique_ptr<opengl::Framebuffer> m_pickFramebuffer;
    std::unique_ptr<opengl::PostProcessingPass> m_postProcessingPass;
    std::unique_ptr<opengl::FXAAPass> m_fxaaPass;
    int m_sceneSamples{1};
    SceneViewport m_sceneViewport;
    /// Application-requested scene viewport in logical coordinates, set via
    /// set_scene_viewport(). std::nullopt means the app has not requested one; the scene
    /// then falls back to the overlay-reserved region, or the whole window.
    std::optional<LogicalViewportRect> m_requestedSceneViewport;
    /// Scene viewport region reported by the active overlay (the window minus the UI it
    /// occupies). Applied only when m_requestedSceneViewport is empty. std::nullopt when no
    /// overlay reserves space.
    std::optional<LogicalViewportRect> m_overlaySceneViewport;
    CursorPosState m_lastWindowCursorPos;
    bool m_cameraMouseInteractionActive{false};
    int m_cameraMouseInteractionButton{-1};
    std::chrono::steady_clock::time_point m_lastFrameTime;
    std::optional<std::chrono::steady_clock::time_point> m_lastCameraInteractionTime;
    bool m_autoFitPending{false};
    CameraAutoFitSettings m_cameraAutoFitSettings;
    double m_cameraFarPlaneMultiplier{3.0};
    int m_maxTextureSize{0};
    int m_maxAnisotropy{1};
    bool m_reversedDepth{false};
    std::uint64_t m_rendererInstance{0U};

    std::vector<CallbackEntry<CursorPosCB>> m_cursorPosCallbacks;
    std::vector<CallbackEntry<ScrollCB>> m_scrollCallbacks;
    std::vector<CallbackEntry<MouseBtnCB>> m_mouseButtonCallbacks;
    std::vector<CallbackEntry<KeyCB>> m_keyCallbacks;

    float m_exposureStops{0.0f};
    renderer::ToneMapMode m_toneMapMode{renderer::ToneMapMode::None};
    bool m_fogEnabled{false};
    renderer::FogMode m_fogMode{renderer::FogMode::Linear};
    float m_fogStart{5.0f};
    float m_fogEnd{50.0f};
    float m_fogDensity{0.05f};
    float m_fogColorR{0.05f};
    float m_fogColorG{0.05f};
    float m_fogColorB{0.08f};
    renderer::VisualizationMode m_visualizationMode{renderer::VisualizationMode::Final};
    float m_hdrDisplayMax{10.0f};
    bool m_grayscale{false};
    bool m_fxaaEnabled{true};
    float m_fxaaEdgeThreshold{0.166f};
    float m_fxaaEdgeThresholdMin{0.0833f};
    float m_fxaaSubpixelAmount{0.75f};
};

} // namespace renderer

#endif // RENDERER_RENDERER_HPP
