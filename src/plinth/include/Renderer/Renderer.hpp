#ifndef RENDERER_RENDERER_HPP
#define RENDERER_RENDERER_HPP

#include <OpenGL/BufferAccessPattern.hpp>
#include <OpenGL/Drawable/DrawablesManager.hpp>
#include <OpenGL/FrameState.hpp>
#include <OpenGL/LineType.hpp>
#include <Renderer/CameraInteractor.hpp>
#include <Renderer/GlfwWindow.hpp>
#include <Renderer/ImGuiOverlay.hpp>
#include <Renderer/InputState.hpp>
#include <Renderer/WindowSettings.hpp>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <utility>
#include <vector>

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
    opengl::ViewportRect framebuffer;
};

class Renderer {
  public:
    Renderer() = delete;
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;
    Renderer(Renderer&&) = delete;
    Renderer& operator=(Renderer&&) = delete;
    ~Renderer() = default;

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
    DrawableHandle
    add_point_drawable(std::span<const float> vertices,
                       std::span<const float> colors,
                       std::span<const std::uint32_t> indices,
                       float pointSize,
                       opengl::BufferAccessPattern accessPattern = opengl::BufferAccessPattern::STATIC_DRAW);

    DrawableHandle
    add_line_drawable(std::span<const float> vertices,
                      std::span<const std::uint32_t> indices,
                      std::span<const float> colors,
                      opengl::LineType lineType,
                      float lineWidth,
                      float pointSize = 0.0F,
                      opengl::BufferAccessPattern accessPattern = opengl::BufferAccessPattern::STATIC_DRAW);

    DrawableHandle
    add_mesh_drawable(std::span<const float> vertices,
                      std::span<const float> normals,
                      std::span<const float> colors,
                      std::span<const std::uint32_t> triangleIndices,
                      opengl::BufferAccessPattern accessPattern = opengl::BufferAccessPattern::STATIC_DRAW);

    DrawableHandle add_mesh_segment_drawable(std::span<const float> positions,
                                             std::span<const std::uint32_t> indices,
                                             std::span<const float> color,
                                             float lineWidth);

    DrawableHandle
    add_mesh_vertex_drawable(std::span<const float> positions, std::span<const float> color, float pointSize);

    void set_mesh_drawable_cull_mode(DrawableHandle handle, opengl::MeshCullFaceMode mode);

    bool remove_drawable(DrawableHandle handle);

    void update_last_point_drawable(std::span<const float> vertices,
                                    std::span<const float> colors,
                                    std::span<const std::uint32_t> indices,
                                    opengl::BufferAccessPattern accessPattern);

    void update_last_line_drawable(std::span<const float> vertices,
                                   std::span<const float> colors,
                                   std::span<const std::uint32_t> indices,
                                   opengl::BufferAccessPattern accessPattern);

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

    void begin_frame(const opengl::ClearColor& clearColor = defaultClearColor);
    void draw();
    void draw(const opengl::LightingConfig& lighting);
    void end_frame();
    void end_frame(bool& autoFitEnabled);
    void end_frame(bool& autoFitEnabled, bool& homeRequested);

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

    static constexpr opengl::ClearColor defaultClearColor{0.05F, 0.05F, 0.08F, 1.0F};

  private:
    Renderer(GlfwWindow window,
             opengl::DrawablesManager drawablesManager,
             std::shared_ptr<CameraInteractor> camera,
             std::shared_ptr<ImGuiOverlay> imgui);

    void wire_callbacks();
    void update_scene_viewport();
    void on_cursor_pos(double xpos, double ypos);
    void on_scroll(double xoff, double yoff);
    void on_mouse_button(int button, Action action, Mods mods);
    [[nodiscard]]
    std::optional<std::pair<double, double>> current_scene_framebuffer_coordinates() const;

    GlfwWindow m_window;
    opengl::DrawablesManager m_drawablesManager;
    std::shared_ptr<CameraInteractor> m_camera;
    std::shared_ptr<ImGuiOverlay> m_imgui;
    SceneViewport m_sceneViewport;
    CursorPosState m_lastWindowCursorPos;
    bool m_cameraMouseInteractionActive{false};

    std::vector<CursorPosCB> m_cursorPosCallbacks;
    std::vector<ScrollCB> m_scrollCallbacks;
    std::vector<MouseBtnCB> m_mouseButtonCallbacks;
    std::vector<KeyCB> m_keyCallbacks;
};

} // namespace renderer

#endif // RENDERER_RENDERER_HPP
