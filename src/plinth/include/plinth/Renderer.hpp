#ifndef RENDERER_RENDERER_HPP
#define RENDERER_RENDERER_HPP

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
#include <plinth/WindowSettings.hpp>
#include <span>
#include <utility>
#include <vector>

namespace opengl {
class DrawablesManager;
class Framebuffer;
class PresentationPass;
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

    [[nodiscard]]
    constexpr bool is_valid() const {
        return kind != DrawableKind::invalid && id != 0U;
    }
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

    // Only one live Renderer is supported at a time. A second call to create()
    // while another Renderer is alive returns nullptr. A new instance may be
    // created after the previous owner is destroyed.
    [[nodiscard]]
    static std::unique_ptr<Renderer> create(const WindowSettings& settings);
    [[nodiscard]]
    static SceneViewport calculate_scene_viewport(std::pair<int, int> windowSize,
                                                  std::pair<int, int> framebufferSize,
                                                  double reservedLogicalWidth);
    [[nodiscard]]
    static std::optional<std::pair<double, double>>
    to_scene_framebuffer_coordinates(const SceneViewport& sceneViewport, double xpos, double ypos);

    // --- Geometry ---
    // All add_*_drawable functions return an invalid handle (is_valid() == false) if drawable
    // creation fails; they do not throw.
    DrawableHandle
    add_point_drawable(std::span<const float> vertices,
                       std::span<const float> colors,
                       std::span<const std::uint32_t> indices,
                       float pointSize,
                       renderer::BufferAccessPattern accessPattern = renderer::BufferAccessPattern::STATIC_DRAW);

    DrawableHandle
    add_line_drawable(std::span<const float> vertices,
                      std::span<const std::uint32_t> indices,
                      std::span<const float> colors,
                      renderer::LineType lineType,
                      float lineWidth,
                      float pointSize = 0.0F,
                      renderer::BufferAccessPattern accessPattern = renderer::BufferAccessPattern::STATIC_DRAW);

    DrawableHandle
    add_mesh_drawable(std::span<const float> vertices,
                      std::span<const float> normals,
                      std::span<const float> colors,
                      std::span<const std::uint32_t> triangleIndices,
                      renderer::BufferAccessPattern accessPattern = renderer::BufferAccessPattern::STATIC_DRAW);

    TextureHandle create_texture_2d(TextureData data);
    bool remove_texture(TextureHandle texture);
    DrawableHandle add_textured_mesh_drawable(
        std::span<const float> vertices,
        std::span<const float> normals,
        std::span<const float> textureCoordinates,
        std::span<const float> colors,
        std::span<const std::uint32_t> triangleIndices,
        TextureHandle texture,
        renderer::BufferAccessPattern accessPattern = renderer::BufferAccessPattern::STATIC_DRAW);

    DrawableHandle add_mesh_segment_drawable(std::span<const float> positions,
                                             std::span<const std::uint32_t> indices,
                                             std::span<const float> color,
                                             float lineWidth);

    DrawableHandle
    add_mesh_vertex_drawable(std::span<const float> positions, std::span<const float> color, float pointSize);

    void set_mesh_drawable_cull_mode(DrawableHandle handle, renderer::MeshCullFaceMode mode);

    bool remove_drawable(DrawableHandle handle);

    bool set_drawable_transform(DrawableHandle handle, const linal::hmatf& transform);
    [[nodiscard]]
    std::optional<linal::hmatf> get_drawable_transform(DrawableHandle handle) const;
    bool reset_drawable_transform(DrawableHandle handle);

    void update_last_point_drawable(std::span<const float> vertices,
                                    std::span<const float> colors,
                                    std::span<const std::uint32_t> indices,
                                    renderer::BufferAccessPattern accessPattern);

    void update_last_line_drawable(std::span<const float> vertices,
                                   std::span<const float> colors,
                                   std::span<const std::uint32_t> indices,
                                   renderer::BufferAccessPattern accessPattern);

    void clear_point_drawables();
    void clear_line_drawables();
    void clear_mesh_drawables();
    void clear_drawables();

    [[nodiscard]]
    bool has_point_drawables() const;
    [[nodiscard]]
    bool has_line_drawables() const;
    [[nodiscard]]
    bool has_mesh_drawables() const;

    // --- Post-processing controls ---
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

    // --- Frame lifecycle ---
    // Renderer owns one GLFW/OpenGL context. All frame methods must be called
    // on its creating thread; each makes that context current before GL work.
    static void poll_events();
    [[nodiscard]]
    bool should_close() const;
    [[nodiscard]]
    bool is_escape_pressed() const;

    void begin_frame(const renderer::ClearColor& clearColor = defaultClearColor);
    void draw();
    void draw(const renderer::LightingConfig& lighting);
    void end_frame();
    void end_frame(bool& autoFitEnabled);
    void end_frame(bool& autoFitEnabled, bool& homeRequested);

    // Makes the renderer-owned context current on the calling thread.
    void make_context_current() const;

    // --- Camera navigation (geometry-fit aware) ---
    void go_to_preset_view(PresetView view);
    void go_to_home_view();

    // --- Callback extension points ---
    void add_cursor_pos_callback(CursorPosCB cb);
    void add_scroll_callback(ScrollCB cb);
    void add_mouse_button_callback(MouseBtnCB cb);
    void add_key_callback(KeyCB cb);

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
    [[nodiscard]]
    // The returned overlay is owned by this Renderer and must not be used
    // after the Renderer is destroyed. It cannot extend backend lifetime.
    [[nodiscard]]
    ImGuiOverlay& imgui() { return *m_imgui; }
    [[nodiscard]]
    ImGuiOverlayView get_imgui() { return ImGuiOverlayView{m_imgui.get(), m_imguiLifetime}; }

    static constexpr renderer::ClearColor defaultClearColor{0.05F, 0.05F, 0.08F, 1.0F};

  private:
    Renderer(GlfwWindow window,
             std::unique_ptr<opengl::DrawablesManager> drawablesManager,
             std::shared_ptr<CameraInteractor> camera,
             std::unique_ptr<ImGuiOverlay> imgui,
             std::unique_ptr<opengl::Framebuffer> sceneFramebuffer,
             std::unique_ptr<opengl::Framebuffer> hdrResolveFramebuffer,
             std::unique_ptr<opengl::Framebuffer> ldrIntermediate,
             std::unique_ptr<opengl::PresentationPass> presentationPass,
             std::unique_ptr<opengl::PostProcessingPass> postProcessingPass,
             std::unique_ptr<opengl::FXAAPass> fxaaPass,
             int sceneSamples,
             int maxTextureSize);

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
    void apply_fit_result(const CameraAutoFitResult& result);

    GlfwWindow m_window;
    std::unique_ptr<opengl::DrawablesManager> m_drawablesManager;
    std::shared_ptr<CameraInteractor> m_camera;
    std::shared_ptr<void> m_imguiLifetime{std::make_shared<int>(0)};
    std::unique_ptr<ImGuiOverlay> m_imgui;
    std::unique_ptr<opengl::Framebuffer> m_sceneFramebuffer;
    std::unique_ptr<opengl::Framebuffer> m_hdrResolveFramebuffer;
    std::unique_ptr<opengl::Framebuffer> m_ldrIntermediate;
    std::unique_ptr<opengl::PresentationPass> m_presentationPass;
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

    std::vector<CursorPosCB> m_cursorPosCallbacks;
    std::vector<ScrollCB> m_scrollCallbacks;
    std::vector<MouseBtnCB> m_mouseButtonCallbacks;
    std::vector<KeyCB> m_keyCallbacks;

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
