#ifndef RENDERER_RENDERER_HPP
#define RENDERER_RENDERER_HPP

#include <plinth/BufferAccessPattern.hpp>
#include <plinth/CameraAutoFit.hpp>
#include <plinth/FrameState.hpp>
#include <plinth/LightingConfig.hpp>
#include <plinth/Texture.hpp>
#include <plinth/LineType.hpp>
#include <plinth/CameraInteractor.hpp>
#include <plinth/GlfwWindow.hpp>
#include <plinth/ImGuiOverlay.hpp>
#include <plinth/InputState.hpp>
#include <plinth/WindowSettings.hpp>
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
class PresentationPass;
}

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

class Renderer {
  public:
    Renderer() = delete;
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;
    Renderer(Renderer&&) = delete;
    Renderer& operator=(Renderer&&) = delete;
    ~Renderer();

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
    DrawableHandle add_textured_mesh_drawable(std::span<const float> vertices, std::span<const float> normals,
                                              std::span<const float> textureCoordinates, std::span<const float> colors,
                                              std::span<const std::uint32_t> triangleIndices, TextureHandle texture,
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

    // --- Frame lifecycle ---
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

    // --- Camera navigation (geometry-fit aware) ---
    // Recommended entry points for jumping to a named preset view or "home": unlike the
    // geometry-agnostic CameraInteractor::go_to_preset_view (which just preserves the current
    // camera-to-target distance/pivot), these gather all current drawable geometry and fit it
    // into view via calculate_camera_auto_fit, falling back to the geometry-agnostic behavior
    // when the scene has no geometry yet.
    void go_to_preset_view(PresetView view);
    void go_to_home_view();

    // --- Callback extension points ---
    void add_cursor_pos_callback(CursorPosCB cb);
    void add_scroll_callback(ScrollCB cb);
    void add_mouse_button_callback(MouseBtnCB cb);
    void add_key_callback(KeyCB cb);

    // --- Accessors ---
    [[nodiscard]]
    GlfwWindow& window() {
        return m_window;
    }
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
    std::weak_ptr<ImGuiOverlay> get_imgui() {
        return m_imgui;
    }

    static constexpr renderer::ClearColor defaultClearColor{0.05F, 0.05F, 0.08F, 1.0F};

  private:
    Renderer(GlfwWindow window,
              std::unique_ptr<opengl::DrawablesManager> drawablesManager,
              std::shared_ptr<CameraInteractor> camera,
               std::shared_ptr<ImGuiOverlay> imgui,
               std::unique_ptr<opengl::Framebuffer> sceneFramebuffer,
               std::unique_ptr<opengl::Framebuffer> resolveFramebuffer,
               std::unique_ptr<opengl::PresentationPass> presentationPass,
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
    // Builds a CameraAutoFitInput from the camera's current projection settings plus the given
    // hypothetical direction/up/target/distance, gathers all current drawable geometry, and
    // returns the fitted result. `direction`/`up`/`targetHint` define the frame to fit within
    // (not necessarily the camera's actual current pose - e.g. a preset-view direction);
    // `currentDistance` is clamped to a safe minimum so callers can never feed a degenerate
    // distance into CameraInteractor's transition machinery.
    [[nodiscard]]
    CameraAutoFitResult compute_fit_destination(const linal::double3& direction,
                                                const linal::double3& up,
                                                const linal::double3& targetHint,
                                                double currentDistance) const;
    void apply_fit_result(const CameraAutoFitResult& result);

    GlfwWindow m_window;
    std::unique_ptr<opengl::DrawablesManager> m_drawablesManager;
    std::shared_ptr<CameraInteractor> m_camera;
    std::shared_ptr<ImGuiOverlay> m_imgui;
    std::unique_ptr<opengl::Framebuffer> m_sceneFramebuffer;
    std::unique_ptr<opengl::Framebuffer> m_resolveFramebuffer;
    std::unique_ptr<opengl::PresentationPass> m_presentationPass;
    int m_sceneSamples{1};
    SceneViewport m_sceneViewport;
    CursorPosState m_lastWindowCursorPos;
    bool m_cameraMouseInteractionActive{false};
    std::chrono::steady_clock::time_point m_lastFrameTime;
    std::chrono::steady_clock::time_point m_lastCameraInteractionTime;
    bool m_autoFitPending{false};
    // Mirrors the caller-owned autoFitEnabled bool passed to end_frame(bool&, bool&) - begin_frame
    // needs to see this to decide whether to run the auto-zoom-when-settled check, but has no
    // access to end_frame's by-reference parameter (a different call each frame).
    bool m_autoFitEnabled{false};
    int m_maxTextureSize{0};

    std::vector<CursorPosCB> m_cursorPosCallbacks;
    std::vector<ScrollCB> m_scrollCallbacks;
    std::vector<MouseBtnCB> m_mouseButtonCallbacks;
    std::vector<KeyCB> m_keyCallbacks;
};

} // namespace renderer

#endif // RENDERER_RENDERER_HPP
