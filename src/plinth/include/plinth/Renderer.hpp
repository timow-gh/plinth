#ifndef RENDERER_RENDERER_HPP
#define RENDERER_RENDERER_HPP

#include <array>
#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>
#include <plinth/BufferAccessPattern.hpp>
#include <plinth/CameraAutoFit.hpp>
#include <plinth/CameraInteractor.hpp>
#include <plinth/FrameState.hpp>
#include <plinth/GlfwWindow.hpp>
#include <plinth/ImGuiOverlay.hpp>
#include <plinth/InputState.hpp>
#include <plinth/LightingConfig.hpp>
#include <plinth/LineType.hpp>
#include <plinth/PostProcessingEnums.hpp>
#include <plinth/Texture.hpp>
#include <plinth/UiMode.hpp>
#include <plinth/WindowSettings.hpp>
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
    meshSegment,
    meshVertex,
};

struct DrawableHandle {
    DrawableKind kind{DrawableKind::invalid};
    std::uint64_t id{0U};
    /// Opaque identity of the Renderer that created this handle. Handles are
    /// renderer-specific; invalid, foreign, removed, and stale handles are rejected.
    std::uint64_t rendererInstance{0U};

    [[nodiscard]]
    constexpr bool is_valid() const {
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

struct LogicalViewportRect {
    double x{0.0};
    double y{0.0};
    double width{1.0};
    double height{1.0};

    [[nodiscard]]
    bool contains(double xpos, double ypos) const {
        return xpos >= x && ypos >= y && xpos < x + width && ypos < y + height;
    }
};

struct SceneViewport {
    LogicalViewportRect logical;
    renderer::ViewportRect framebuffer;
};

class ImGuiOverlayView {
  public:
    [[nodiscard]]
    ImGuiOverlay* lock() const {
        return m_lifetime.expired() ? nullptr : m_overlay;
    }

  private:
    friend class Renderer;
    ImGuiOverlayView(ImGuiOverlay* overlay, std::weak_ptr<void> lifetime)
        : m_overlay(overlay), m_lifetime(std::move(lifetime)) {}

    ImGuiOverlay* m_overlay{nullptr};
    std::weak_ptr<void> m_lifetime;
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
    [[nodiscard]]
    static std::unique_ptr<Renderer> create(const WindowSettings& settings);
    /// Computes the scene viewport layout from window and framebuffer dimensions.
    /// The logical viewport reserves the given width for UI; the framebuffer
    /// viewport covers the remaining area in pixel coordinates.
    [[nodiscard]]
    static SceneViewport calculate_scene_viewport(std::pair<int, int> windowSize,
                                                  std::pair<int, int> framebufferSize,
                                                  double reservedLogicalWidth);
    /// Converts window cursor coordinates to the scene framebuffer coordinate
    /// system. Returns std::nullopt when the position lies outside the scene
    /// logical viewport.
    [[nodiscard]]
    static std::optional<std::pair<double, double>>
    to_scene_framebuffer_coordinates(const SceneViewport& sceneViewport, double xpos, double ypos);

    /// Geometry input uses three floats per position or normal, four floats per
    /// color, and two floats per texture coordinate. Indices address vertices and
    /// use pairs for lines or triples for triangles. Input spans are copied during
    /// the call and may be released afterwards. All add_*_drawable functions return
    /// an invalid handle when creation fails; they do not throw.
    DrawableHandle
    add_point_drawable(std::span<const float> vertices,
                       std::span<const float> colors,
                       std::span<const std::uint32_t> indices,
                       float pointSize,
                        renderer::BufferAccessPattern accessPattern = renderer::BufferAccessPattern::Static);

    DrawableHandle
    add_line_drawable(std::span<const float> vertices,
                      std::span<const std::uint32_t> indices,
                      std::span<const float> colors,
                      renderer::LineType lineType,
                      float lineWidth,
                      float pointSize = 0.0F,
                       renderer::BufferAccessPattern accessPattern = renderer::BufferAccessPattern::Static);

    /// Returns an invalid handle when creation fails. Does not set a texture;
    /// use add_textured_mesh_drawable for textured geometry.
    DrawableHandle
    add_mesh_drawable(std::span<const float> vertices,
                      std::span<const float> normals,
                      std::span<const float> colors,
                      std::span<const std::uint32_t> triangleIndices,
                       renderer::BufferAccessPattern accessPattern = renderer::BufferAccessPattern::Static);

    /// Returns an invalid handle when creation fails. TextureData::rgba8 is copied.
    TextureHandle create_texture_2d(TextureData data);
    /// Returns false for invalid, foreign, removed, or stale handles.
    bool remove_texture(TextureHandle texture);
    /// Returns an invalid handle when creation fails. The texture must outlive
    /// this drawable or be re-registered before removal.
    DrawableHandle add_textured_mesh_drawable(
        std::span<const float> vertices,
        std::span<const float> normals,
        std::span<const float> textureCoordinates,
        std::span<const float> colors,
        std::span<const std::uint32_t> triangleIndices,
        TextureHandle texture,
         renderer::BufferAccessPattern accessPattern = renderer::BufferAccessPattern::Static);

    /// Returns an invalid handle when creation fails. Input spans are copied.
    DrawableHandle add_mesh_segment_drawable(std::span<const float> positions,
                                             std::span<const std::uint32_t> indices,
                                             std::span<const float> color,
                                             float lineWidth);

    /// Returns an invalid handle when creation fails. Input spans are copied.
    DrawableHandle
    add_mesh_vertex_drawable(std::span<const float> positions, std::span<const float> color, float pointSize);

    /// Invalid, foreign, removed, and stale handles leave state unchanged.
    void set_mesh_drawable_cull_mode(DrawableHandle handle, renderer::MeshCullFaceMode mode);

    /// Returns false for invalid, foreign, removed, or stale handles.
    bool remove_drawable(DrawableHandle handle);

    /// Transform operations return false (or std::nullopt) for invalid, foreign,
    /// removed, and stale handles; reset restores the identity transform.
    bool set_drawable_transform(DrawableHandle handle, const linal::hmatf& transform);
    [[nodiscard]]
    std::optional<linal::hmatf> get_drawable_transform(DrawableHandle handle) const;
    bool reset_drawable_transform(DrawableHandle handle);

    /// Updates affect the most recently added drawable of that kind. If none
    /// exists, the call is ignored. Input spans are copied during the call.
    void update_last_point_drawable(std::span<const float> vertices,
                                    std::span<const float> colors,
                                    std::span<const std::uint32_t> indices,
                                    renderer::BufferAccessPattern accessPattern);

    void update_last_line_drawable(std::span<const float> vertices,
                                   std::span<const float> colors,
                                   std::span<const std::uint32_t> indices,
                                   renderer::BufferAccessPattern accessPattern);

    /// Removes all drawables of the given kind. Handles previously returned for
    /// those drawables become invalid and are rejected by subsequent operations.
    void clear_point_drawables();
    void clear_line_drawables();
    void clear_mesh_drawables();
    /// Removes all drawables regardless of kind. All previously-returned handles
    /// become invalid.
    void clear_drawables();

    [[nodiscard]]
    bool has_point_drawables() const;
    [[nodiscard]]
    bool has_line_drawables() const;
    [[nodiscard]]
    bool has_mesh_drawables() const;

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

    /// Selects the ImGui control surface. Switching to Release pins debug-only
    /// state (visualization mode, grayscale) back to sensible defaults so leftover
    /// debug state cannot persist into the release panel. Callable at any time.
    void set_ui_mode(renderer::UiMode mode);
    [[nodiscard]] renderer::UiMode ui_mode() const { return m_uiMode; }

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
    /// Animates the camera to a preset view orientation.
    void go_to_preset_view(PresetView view);
    /// Animates the camera to the home (initial) view.
    void go_to_home_view();

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
    [[nodiscard]]
    [[nodiscard]]
    const GlfwWindow& window() const {
        return m_window;
    }
    [[nodiscard]]
    std::weak_ptr<CameraInteractor> get_camera() {
        return m_camera;
    }
    [[nodiscard]]
    std::weak_ptr<const CameraInteractor> get_camera() const {
        return m_camera;
    }
    [[nodiscard]] bool is_auto_fit_enabled() const noexcept { return m_autoFitEnabled; }
    [[nodiscard]]
    /// The returned overlay is owned by this Renderer and must not be used
    /// after the Renderer is destroyed. Use get_imgui() for a lifetime-safe view.
    [[nodiscard]]
    ImGuiOverlay& imgui() { return *m_imgui; }
    [[nodiscard]]
    /// Returns a lifetime-safe view of the ImGui overlay. Returns nullptr from
    /// lock() if the Renderer has been destroyed.
    ImGuiOverlayView get_imgui() { return ImGuiOverlayView{m_imgui.get(), m_imguiLifetime}; }

    static constexpr renderer::ClearColor defaultClearColor{0.05F, 0.05F, 0.05F, 1.0F};

  private:
    Renderer(GlfwWindow window,
             std::unique_ptr<opengl::DrawablesManager> drawablesManager,
             std::shared_ptr<CameraInteractor> camera,
             std::unique_ptr<ImGuiOverlay> imgui,
             std::unique_ptr<opengl::Framebuffer> sceneFramebuffer,
             std::unique_ptr<opengl::Framebuffer> hdrResolveFramebuffer,
              std::unique_ptr<opengl::Framebuffer> ldrIntermediate,
              std::unique_ptr<opengl::PostProcessingPass> postProcessingPass,
              std::unique_ptr<opengl::FXAAPass> fxaaPass,
              int sceneSamples,
              int maxTextureSize,
              int maxAnisotropy,
              std::uint64_t rendererInstance);

    void wire_callbacks();
    void update_scene_viewport();
    void present_scene();
    void on_cursor_pos(double xpos, double ypos);
    void on_scroll(double xoff, double yoff);
    void on_mouse_button(int button, Action action, Mods mods);
    [[nodiscard]]
    std::optional<std::pair<double, double>> current_scene_framebuffer_coordinates() const;

    [[nodiscard]]
    CameraAutoFitResult compute_fit_destination(const linal::double3& direction,
                                                const linal::double3& up,
                                                const linal::double3& targetHint,
                                                double currentDistance) const;
    void maybe_update_auto_fit(std::chrono::steady_clock::time_point now);
    void apply_fit_result(const CameraAutoFitResult& result);

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
    std::shared_ptr<void> m_imguiLifetime{std::make_shared<int>(0)};
    std::unique_ptr<ImGuiOverlay> m_imgui;
    std::unique_ptr<opengl::Framebuffer> m_sceneFramebuffer;
    std::unique_ptr<opengl::Framebuffer> m_hdrResolveFramebuffer;
    std::unique_ptr<opengl::Framebuffer> m_ldrIntermediate;
    std::unique_ptr<opengl::PostProcessingPass> m_postProcessingPass;
    std::unique_ptr<opengl::FXAAPass> m_fxaaPass;
    int m_sceneSamples{1};
    SceneViewport m_sceneViewport;
    CursorPosState m_lastWindowCursorPos;
    bool m_cameraMouseInteractionActive{false};
    std::chrono::steady_clock::time_point m_lastFrameTime;
    std::chrono::steady_clock::time_point m_lastCameraInteractionTime;
    bool m_autoFitPending{false};
    bool m_autoFitEnabled{false};
    int m_maxTextureSize{0};
    int m_maxAnisotropy{1};
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
    renderer::UiMode m_uiMode{renderer::UiMode::Release};
};

} // namespace renderer

#endif // RENDERER_RENDERER_HPP
