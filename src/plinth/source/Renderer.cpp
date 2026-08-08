#include "plinth/Renderer.hpp"

#include "OpenGL/Drawable/DrawablesManager.hpp"
#include "OpenGL/ErrorReporting.hpp"
#include "OpenGL/FXAAPass.hpp"
#include "OpenGL/FrameState.hpp"
#include "OpenGL/Framebuffer.hpp"
#include "OpenGL/GpuCapabilities.hpp"
#include "OpenGL/OpenGL.hpp"
#include "OpenGL/PostProcessingPass.hpp"
#include "plinth/ImGuiOverlay.hpp"
#include "plinth/ScopeExit.hpp"

#include <linal/vec.hpp>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <string>
#include <string_view>

namespace renderer {

struct CallbackConnection {
    bool connected{true};
};

CallbackSubscription::CallbackSubscription(std::shared_ptr<CallbackConnection> connection)
    : m_connection(std::move(connection)) {
}

CallbackSubscription::CallbackSubscription(CallbackSubscription&& other) noexcept = default;
CallbackSubscription& CallbackSubscription::operator=(CallbackSubscription&& other) noexcept {
    if (this != &other) {
        disconnect();
        m_connection = std::move(other.m_connection);
    }
    return *this;
}
CallbackSubscription::~CallbackSubscription() {
    disconnect();
}

void CallbackSubscription::disconnect() noexcept {
    if (m_connection) {
        m_connection->connected = false;
        m_connection.reset();
    }
}

bool CallbackSubscription::is_connected() const noexcept {
    return m_connection && m_connection->connected;
}

Renderer::~Renderer() {
    m_window.make_context_current();
    // Destroy the overlay while the GL context is current so an ImGui backend (if any) shuts
    // down cleanly.
    m_overlay.reset();
    m_fxaaPass.reset();
    m_postProcessingPass.reset();
    m_ldrIntermediate.reset();
    m_hdrResolveFramebuffer.reset();
    m_sceneFramebuffer.reset();
    m_drawablesManager.reset();
}

namespace {

constexpr linal::double3 defaultCameraPosition{5.0, 5.0, 5.0};
constexpr double minimumFitDistance = 1.0;
// Absolute smallest near plane handed to the clip-plane fitter. It is a fixed
// constant, not the camera's current near plane: feeding the live near plane
// back in ratchets it upward across frames and clips close geometry after a
// zoom-out/zoom-in cycle. The fitter derives an adaptive scene-scale floor at
// or above this value.
constexpr double minimumNearPlane = 0.01;
constexpr double maxFrameDeltaSeconds = 0.1;

constexpr float maxFxaaEdgeThreshold = 0.5F;
constexpr float maxFxaaEdgeThresholdMin = 0.25F;

class ScopedFullSampleShading {
  public:
    explicit ScopedFullSampleShading(bool enabled)
        : m_active(enabled) {
        if (!m_active) {
            return;
        }
        m_wasEnabled = glIsEnabled(GL_SAMPLE_SHADING);
        glGetFloatv(GL_MIN_SAMPLE_SHADING_VALUE, &m_previousMinimum);
        glEnable(GL_SAMPLE_SHADING);
        glMinSampleShading(1.0F);
    }

    ScopedFullSampleShading(const ScopedFullSampleShading&) = delete;
    ScopedFullSampleShading& operator=(const ScopedFullSampleShading&) = delete;

    ~ScopedFullSampleShading() {
        if (!m_active) {
            return;
        }
        glMinSampleShading(m_previousMinimum);
        if (m_wasEnabled == GL_FALSE) {
            glDisable(GL_SAMPLE_SHADING);
        }
    }

  private:
    bool m_active{false};
    GLboolean m_wasEnabled{GL_FALSE};
    GLfloat m_previousMinimum{0.0F};
};

std::optional<std::uint64_t> next_renderer_instance() {
    static std::atomic<std::uint64_t> next{1U};
    std::uint64_t current = next.load(std::memory_order_relaxed);
    while (current != std::numeric_limits<std::uint64_t>::max()) {
        if (next.compare_exchange_weak(current, current + 1U, std::memory_order_relaxed)) {
            return current;
        }
    }
    return std::nullopt;
}

bool is_finite(float value) {
    return std::isfinite(value);
}

bool is_valid_tone_map_mode(ToneMapMode mode) {
    return mode == ToneMapMode::None || mode == ToneMapMode::Reinhard;
}

bool is_valid_visualization_mode(VisualizationMode mode) {
    return mode >= VisualizationMode::Final && mode <= VisualizationMode::Grayscale;
}

void report_invalid_argument(std::string_view setter) {
    opengl::report_error(std::string{"Error: Renderer::"} + std::string{setter} + " rejected an invalid value");
}

std::uint32_t valid_framebuffer_dimension(int dimension) {
    return static_cast<std::uint32_t>(dimension > 0 ? dimension : 1);
}

double valid_logical_dimension(int dimension) {
    return static_cast<double>(dimension > 0 ? dimension : 1);
}

int round_to_framebuffer_pixel(double value) {
    return static_cast<int>(std::round(value));
}

// Default geometry computed for the convenience add_*_drawable overloads that
// omit indices, per-vertex colors, or normals. Each helper produces the buffer
// the fully-parameterized path expects, so the shorter overloads can delegate.

// Sequential indices 0..N-1 for N = vertices.size()/3, used when the caller
// supplies no explicit index buffer. Mirrors DrawablesManager::add_mesh_vertex_drawable.
std::vector<std::uint32_t> make_sequential_indices(std::span<const float> vertices) {
    std::vector<std::uint32_t> indices(vertices.size() / 3U);
    std::iota(indices.begin(), indices.end(), 0U);
    return indices;
}

// Replicate a single RGBA color into one color per vertex.
std::vector<float> expand_color(std::span<const float> vertices, std::array<float, 4> color) {
    const std::size_t vertexCount = vertices.size() / 3U;
    std::vector<float> colors(vertexCount * 4U);
    for (std::size_t v = 0; v < vertexCount; ++v) {
        for (std::size_t c = 0; c < 4U; ++c) {
            colors[(v * 4U) + c] = color[c];
        }
    }
    return colors;
}

// Area-weighted smooth per-vertex normals from triangle indices. Vertices are
// xyz triplets; indices are triples. Triangles referencing out-of-range vertices
// are skipped; vertices with a zero-length accumulation fall back to {0,0,1}.
std::vector<float> compute_vertex_normals(std::span<const float> vertices,
                                          std::span<const std::uint32_t> triangleIndices) {
    const std::size_t vertexCount = vertices.size() / 3U;
    std::vector<linal::float3> accum(vertexCount, linal::float3{0.0F, 0.0F, 0.0F});
    for (std::size_t t = 0; t + 2U < triangleIndices.size(); t += 3U) {
        const std::uint32_t i0 = triangleIndices[t];
        const std::uint32_t i1 = triangleIndices[t + 1U];
        const std::uint32_t i2 = triangleIndices[t + 2U];
        if (i0 >= vertexCount || i1 >= vertexCount || i2 >= vertexCount) {
            continue;
        }
        const linal::float3 v0{vertices[(i0 * 3U)], vertices[(i0 * 3U) + 1U], vertices[(i0 * 3U) + 2U]};
        const linal::float3 v1{vertices[(i1 * 3U)], vertices[(i1 * 3U) + 1U], vertices[(i1 * 3U) + 2U]};
        const linal::float3 v2{vertices[(i2 * 3U)], vertices[(i2 * 3U) + 1U], vertices[(i2 * 3U) + 2U]};
        const linal::float3 faceNormal = linal::cross(v1 - v0, v2 - v0); // un-normalized => area weighting
        accum[i0] = accum[i0] + faceNormal;
        accum[i1] = accum[i1] + faceNormal;
        accum[i2] = accum[i2] + faceNormal;
    }
    std::vector<float> normals(vertexCount * 3U, 0.0F);
    for (std::size_t v = 0; v < vertexCount; ++v) {
        const linal::float3 unit =
            linal::length(accum[v]) > 1.0e-6F ? linal::normalize(accum[v]) : linal::float3{0.0F, 0.0F, 1.0F};
        normals[v * 3U] = unit[0];
        normals[(v * 3U) + 1U] = unit[1];
        normals[(v * 3U) + 2U] = unit[2];
    }
    return normals;
}

} // namespace

SceneViewport Renderer::calculate_scene_viewport(std::pair<int, int> windowSize,
                                                 std::pair<int, int> framebufferSize,
                                                 const LogicalViewportRect& logicalRect) {
    const double logicalWindowWidth = valid_logical_dimension(windowSize.first);
    const double logicalWindowHeight = valid_logical_dimension(windowSize.second);
    const int framebufferWidth = std::max(1, framebufferSize.first);
    const int framebufferHeight = std::max(1, framebufferSize.second);

    // Clamp the requested logical rect to the window so the mapped framebuffer viewport
    // always covers at least one pixel and never runs past the framebuffer edges.
    const double sceneX = std::clamp(logicalRect.x, 0.0, logicalWindowWidth - 1.0);
    const double sceneY = std::clamp(logicalRect.y, 0.0, logicalWindowHeight - 1.0);
    const double sceneWidth = std::clamp(logicalRect.width, 1.0, logicalWindowWidth - sceneX);
    const double sceneHeight = std::clamp(logicalRect.height, 1.0, logicalWindowHeight - sceneY);

    const double xScale = static_cast<double>(framebufferWidth) / logicalWindowWidth;
    const double yScale = static_cast<double>(framebufferHeight) / logicalWindowHeight;

    const int framebufferX = std::clamp(round_to_framebuffer_pixel(sceneX * xScale), 0, framebufferWidth - 1);
    const int framebufferY = std::clamp(round_to_framebuffer_pixel(sceneY * yScale), 0, framebufferHeight - 1);
    const int framebufferSceneWidth =
        std::clamp(round_to_framebuffer_pixel(sceneWidth * xScale), 1, framebufferWidth - framebufferX);
    const int framebufferSceneHeight =
        std::clamp(round_to_framebuffer_pixel(sceneHeight * yScale), 1, framebufferHeight - framebufferY);

    SceneViewport viewport;
    viewport.logical = LogicalViewportRect{sceneX, sceneY, sceneWidth, sceneHeight};
    viewport.framebuffer =
        renderer::ViewportRect{framebufferX, framebufferY, framebufferSceneWidth, framebufferSceneHeight};
    return viewport;
}

std::optional<std::pair<double, double>>
Renderer::to_scene_framebuffer_coordinates(const SceneViewport& sceneViewport, double xpos, double ypos) {
    if (!sceneViewport.logical.contains(xpos, ypos)) {
        return std::nullopt;
    }

    const double localLogicalX = xpos - sceneViewport.logical.x;
    const double localLogicalY = ypos - sceneViewport.logical.y;
    const double xScale = static_cast<double>(sceneViewport.framebuffer.width) / sceneViewport.logical.width;
    const double yScale = static_cast<double>(sceneViewport.framebuffer.height) / sceneViewport.logical.height;
    return std::pair<double, double>{localLogicalX * xScale, localLogicalY * yScale};
}

std::unique_ptr<Renderer> Renderer::create(const WindowSettings& settings) {
    auto window = GlfwWindow::create(settings);
    if (!window.has_value()) {
        opengl::report_error("Error: Renderer::create failed - window creation failed (see above)");
        return nullptr;
    }

    const auto capabilities = opengl::query_gpu_capabilities();
    const bool reversedDepth = capabilities.supportsClipControl;
    if (reversedDepth) {
        glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE);
    }
    const auto [fbWidth, fbHeight] = window->get_framebuffer_size();
    CameraSettings cameraSettings;
    cameraSettings.m_defaultPosition = defaultCameraPosition;
    cameraSettings.m_defaultTarget = linal::double3{0.0, 0.0, 0.0};
    cameraSettings.m_defaultUp = linal::double3{0.0, 0.0, 1.0};
    cameraSettings.m_camera.set_reversed_z(reversedDepth);
    cameraSettings.m_camera.set_viewport(0,
                                         0,
                                         valid_framebuffer_dimension(fbWidth),
                                         valid_framebuffer_dimension(fbHeight));
    auto camera = std::make_shared<CameraInteractor>(window->get_input_state(), cameraSettings);

    // The built-in ImGui overlay is created only for OverlayKind::BuiltInImGui. For None,
    // no ImGui context or GLFW/GL backends are initialized and the application drives its
    // own UI. The Renderer drives whatever overlay it is given only through IOverlay.
    std::shared_ptr<IOverlay> overlay;
    if (settings.overlay == OverlayKind::BuiltInImGui) {
        overlay = std::make_shared<ImGuiOverlay>(window->get_native_handle());
    }

    auto drawablesManager = opengl::DrawablesManager::create();
    if (!drawablesManager) {
        opengl::report_error(
            "Error: Renderer::create failed - GL program/drawables-manager creation failed (see above)");
        return nullptr;
    }

    GLint maxSamples{0};
    GLint maxColorSamples{0};
    GLint maxDepthSamples{0};
    glGetIntegerv(GL_MAX_SAMPLES, &maxSamples);
    glGetIntegerv(GL_MAX_COLOR_TEXTURE_SAMPLES, &maxColorSamples);
    glGetIntegerv(GL_MAX_DEPTH_TEXTURE_SAMPLES, &maxDepthSamples);
    maxSamples = std::min({maxSamples, maxColorSamples, maxDepthSamples});
    if (maxSamples < 1) {
        opengl::report_error("Error: Renderer::create failed - GL_MAX_SAMPLES is less than one");
        return nullptr;
    }
    const int sceneSamples = std::clamp(settings.samples, 1, maxSamples);
    const int framebufferWidth = static_cast<int>(valid_framebuffer_dimension(fbWidth));
    const int framebufferHeight = static_cast<int>(valid_framebuffer_dimension(fbHeight));

    opengl::Framebuffer::HdrConfig hdrConfig{framebufferWidth, framebufferHeight, sceneSamples, true, reversedDepth};
    auto hdrSceneFb = opengl::Framebuffer::create_hdr(hdrConfig);
    if (!hdrSceneFb.has_value()) {
        opengl::report_error("Error: Renderer::create failed - HDR scene framebuffer creation failed");
        return nullptr;
    }

    std::unique_ptr<opengl::Framebuffer> hdrResolveFb;
    if (sceneSamples > 1) {
        opengl::Framebuffer::HdrConfig resolveConfig{framebufferWidth, framebufferHeight, 1, true, reversedDepth};
        auto resolve = opengl::Framebuffer::create_hdr(resolveConfig);
        if (!resolve.has_value()) {
            opengl::report_error("Error: Renderer::create failed - HDR resolve framebuffer creation failed");
            return nullptr;
        }
        hdrResolveFb = std::make_unique<opengl::Framebuffer>(std::move(*resolve));
    }

    auto ldrFb = opengl::Framebuffer::create_ldr_intermediate(framebufferWidth, framebufferHeight);
    if (!ldrFb.has_value()) {
        opengl::report_error("Error: Renderer::create failed - LDR intermediate framebuffer creation failed");
        return nullptr;
    }

    auto postProcess = opengl::PostProcessingPass::create();
    if (!postProcess.has_value()) {
        opengl::report_error("Error: Renderer::create failed - post-processing pass creation failed");
        return nullptr;
    }

    auto fxaa = opengl::FXAAPass::create();
    if (!fxaa.has_value()) {
        opengl::report_error("Error: Renderer::create failed - FXAA pass creation failed");
        return nullptr;
    }

    const auto rendererInstance = next_renderer_instance();
    if (!rendererInstance) {
        opengl::report_error("Error: Renderer::create failed - renderer instance IDs exhausted");
        return nullptr;
    }

    std::unique_ptr<Renderer> renderer(
        new Renderer(std::move(window.value()),
                     std::move(drawablesManager),
                     std::move(camera),
                     std::move(overlay),
                     std::make_unique<opengl::Framebuffer>(std::move(*hdrSceneFb)),
                     std::move(hdrResolveFb),
                     std::make_unique<opengl::Framebuffer>(std::move(*ldrFb)),
                     std::make_unique<opengl::PostProcessingPass>(std::move(*postProcess)),
                     std::make_unique<opengl::FXAAPass>(std::move(*fxaa)),
                     sceneSamples,
                     maxSamples,
                     capabilities.maxTextureSize,
                     capabilities.maxAnisotropy,
                     reversedDepth,
                     *rendererInstance));

    renderer->update_scene_viewport();
    renderer->wire_callbacks();

    return renderer;
}

Renderer::Renderer(GlfwWindow window,
                   std::unique_ptr<opengl::DrawablesManager> drawables,
                   std::shared_ptr<CameraInteractor> camera,
                   std::shared_ptr<IOverlay> overlay,
                   std::unique_ptr<opengl::Framebuffer> sceneFramebuffer,
                   std::unique_ptr<opengl::Framebuffer> hdrResolveFramebuffer,
                   std::unique_ptr<opengl::Framebuffer> ldrIntermediate,
                   std::unique_ptr<opengl::PostProcessingPass> postProcessingPass,
                   std::unique_ptr<opengl::FXAAPass> fxaaPass,
                   int sceneSamples,
                   int maxSceneSamples,
                   int maxTextureSize,
                   int maxAnisotropy,
                   bool reversedDepth,
                   std::uint64_t rendererInstance)
    : m_window(std::move(window))
    , m_drawablesManager(std::move(drawables))
    , m_camera(std::move(camera))
    , m_overlay(std::move(overlay))
    , m_sceneFramebuffer(std::move(sceneFramebuffer))
    , m_hdrResolveFramebuffer(std::move(hdrResolveFramebuffer))
    , m_ldrIntermediate(std::move(ldrIntermediate))
    , m_postProcessingPass(std::move(postProcessingPass))
    , m_fxaaPass(std::move(fxaaPass))
    , m_sceneSamples(sceneSamples)
    , m_requestedSceneSamples(sceneSamples)
    , m_maxSceneSamples(maxSceneSamples)
    , m_lastFrameTime(std::chrono::steady_clock::now())
    , m_maxTextureSize(maxTextureSize)
    , m_maxAnisotropy(maxAnisotropy)
    , m_reversedDepth(reversedDepth)
    , m_rendererInstance(rendererInstance) {
    // Preserve the historical Renderer default. CameraAutoFitSettings itself
    // remains enabled by default for direct calculate_camera_auto_fit callers.
    m_cameraAutoFitSettings.enabled = false;
}

void Renderer::on_cursor_pos(double xpos, double ypos) {
    m_lastWindowCursorPos = CursorPosState{xpos, ypos};
    if (m_overlay && !m_overlay->handle_cursor_position(xpos, ypos)) {
        return;
    }
    const auto sceneCoordinates = Renderer::to_scene_framebuffer_coordinates(m_sceneViewport, xpos, ypos);
    if (!sceneCoordinates.has_value()) {
        return;
    }
    const auto [sceneX, sceneY] = *sceneCoordinates;
    m_window.get_input_state().cursorPosState = CursorPosState{sceneX, sceneY};
    m_camera->on_cursor_position(sceneX, sceneY);
    dispatch_callbacks(m_cursorPosCallbacks, sceneX, sceneY);
}

void Renderer::on_scroll(double xoff, double yoff) {
    if (m_overlay && !m_overlay->handle_scroll(xoff, yoff)) {
        return;
    }
    if (!current_scene_framebuffer_coordinates().has_value()) {
        return;
    }
    const auto [sceneX, sceneY] = *current_scene_framebuffer_coordinates();
    m_window.get_input_state().cursorPosState = CursorPosState{sceneX, sceneY};
    m_camera->on_scroll(xoff, yoff);
    dispatch_callbacks(m_scrollCallbacks, xoff, yoff);
}

void Renderer::on_mouse_button(int button, Action action, Mods mods) {
    if (m_overlay && !m_overlay->handle_mouse_button(button, action, mods)) {
        return;
    }
    const auto sceneCoordinates = current_scene_framebuffer_coordinates();
    if (!sceneCoordinates.has_value() && !m_cameraMouseInteractionActive) {
        return;
    }
    if (sceneCoordinates.has_value()) {
        const auto [sceneX, sceneY] = *sceneCoordinates;
        m_window.get_input_state().cursorPosState = CursorPosState{sceneX, sceneY};
    }
    if (action == Action::PRESS && !m_cameraMouseInteractionActive && sceneCoordinates.has_value() &&
        (button == 1 || button == 2)) {
        m_cameraMouseInteractionActive = true;
        m_cameraMouseInteractionButton = button;
    }
    m_camera->on_mouse_button(button, action, mods);
    if (action == Action::RELEASE && button == m_cameraMouseInteractionButton) {
        m_cameraMouseInteractionActive = false;
        m_cameraMouseInteractionButton = -1;
    }
    dispatch_callbacks(m_mouseButtonCallbacks, button, action, mods);
}

void Renderer::wire_callbacks() {
    m_window.set_cursor_pos_callback([this](double xpos, double ypos) { on_cursor_pos(xpos, ypos); });

    m_window.set_scroll_callback([this](double xoff, double yoff) { on_scroll(xoff, yoff); });

    m_window.set_mouse_button_callback(
        [this](int button, Action action, Mods mods) { on_mouse_button(button, action, mods); });

    m_window.set_key_callback([this](Key key, Scancode scancode, Action action, Mods mods) {
        if (m_overlay && !m_overlay->handle_key(key, scancode, action, mods)) {
            return;
        }
        if (key == Key::KEY_H && action == Action::PRESS && (!m_overlay || !m_overlay->wants_keyboard())) {
            go_to_home_view();
        }
        dispatch_callbacks(m_keyCallbacks, key, scancode, action, mods);
    });

    m_window.set_char_callback([this](std::uint32_t codepoint) {
        if (m_overlay) {
            m_overlay->handle_char(codepoint);
        }
    });

    m_window.set_framebuffer_size_callback([this]([[maybe_unused]] std::uint32_t width,
                                                  [[maybe_unused]] std::uint32_t height) { update_scene_viewport(); });
}

// --- Geometry ---

DrawableHandle Renderer::add_point_drawable(std::span<const float> vertices,
                                            std::array<float, 4> color,
                                            float pointSize,
                                            renderer::BufferAccessPattern accessPattern) {
    const std::vector<float> colors = expand_color(vertices, color);
    const std::vector<std::uint32_t> indices = make_sequential_indices(vertices);
    return add_point_drawable(vertices, indices, colors, pointSize, accessPattern);
}

DrawableHandle Renderer::add_point_drawable(std::span<const float> vertices,
                                            std::span<const float> colors,
                                            float pointSize,
                                            renderer::BufferAccessPattern accessPattern) {
    const std::vector<std::uint32_t> indices = make_sequential_indices(vertices);
    return add_point_drawable(vertices, indices, colors, pointSize, accessPattern);
}

DrawableHandle Renderer::add_point_drawable(std::span<const float> vertices,
                                            std::span<const std::uint32_t> indices,
                                            std::span<const float> colors,
                                            float pointSize,
                                            renderer::BufferAccessPattern accessPattern) {
    const auto id = m_drawablesManager->add_point_drawable(vertices, colors, indices, pointSize, accessPattern);
    if (!id.has_value()) {
        return DrawableHandle{};
    }
    request_auto_fit();
    return DrawableHandle{DrawableKind::point, *id, m_rendererInstance};
}

DrawableHandle Renderer::add_line_drawable(std::span<const float> vertices,
                                           std::span<const std::uint32_t> indices,
                                           std::span<const float> colors,
                                           renderer::LineType lineType,
                                           const renderer::StrokeStyle& style,
                                           float pointSize,
                                           renderer::BufferAccessPattern accessPattern,
                                           std::span<const std::uint8_t> perVertexDashFlags) {
    const auto id = m_drawablesManager->add_line_drawable(vertices,
                                                          indices,
                                                          colors,
                                                          lineType,
                                                          style.lineWidth,
                                                          pointSize,
                                                          accessPattern,
                                                          style.cap,
                                                          style.join,
                                                          style.dashPattern,
                                                          style.dashSpace,
                                                          perVertexDashFlags);
    if (!id.has_value()) {
        return DrawableHandle{};
    }
    if (style.dashPhase != 0.0F) {
        m_drawablesManager->set_line_dash_phase(*id, style.dashPhase);
    }
    request_auto_fit();
    return DrawableHandle{DrawableKind::line, *id, m_rendererInstance};
}

DrawableHandle Renderer::add_line_drawable(std::span<const float> vertices,
                                           std::array<float, 4> color,
                                           renderer::LineType lineType,
                                           const renderer::StrokeStyle& style,
                                           float pointSize,
                                           renderer::BufferAccessPattern accessPattern) {
    const std::vector<float> colors = expand_color(vertices, color);
    const std::vector<std::uint32_t> indices = make_sequential_indices(vertices);
    return add_line_drawable(vertices, indices, colors, lineType, style, pointSize, accessPattern);
}

DrawableHandle Renderer::add_line_drawable(std::span<const float> vertices,
                                           std::span<const float> colors,
                                           renderer::LineType lineType,
                                           const renderer::StrokeStyle& style,
                                           float pointSize,
                                           renderer::BufferAccessPattern accessPattern) {
    const std::vector<std::uint32_t> indices = make_sequential_indices(vertices);
    return add_line_drawable(vertices, indices, colors, lineType, style, pointSize, accessPattern);
}

DrawableHandle Renderer::add_mesh_drawable(std::span<const float> vertices,
                                           std::span<const std::uint32_t> triangleIndices,
                                           std::array<float, 4> color,
                                           renderer::MeshCullFaceMode cullMode,
                                           renderer::BufferAccessPattern accessPattern) {
    const std::vector<float> colors = expand_color(vertices, color);
    const std::vector<float> normals = compute_vertex_normals(vertices, triangleIndices);
    return add_mesh_drawable(vertices, triangleIndices, normals, colors, cullMode, accessPattern);
}

DrawableHandle Renderer::add_mesh_drawable(std::span<const float> vertices,
                                           std::span<const std::uint32_t> triangleIndices,
                                           std::span<const float> colors,
                                           renderer::MeshCullFaceMode cullMode,
                                           renderer::BufferAccessPattern accessPattern) {
    const std::vector<float> normals = compute_vertex_normals(vertices, triangleIndices);
    return add_mesh_drawable(vertices, triangleIndices, normals, colors, cullMode, accessPattern);
}

DrawableHandle Renderer::add_mesh_drawable(std::span<const float> vertices,
                                           std::span<const std::uint32_t> triangleIndices,
                                           std::array<float, 4> color,
                                           std::span<const float> normals,
                                           renderer::MeshCullFaceMode cullMode,
                                           renderer::BufferAccessPattern accessPattern) {
    const std::vector<float> colors = expand_color(vertices, color);
    return add_mesh_drawable(vertices, triangleIndices, normals, colors, cullMode, accessPattern);
}

DrawableHandle Renderer::add_mesh_drawable(std::span<const float> vertices,
                                           std::span<const std::uint32_t> triangleIndices,
                                           std::span<const float> normals,
                                           std::span<const float> colors,
                                           renderer::MeshCullFaceMode cullMode,
                                           renderer::BufferAccessPattern accessPattern) {
    const auto id =
        m_drawablesManager->add_mesh_drawable(vertices, 3, normals, colors, 4, triangleIndices, accessPattern);
    if (!id.has_value()) {
        return DrawableHandle{};
    }
    m_drawablesManager->set_mesh_drawable_cull_mode(*id, cullMode);
    // Adding geometry schedules a camera re-pose so the new mesh is framed on the
    // next auto-fit pass. Clip planes are refit every frame regardless.
    request_auto_fit();
    return DrawableHandle{DrawableKind::mesh, *id, m_rendererInstance};
}

DrawableHandle Renderer::add_mesh_drawable(const renderer::MeshData& mesh,
                                           std::array<float, 4> color,
                                           renderer::MeshCullFaceMode cullMode,
                                           renderer::BufferAccessPattern accessPattern) {
    if (mesh.empty()) {
        return DrawableHandle{};
    }
    // Fill omitted attributes exactly like the shorter span overloads do, then
    // delegate to the fully-specified path so the auto-fill logic lives in one
    // place.
    const std::vector<std::uint32_t> generatedIndices =
        mesh.triangleIndices.empty() ? make_sequential_indices(mesh.vertices) : std::vector<std::uint32_t>{};
    const std::span<const std::uint32_t> indices =
        mesh.triangleIndices.empty() ? std::span<const std::uint32_t>{generatedIndices} : mesh.triangleIndices;

    const std::vector<float> generatedNormals =
        mesh.normals.empty() ? compute_vertex_normals(mesh.vertices, indices) : std::vector<float>{};
    const std::span<const float> normals =
        mesh.normals.empty() ? std::span<const float>{generatedNormals} : mesh.normals;

    const std::vector<float> generatedColors =
        mesh.colors.empty() ? expand_color(mesh.vertices, color) : std::vector<float>{};
    const std::span<const float> colors = mesh.colors.empty() ? std::span<const float>{generatedColors} : mesh.colors;

    return add_mesh_drawable(mesh.vertices, indices, normals, colors, cullMode, accessPattern);
}

TextureHandle Renderer::create_texture_2d(TextureData data) {
    const auto id = m_drawablesManager->create_texture_2d(data, m_maxTextureSize, m_maxAnisotropy);
    return id ? TextureHandle{*id, m_rendererInstance} : TextureHandle{};
}

bool Renderer::remove_texture(TextureHandle texture) {
    return texture.is_valid() && texture.rendererInstance == m_rendererInstance &&
           m_drawablesManager->remove_texture(texture.id);
}

DrawableHandle Renderer::add_textured_mesh_drawable(std::span<const float> vertices,
                                                    std::span<const float> normals,
                                                    std::span<const float> textureCoordinates,
                                                    std::span<const float> colors,
                                                    std::span<const std::uint32_t> triangleIndices,
                                                    TextureHandle texture,
                                                    renderer::BufferAccessPattern accessPattern) {
    if (!texture.is_valid() || texture.rendererInstance != m_rendererInstance) {
        return {};
    }
    const auto id = m_drawablesManager->add_textured_mesh_drawable(vertices,
                                                                   3,
                                                                   normals,
                                                                   textureCoordinates,
                                                                   colors,
                                                                   4,
                                                                   triangleIndices,
                                                                   texture.id,
                                                                   accessPattern);
    if (!id.has_value()) {
        return {};
    }
    request_auto_fit();
    return DrawableHandle{DrawableKind::mesh, *id, m_rendererInstance};
}

void Renderer::set_mesh_drawable_cull_mode(DrawableHandle handle, renderer::MeshCullFaceMode mode) {
    if (handle.kind == DrawableKind::mesh && handle.is_valid() && handle.rendererInstance == m_rendererInstance) {
        m_drawablesManager->set_mesh_drawable_cull_mode(handle.id, mode);
    }
}

DrawableHandle Renderer::add_sphere_point_drawable(std::span<const float> centers,
                                                   std::span<const float> radii,
                                                   std::array<float, 4> color,
                                                   renderer::BufferAccessPattern accessPattern) {
    const std::vector<float> colors = expand_color(centers, color);
    return add_sphere_point_drawable(centers, radii, colors, accessPattern);
}

DrawableHandle Renderer::add_sphere_point_drawable(std::span<const float> centers,
                                                   std::span<const float> radii,
                                                   std::span<const float> colors,
                                                   renderer::BufferAccessPattern accessPattern) {
    const auto id = m_drawablesManager->add_sphere_drawable(centers, radii, colors, accessPattern);
    if (!id.has_value()) {
        return DrawableHandle{};
    }
    request_auto_fit();
    return DrawableHandle{DrawableKind::sphere, *id, m_rendererInstance};
}

bool Renderer::remove_drawable(DrawableHandle handle) {
    if (!handle.is_valid() || handle.rendererInstance != m_rendererInstance) {
        return false;
    }

    bool removed = false;
    switch (handle.kind) {
    case DrawableKind::point:   removed = m_drawablesManager->remove_point_drawable(handle.id); break;
    case DrawableKind::line:    removed = m_drawablesManager->remove_line_drawable(handle.id); break;
    case DrawableKind::mesh:    removed = m_drawablesManager->remove_mesh_drawable(handle.id); break;
    case DrawableKind::sphere:  removed = m_drawablesManager->remove_sphere_drawable(handle.id); break;
    case DrawableKind::invalid: return false;
    }
    if (removed) {
        request_auto_fit();
    }
    return removed;
}

bool Renderer::set_drawable_transform(DrawableHandle handle, const linal::hmatf& transform) {
    if (!handle.is_valid() || handle.rendererInstance != m_rendererInstance) {
        return false;
    }

    bool transformed = false;
    switch (handle.kind) {
    case DrawableKind::point:
        transformed = m_drawablesManager->set_point_drawable_transform(handle.id, transform);
        break;
    case DrawableKind::line:    transformed = m_drawablesManager->set_line_drawable_transform(handle.id, transform); break;
    case DrawableKind::mesh:    transformed = m_drawablesManager->set_mesh_drawable_transform(handle.id, transform); break;
    case DrawableKind::sphere:  transformed = m_drawablesManager->set_sphere_drawable_transform(handle.id, transform); break;
    case DrawableKind::invalid: return false;
    }
    if (transformed) {
        request_auto_fit();
    }
    return transformed;
}

std::optional<linal::hmatf> Renderer::get_drawable_transform(DrawableHandle handle) const {
    if (!handle.is_valid() || handle.rendererInstance != m_rendererInstance) {
        return std::nullopt;
    }

    switch (handle.kind) {
    case DrawableKind::point:   return m_drawablesManager->get_point_drawable_transform(handle.id);
    case DrawableKind::line:    return m_drawablesManager->get_line_drawable_transform(handle.id);
    case DrawableKind::mesh:    return m_drawablesManager->get_mesh_drawable_transform(handle.id);
    case DrawableKind::sphere:  return m_drawablesManager->get_sphere_drawable_transform(handle.id);
    case DrawableKind::invalid: return std::nullopt;
    }

    return std::nullopt;
}

bool Renderer::reset_drawable_transform(DrawableHandle handle) {
    return set_drawable_transform(handle, linal::hmatf::identity());
}

bool Renderer::set_line_cap(DrawableHandle handle, renderer::LineCap cap) {
    if (handle.kind != DrawableKind::line || !handle.is_valid() || handle.rendererInstance != m_rendererInstance) {
        return false;
    }
    return m_drawablesManager->set_line_cap(handle.id, cap);
}

bool Renderer::set_line_join(DrawableHandle handle, renderer::LineJoin join) {
    if (handle.kind != DrawableKind::line || !handle.is_valid() || handle.rendererInstance != m_rendererInstance) {
        return false;
    }
    return m_drawablesManager->set_line_join(handle.id, join);
}

bool Renderer::set_line_stroke_style(DrawableHandle handle, const renderer::StrokeStyle& style) {
    if (handle.kind != DrawableKind::line || !handle.is_valid() || handle.rendererInstance != m_rendererInstance) {
        return false;
    }
    m_drawablesManager->set_line_cap(handle.id, style.cap);
    m_drawablesManager->set_line_join(handle.id, style.join);
    m_drawablesManager->set_line_dash_pattern(handle.id, style.dashPattern);
    m_drawablesManager->set_line_dash_phase(handle.id, style.dashPhase);
    m_drawablesManager->set_line_dash_space(handle.id, style.dashSpace);
    return true;
}

bool Renderer::set_line_dash_pattern(DrawableHandle handle, std::span<const float> pattern) {
    if (handle.kind != DrawableKind::line || !handle.is_valid() || handle.rendererInstance != m_rendererInstance) {
        return false;
    }
    return m_drawablesManager->set_line_dash_pattern(handle.id, pattern);
}

bool Renderer::set_line_dash_phase(DrawableHandle handle, float phase) {
    if (handle.kind != DrawableKind::line || !handle.is_valid() || handle.rendererInstance != m_rendererInstance) {
        return false;
    }
    return m_drawablesManager->set_line_dash_phase(handle.id, phase);
}

bool Renderer::set_line_dash_space(DrawableHandle handle, renderer::DashSpace space) {
    if (handle.kind != DrawableKind::line || !handle.is_valid() || handle.rendererInstance != m_rendererInstance) {
        return false;
    }
    return m_drawablesManager->set_line_dash_space(handle.id, space);
}

bool Renderer::set_line_dash_enabled(DrawableHandle handle, bool enabled) {
    if (handle.kind != DrawableKind::line || !handle.is_valid() || handle.rendererInstance != m_rendererInstance) {
        return false;
    }
    return m_drawablesManager->set_line_dash_enabled(handle.id, enabled);
}

bool Renderer::set_line_dash(DrawableHandle handle, float dashSize, float gapSize) {
    if (handle.kind != DrawableKind::line || !handle.is_valid() || handle.rendererInstance != m_rendererInstance) {
        return false;
    }
    return m_drawablesManager->set_line_dash(handle.id, dashSize, gapSize);
}

void Renderer::update_last_point_drawable(std::span<const float> vertices,
                                          std::span<const float> colors,
                                          std::span<const std::uint32_t> indices,
                                          renderer::BufferAccessPattern accessPattern) {
    if (m_drawablesManager->update_last_point_drawable(vertices, colors, indices, accessPattern)) {
        request_auto_fit();
    }
}

void Renderer::update_last_line_drawable(std::span<const float> vertices,
                                         std::span<const float> colors,
                                         std::span<const std::uint32_t> indices,
                                         renderer::BufferAccessPattern accessPattern) {
    if (m_drawablesManager->update_last_line_drawable(vertices, colors, indices, accessPattern)) {
        request_auto_fit();
    }
}

void Renderer::clear_point_drawables() {
    if (m_drawablesManager->clear_point_drawables()) {
        request_auto_fit();
    }
}
void Renderer::clear_line_drawables() {
    if (m_drawablesManager->clear_line_drawables()) {
        request_auto_fit();
    }
}
void Renderer::clear_mesh_drawables() {
    if (m_drawablesManager->clear_mesh_drawables()) {
        request_auto_fit();
    }
}
void Renderer::clear_sphere_point_drawables() {
    if (m_drawablesManager->clear_sphere_drawables()) {
        request_auto_fit();
    }
}
void Renderer::clear_drawables() {
    if (m_drawablesManager->clear_drawables()) {
        request_auto_fit();
    }
}

bool Renderer::has_point_drawables() const {
    return m_drawablesManager->has_point_drawables();
}
bool Renderer::has_line_drawables() const {
    return m_drawablesManager->has_line_drawables();
}
bool Renderer::has_mesh_drawables() const {
    return m_drawablesManager->has_mesh_drawables();
}
bool Renderer::has_sphere_point_drawables() const {
    return m_drawablesManager->has_sphere_drawables();
}

Renderer::PickRay Renderer::compute_pick_ray(double xpos, double ypos) const {
    const renderer::PickRay ray = m_camera->get_pick_ray(xpos, ypos);
    return PickRay{linal::float3{static_cast<float>(ray.origin[0]),
                                 static_cast<float>(ray.origin[1]),
                                 static_cast<float>(ray.origin[2])},
                   linal::float3{static_cast<float>(ray.direction[0]),
                                 static_cast<float>(ray.direction[1]),
                                 static_cast<float>(ray.direction[2])}};
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
std::vector<Renderer::PickResult> Renderer::pick_drawables(double xpos, double ypos, double radius) const {
    std::vector<PickResult> results;
    if (!m_drawablesManager->has_drawables()) {
        return results;
    }

    // The pick pass renders into an offscreen target sized to the scene viewport. Incoming
    // coordinates are scene-framebuffer coordinates (top-left origin, local to the scene viewport),
    // the same space camera pick rays use, so they index directly into this target.
    const int width = static_cast<int>(valid_framebuffer_dimension(m_sceneViewport.framebuffer.width));
    const int height = static_cast<int>(valid_framebuffer_dimension(m_sceneViewport.framebuffer.height));
    if (width <= 0 || height <= 0) {
        return results;
    }

    const PickRay pickRay = compute_pick_ray(xpos, ypos);

    make_context_current();

    // Lazily create or resize the single-sample, non-sRGB pick target. Single sample avoids MSAA
    // color averaging that would corrupt encoded IDs.
    if (!m_pickFramebuffer || m_pickFramebuffer->get_width() != width || m_pickFramebuffer->get_height() != height) {
        auto pick = opengl::Framebuffer::create(width, height, 1, false);
        if (!pick.has_value()) {
            opengl::report_error("Error: Renderer::pick_drawables failed to create pick framebuffer");
            return results;
        }
        m_pickFramebuffer = std::make_unique<opengl::Framebuffer>(std::move(*pick));
    }

    m_pickFramebuffer->bind();

    // Restore the scene framebuffer binding on every exit path (including exceptions) so a pick
    // performed mid-frame leaves rendering intact.
    const ScopeExit restoreSceneFramebuffer{[this] { m_sceneFramebuffer->bind(); }};

    // Configure state for exact, un-blended, un-encoded ID output. Clear to black (index 0 = no hit).
    glDisable(GL_FRAMEBUFFER_SRGB);
    glDisable(GL_BLEND);
    glDisable(GL_MULTISAMPLE);
    glEnable(GL_DEPTH_TEST);
    glClearDepth(m_reversedDepth ? 0.0 : 1.0);
    glDepthMask(GL_TRUE);
    glDepthFunc(m_reversedDepth ? GL_GREATER : GL_LESS);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glViewport(0, 0, width, height);
    glClearColor(0.0F, 0.0F, 0.0F, 1.0F);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    const linal::float2 pickViewportSize{static_cast<float>(width), static_cast<float>(height)};
    const std::vector<opengl::DrawablesManager::PickEntry> entries =
        m_drawablesManager->draw_pick_pass(m_camera->get_current_MVP(),
                                           m_camera->get_view_matrix(),
                                           m_camera->get_projection_matrix(),
                                           m_camera->get_inverse_projection_matrix(),
                                           pickViewportSize,
                                           m_reversedDepth);

    // Read back the axis-aligned pixel box that bounds the circular pick region, clamped to the
    // target. Coordinates flip on Y because glReadPixels uses a bottom-left origin.
    const double centerX = xpos;
    const double centerYTop = ypos;
    const double pickRadius = std::max(0.0, radius);
    const bool exactPixelPick = pickRadius == 0.0;

    const int minX = std::max(0, static_cast<int>(std::floor(centerX - pickRadius)));
    const int maxX = exactPixelPick
        ? std::min(width - 1, minX)
        : std::min(width - 1, static_cast<int>(std::ceil(centerX + pickRadius)));
    const int minYTop = std::max(0, static_cast<int>(std::floor(centerYTop - pickRadius)));
    const int maxYTop = exactPixelPick
        ? std::min(height - 1, minYTop)
        : std::min(height - 1, static_cast<int>(std::ceil(centerYTop + pickRadius)));

    const bool exactPixelInBounds = centerX >= 0.0 && centerX < static_cast<double>(width) &&
                                    centerYTop >= 0.0 && centerYTop < static_cast<double>(height);
    if (minX <= maxX && minYTop <= maxYTop && (!exactPixelPick || exactPixelInBounds)) {
        const int boxWidth = maxX - minX + 1;
        const int boxHeight = maxYTop - minYTop + 1;
        const int glReadY = height - 1 - maxYTop; // bottom-left origin of the box

        std::vector<std::uint8_t> pixels(static_cast<std::size_t>(boxWidth) * static_cast<std::size_t>(boxHeight) * 4U);
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glReadPixels(minX, glReadY, boxWidth, boxHeight, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

        const double radiusSquared = pickRadius * pickRadius;
        std::vector<std::uint8_t> seen(entries.size(), 0U); // dedupe by pass index

        for (int row = 0; row < boxHeight; ++row) {
            // Row 0 of the readback is the bottom of the box; map back to a top-left pixel Y.
            const int topY = maxYTop - row;
            for (int col = 0; col < boxWidth; ++col) {
                const int px = minX + col;
                const double dx = static_cast<double>(px) - centerX;
                const double dy = static_cast<double>(topY) - centerYTop;
                if (!exactPixelPick && dx * dx + dy * dy > radiusSquared) {
                    continue;
                }
                const std::size_t base = (static_cast<std::size_t>(row) * static_cast<std::size_t>(boxWidth) +
                                          static_cast<std::size_t>(col)) *
                                         4U;
                const std::uint32_t index = opengl::decode_pick_index(pixels[base], pixels[base + 1], pixels[base + 2]);
                if (index == 0U || index > entries.size()) {
                    continue;
                }
                if (seen[index - 1U] != 0U) {
                    continue;
                }
                seen[index - 1U] = 1U;
                const opengl::DrawablesManager::PickEntry& entry = entries[index - 1U];
                DrawableKind kind = DrawableKind::invalid;
                switch (entry.kind) {
                case opengl::PickDrawableKind::point:  kind = DrawableKind::point; break;
                case opengl::PickDrawableKind::line:   kind = DrawableKind::line; break;
                case opengl::PickDrawableKind::mesh:   kind = DrawableKind::mesh; break;
                case opengl::PickDrawableKind::sphere: kind = DrawableKind::sphere; break;
                }
                results.push_back(PickResult{DrawableHandle{kind, entry.id, m_rendererInstance}, pickRay});
            }
        }
    }

    return results;
}

// --- Frame lifecycle ---

void Renderer::poll_events() {
    GlfwWindow::poll_events();
}

bool Renderer::should_close() const {
    return m_window.should_close();
}

bool Renderer::is_escape_pressed() const {
    return m_window.is_escape_pressed();
}

void Renderer::begin_frame(const renderer::ClearColor& clearColor) {
    make_context_current();
    const auto now = std::chrono::steady_clock::now();
    const double deltaSeconds =
        std::min(maxFrameDeltaSeconds, std::chrono::duration<double>(now - m_lastFrameTime).count());
    m_lastFrameTime = now;

    m_camera->update(deltaSeconds, [this](Key key) {
        return (!m_overlay || !m_overlay->wants_keyboard()) && m_window.is_key_pressed(key);
    });

    maybe_update_auto_fit(now);
    update_clip_planes_from_bounds();

    update_scene_viewport();

    // The scene framebuffers are sized to the scene viewport rect, not the whole window.
    // Geometry is drawn at the origin and the whole scene texture is post-processed and
    // presented at the viewport's window offset, so the reserved UI band is never touched.
    const int sceneWidth = static_cast<int>(valid_framebuffer_dimension(m_sceneViewport.framebuffer.width));
    const int sceneHeight = static_cast<int>(valid_framebuffer_dimension(m_sceneViewport.framebuffer.height));
    const bool dimensionsChanged = m_sceneFramebuffer->get_width() != sceneWidth ||
                                   m_sceneFramebuffer->get_height() != sceneHeight;
    const bool samplesChanged = m_sceneSamples != m_requestedSceneSamples;
    if ((dimensionsChanged || samplesChanged) &&
        !rebuild_scene_targets(sceneWidth, sceneHeight, m_requestedSceneSamples, dimensionsChanged)) {
        if (samplesChanged) {
            m_requestedSceneSamples = m_sceneSamples;
        }
        opengl::report_error("Error: Renderer::begin_frame failed to rebuild framebuffer targets");
        return;
    }

    if (m_sceneFramebuffer->get_width() != sceneWidth || m_sceneFramebuffer->get_height() != sceneHeight) {
        return;
    }
    m_sceneFramebuffer->bind();
    // The scene framebuffer *is* the viewport, so render into all of it starting at the origin.
    opengl::begin_frame(clearColor, renderer::ViewportRect{0, 0, sceneWidth, sceneHeight}, false, m_reversedDepth);
}

void Renderer::draw() {
    const renderer::LightingConfig lighting;
    draw(lighting);
}

void Renderer::draw(const renderer::LightingConfig& lighting) {
    make_context_current();
    if (m_sceneFramebuffer->get_width() !=
            static_cast<int>(valid_framebuffer_dimension(m_sceneViewport.framebuffer.width)) ||
        m_sceneFramebuffer->get_height() !=
            static_cast<int>(valid_framebuffer_dimension(m_sceneViewport.framebuffer.height))) {
        return;
    }

    const linal::float2 sceneViewportSize{static_cast<float>(m_sceneViewport.framebuffer.width),
                                          static_cast<float>(m_sceneViewport.framebuffer.height)};
    {
        const ScopedFullSampleShading sampleShading{m_sceneSamples > 1};
        m_drawablesManager->draw_lines_and_points(
            m_camera->get_current_MVP(), sceneViewportSize, m_camera->get_position());
    }

    const linal::float3 viewPosF{static_cast<float>(m_camera->get_position()[0]),
                                 static_cast<float>(m_camera->get_position()[1]),
                                 static_cast<float>(m_camera->get_position()[2])};
    renderer::LightingConfig effectiveLighting = lighting;
    effectiveLighting.lightPosition = viewPosF;

    if (m_drawablesManager->has_mesh_drawables()) {
        m_drawablesManager->draw_meshes(m_camera->get_view_matrix(),
                                        m_camera->get_projection_matrix(),
                                        viewPosF,
                                        effectiveLighting);
    }

    if (m_drawablesManager->has_sphere_drawables()) {
        const ScopedFullSampleShading sampleShading{m_sceneSamples > 1};
        m_drawablesManager->draw_spheres(m_camera->get_view_matrix(),
                                         m_camera->get_projection_matrix(),
                                         m_camera->get_inverse_projection_matrix(),
                                         sceneViewportSize,
                                         m_reversedDepth,
                                         m_camera->get_position(),
                                         effectiveLighting);
    }
}

void Renderer::end_frame() {
    bool homeRequested = false;
    end_frame(m_cameraAutoFitSettings.enabled, homeRequested);
}

void Renderer::end_frame(bool& autoFitEnabled) {
    bool homeRequested = false;
    end_frame(autoFitEnabled, homeRequested);
}

void Renderer::end_frame(bool& autoFitEnabled, bool& homeRequested) {
    make_context_current();
    if (m_sceneFramebuffer->get_width() !=
            static_cast<int>(valid_framebuffer_dimension(m_sceneViewport.framebuffer.width)) ||
        m_sceneFramebuffer->get_height() !=
            static_cast<int>(valid_framebuffer_dimension(m_sceneViewport.framebuffer.height))) {
        return;
    }

    if (m_overlay) {
        m_overlay->new_frame();
    }

    present_scene();

    if (m_overlay) {
        OverlayFrameContext ctx{*this};
        ctx.autoFitEnabled = autoFitEnabled;
        ctx.projectionType = m_camera->get_projection_type();
        ctx.homeRequested = homeRequested;

        m_overlay->build_controls(ctx);
        m_overlay->render();
        m_overlay->end_frame();

        // The overlay reports the region it leaves free for the scene. It takes effect on the
        // next begin_frame() and only when the application has not set an explicit viewport.
        m_overlaySceneViewport = ctx.sceneViewportHint;

        if (ctx.projectionType != m_camera->get_projection_type()) {
            m_camera->set_projection_type(ctx.projectionType);
        }

        autoFitEnabled = ctx.autoFitEnabled;
        m_cameraAutoFitSettings.enabled = autoFitEnabled;

        homeRequested = ctx.homeRequested;
        if (homeRequested) {
            go_to_home_view();
            homeRequested = false;
        }
    }

    m_window.swap_buffers();
}

void Renderer::make_context_current() const {
    m_window.make_context_current();
}

bool Renderer::rebuild_scene_targets(int width, int height, int samples, bool resizeLdrTarget) {
    const opengl::Framebuffer::HdrConfig sceneConfig{width, height, samples, true, m_reversedDepth};
    auto scene = opengl::Framebuffer::create_hdr(sceneConfig);
    if (!scene.has_value()) {
        return false;
    }

    std::optional<opengl::Framebuffer> resolve;
    if (samples > 1) {
        const opengl::Framebuffer::HdrConfig resolveConfig{width, height, 1, true, m_reversedDepth};
        resolve = opengl::Framebuffer::create_hdr(resolveConfig);
        if (!resolve.has_value()) {
            return false;
        }
    }

    std::optional<opengl::Framebuffer> ldr;
    if (resizeLdrTarget) {
        ldr = opengl::Framebuffer::create_ldr_intermediate(width, height);
        if (!ldr.has_value()) {
            return false;
        }
    }

    m_sceneFramebuffer = std::make_unique<opengl::Framebuffer>(std::move(*scene));
    m_hdrResolveFramebuffer = samples > 1
        ? std::make_unique<opengl::Framebuffer>(std::move(*resolve))
        : nullptr;
    if (ldr.has_value()) {
        m_ldrIntermediate = std::make_unique<opengl::Framebuffer>(std::move(*ldr));
    }
    m_sceneSamples = samples;
    return true;
}

void Renderer::present_scene() {
    GLuint hdrColorTex{0};
    GLuint depthTex{0};
    if (m_sceneSamples > 1) {
        if (!m_sceneFramebuffer->resolve_to(*m_hdrResolveFramebuffer, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)) {
            opengl::report_error("Error: present_scene failed to resolve HDR framebuffer");
            return;
        }
        hdrColorTex = m_hdrResolveFramebuffer->get_color_texture();
        depthTex = m_hdrResolveFramebuffer->get_depth_texture();
    } else {
        hdrColorTex = m_sceneFramebuffer->get_color_texture();
        depthTex = m_sceneFramebuffer->get_depth_texture();
    }

    m_postProcessingPass->set_reversed_depth(m_reversedDepth);
    m_postProcessingPass->set_exposure_stops(m_exposureStops);
    m_postProcessingPass->set_tone_map_mode(static_cast<int>(m_toneMapMode));
    m_postProcessingPass->set_visualization_mode(static_cast<int>(m_visualizationMode));
    m_postProcessingPass->set_hdr_display_max(m_hdrDisplayMax);
    m_postProcessingPass->set_grayscale(m_grayscale);

    // The scene framebuffers are viewport-sized, so post-processing runs over the whole
    // texture at the origin.
    const int w = m_sceneFramebuffer->get_width();
    const int h = m_sceneFramebuffer->get_height();

    m_ldrIntermediate->bind();
    m_postProcessingPass->process(hdrColorTex, depthTex, w, h);

    opengl::Framebuffer::unbind();

    // FXAA presents only the scene viewport rect into the default framebuffer. Clear the whole
    // window first so any region outside the scene (a reserved UI band) is a defined color
    // instead of stale garbage; an overlay or the application draws over it afterwards.
    const auto [windowFramebufferWidth, windowFramebufferHeight] = m_window.get_framebuffer_size();
    glViewport(0,
               0,
               static_cast<GLsizei>(valid_framebuffer_dimension(windowFramebufferWidth)),
               static_cast<GLsizei>(valid_framebuffer_dimension(windowFramebufferHeight)));
    glDisable(GL_SCISSOR_TEST);
    glClearColor(defaultClearColor.r, defaultClearColor.g, defaultClearColor.b, defaultClearColor.a);
    glClear(GL_COLOR_BUFFER_BIT);

    m_fxaaPass->set_enabled(m_fxaaEnabled);
    m_fxaaPass->set_edge_threshold(m_fxaaEdgeThreshold);
    m_fxaaPass->set_edge_threshold_min(m_fxaaEdgeThresholdMin);
    m_fxaaPass->set_subpixel_amount(m_fxaaSubpixelAmount);
    // Present into the default (whole-window) framebuffer at the scene viewport's offset so
    // the scene lands beside the reserved UI band instead of stretching across it.
    m_fxaaPass->process(m_ldrIntermediate->get_color_texture(),
                        w,
                        h,
                        m_sceneViewport.framebuffer.x,
                        m_sceneViewport.framebuffer.y);
}

// --- Post-processing setters ---

void Renderer::set_exposure_stops(float stops) {
    if (!is_finite(stops)) {
        report_invalid_argument("set_exposure_stops");
        return;
    }
    m_exposureStops = stops;
}

void Renderer::set_tone_map_mode(renderer::ToneMapMode mode) {
    if (!is_valid_tone_map_mode(mode)) {
        report_invalid_argument("set_tone_map_mode");
        return;
    }
    m_toneMapMode = mode;
}

void Renderer::set_visualization_mode(renderer::VisualizationMode mode) {
    if (!is_valid_visualization_mode(mode)) {
        report_invalid_argument("set_visualization_mode");
        return;
    }
    m_visualizationMode = mode;
}

void Renderer::set_hdr_display_max(float maxVal) {
    if (!is_finite(maxVal) || maxVal <= 0.0F) {
        report_invalid_argument("set_hdr_display_max");
        return;
    }
    m_hdrDisplayMax = maxVal;
}

void Renderer::set_grayscale(bool enabled) {
    m_grayscale = enabled;
}

void Renderer::set_fxaa_enabled(bool enabled) {
    m_fxaaEnabled = enabled;
}

void Renderer::set_fxaa_edge_threshold(float threshold) {
    if (!is_finite(threshold) || threshold < 0.0F || threshold > maxFxaaEdgeThreshold) {
        report_invalid_argument("set_fxaa_edge_threshold");
        return;
    }
    m_fxaaEdgeThreshold = threshold;
}

void Renderer::set_fxaa_edge_threshold_min(float threshold) {
    if (!is_finite(threshold) || threshold < 0.0F || threshold > maxFxaaEdgeThresholdMin) {
        report_invalid_argument("set_fxaa_edge_threshold_min");
        return;
    }
    m_fxaaEdgeThresholdMin = threshold;
}

void Renderer::set_fxaa_subpixel_amount(float amount) {
    if (!is_finite(amount) || amount < 0.0F || amount > 1.0F) {
        report_invalid_argument("set_fxaa_subpixel_amount");
        return;
    }
    m_fxaaSubpixelAmount = amount;
}

void Renderer::set_msaa_samples(int samples) {
    if (samples <= 0) {
        report_invalid_argument("set_msaa_samples");
        return;
    }
    m_requestedSceneSamples = std::clamp(samples, 1, m_maxSceneSamples);
}

void Renderer::set_overlay(std::shared_ptr<IOverlay> overlay) {
    m_overlay = std::move(overlay);
}

// --- Camera navigation (geometry-fit aware) ---

void Renderer::request_auto_fit() noexcept {
    m_autoFitPending = true;
}

void Renderer::set_camera_auto_fit_settings(const CameraAutoFitSettings& settings) {
    m_cameraAutoFitSettings = settings;
    request_auto_fit();
}

void Renderer::set_camera_far_plane_multiplier(double multiplier) {
    m_cameraFarPlaneMultiplier = multiplier;
    request_auto_fit();
}

CameraAutoFitResult Renderer::compute_fit_destination(const linal::double3& direction,
                                                      const linal::double3& up,
                                                      const linal::double3& targetHint,
                                                      double currentDistance,
                                                      bool suppressZoomIn) const {
    const double distance = std::max(currentDistance, minimumFitDistance);

    CameraAutoFitInput input;
    input.position = targetHint + direction * distance;
    input.target = targetHint;
    input.vertical = up;
    input.projectionType = m_camera->get_projection_type();
    input.verticalFovDegrees = m_camera->get_fov();
    input.aspectRatio = m_camera->get_viewport().get_aspect_ratio();
    input.nearPlane = minimumNearPlane;
    input.farPlaneMultiplier = m_cameraFarPlaneMultiplier;
    input.suppressZoomIn = suppressZoomIn;
    input.settings = m_cameraAutoFitSettings;
    // Automatic callers gate on the renderer-owned enabled state. Explicit
    // Home and preset fits must remain geometry-aware even when it is disabled.
    input.settings.enabled = true;
    input.useReversedDepth = m_reversedDepth;
    const auto orthoParams = m_camera->get_orthographic_params();
    input.orthographicWidth = orthoParams.width;
    input.orthographicHeight = orthoParams.height;

    const std::vector<std::vector<float>> positionBuffers = m_drawablesManager->collect_vertex_position_buffers();
    std::vector<std::span<const float>> positionBufferSpans;
    positionBufferSpans.reserve(positionBuffers.size());
    for (const auto& buffer: positionBuffers) {
        positionBufferSpans.emplace_back(buffer);
    }
    return calculate_camera_auto_fit(std::span<const std::span<const float>>{positionBufferSpans}, input);
}

void Renderer::maybe_update_auto_fit(std::chrono::steady_clock::time_point now) {
    if (m_camera->get_was_blocking()) {
        m_lastCameraInteractionTime = now;
        m_camera->reset_was_blocking();
    }
    if (m_cameraAutoFitSettings.enabled && m_autoFitPending && !m_camera->is_transitioning_view()) {
        const bool suppressZoomIn =
            m_lastCameraInteractionTime.has_value() &&
            (now - *m_lastCameraInteractionTime) < m_cameraAutoFitSettings.suppressAfterUserCameraInteraction;
        const linal::double3 currentPosition = m_camera->get_position();
        const linal::double3 currentTarget = m_camera->get_target();
        const double currentDistance = linal::length(currentPosition - currentTarget);
        const linal::double3 direction = currentDistance > 1.0e-9 ? linal::normalize(currentPosition - currentTarget)
                                                                  : linal::double3{0.0, -1.0, 0.0};

        const CameraAutoFitResult result = compute_fit_destination(direction,
                                                                   m_camera->get_vertical(),
                                                                   currentTarget,
                                                                   currentDistance,
                                                                   suppressZoomIn);
        if (result.hasGeometry && result.changed) {
            m_camera->transition_to_pose(result.position, result.target, result.vertical);
            apply_fit_result(result);
        }
        m_autoFitPending = false;
    }
}

void Renderer::apply_fit_result(const CameraAutoFitResult& result) {
    if (m_camera->get_projection_type() == CameraProjectionType::ORTHOGRAPHIC) {
        m_camera->set_orthographic_size(result.orthographicWidth, result.orthographicHeight);
    }
    apply_clip_planes(result.nearPlane, result.farPlane);
}

void Renderer::apply_clip_planes(double nearPlane, double farPlane) {
    // set_clip_planes validates near < far atomically, so a fitted near that
    // exceeds the previous far (or a fitted far below the previous near) is fine:
    // there is no intermediate single-plane state to violate the invariant.
    m_camera->set_clip_planes(nearPlane, farPlane);
}

void Renderer::update_clip_planes_from_bounds() {
    // Clip-plane fitting runs every frame, independent of the auto-fit toggle:
    // it only adjusts near/far to keep the current geometry inside the frustum
    // and never moves the camera, so it is safe to apply continuously while the
    // user navigates. The auto-fit toggle governs the camera *re-pose* only.
    if (!m_drawablesManager->has_drawables()) {
        return;
    }

    CameraAutoFitInput input;
    input.position = m_camera->get_position();
    input.target = m_camera->get_target();
    input.vertical = m_camera->get_vertical();
    input.projectionType = m_camera->get_projection_type();
    input.verticalFovDegrees = m_camera->get_fov();
    input.aspectRatio = m_camera->get_viewport().get_aspect_ratio();
    input.nearPlane = minimumNearPlane;
    input.farPlaneMultiplier = m_cameraFarPlaneMultiplier;
    input.useReversedDepth = m_reversedDepth;

    const std::vector<std::vector<float>> positionBuffers = m_drawablesManager->collect_vertex_position_buffers();
    std::vector<std::span<const float>> positionBufferSpans;
    positionBufferSpans.reserve(positionBuffers.size());
    for (const auto& buffer: positionBuffers) {
        positionBufferSpans.emplace_back(buffer);
    }

    const CameraClipPlanes planes =
        calculate_clip_planes(std::span<const std::span<const float>>{positionBufferSpans}, input);
    if (planes.hasGeometry) {
        apply_clip_planes(planes.nearPlane, planes.farPlane);
    }
}

void Renderer::go_to_preset_view(PresetView view) {
    const PresetViewDirection preset = preset_view_direction(view);
    const linal::double3 currentTarget = m_camera->get_target();
    const double currentDistance = linal::length(m_camera->get_position() - currentTarget);

    const CameraAutoFitResult result =
        compute_fit_destination(preset.direction, preset.up, currentTarget, currentDistance);

    if (!result.hasGeometry) {
        m_camera->go_to_preset_view(view);
        return;
    }

    m_camera->transition_to_pose(result.position, result.target, result.vertical);
    apply_fit_result(result);
}

void Renderer::go_to_home_view() {
    const linal::double3 defaultPosition = m_camera->get_default_position();
    const linal::double3 defaultTarget = m_camera->get_default_target();
    const linal::double3 defaultUp = m_camera->get_default_up();
    const linal::double3 direction = linal::normalize(defaultPosition - defaultTarget);
    const double defaultDistance = linal::length(defaultPosition - defaultTarget);

    const CameraAutoFitResult result = compute_fit_destination(direction, defaultUp, defaultTarget, defaultDistance);

    if (!result.hasGeometry) {
        m_camera->transition_to_pose(defaultPosition, defaultTarget, defaultUp);
        return;
    }

    m_camera->transition_to_pose(result.position, result.target, result.vertical);
    apply_fit_result(result);
}

void Renderer::refit_current_view() {
    const linal::double3 currentPosition = m_camera->get_position();
    const linal::double3 currentTarget = m_camera->get_target();
    const double currentDistance = linal::length(currentPosition - currentTarget);
    // Degenerate pose (camera on its target): fall back to the default direction so normalize is
    // well-defined. This mirrors maybe_update_auto_fit's guard.
    const linal::double3 direction =
        currentDistance > 1.0e-9 ? linal::normalize(currentPosition - currentTarget) : linal::double3{0.0, -1.0, 0.0};

    const CameraAutoFitResult result =
        compute_fit_destination(direction, m_camera->get_vertical(), currentTarget, currentDistance);

    if (!result.hasGeometry) {
        return;
    }

    m_camera->transition_to_pose(result.position, result.target, result.vertical);
    apply_fit_result(result);
}

// --- Callback extension ---

template <typename Callback>
CallbackSubscription Renderer::add_callback(std::vector<CallbackEntry<Callback>>& callbacks, Callback callback) {
    auto connection = std::make_shared<CallbackConnection>();
    callbacks.push_back(CallbackEntry<Callback>{std::move(callback), connection});
    return CallbackSubscription{std::move(connection)};
}

template <typename Callback, typename... Args>
void Renderer::dispatch_callbacks(std::vector<CallbackEntry<Callback>>& callbacks, Args&&... args) {
    const auto snapshot = callbacks;
    for (const auto& entry: snapshot) {
        if (entry.connection->connected) {
            entry.callback(std::forward<Args>(args)...);
        }
    }
    std::erase_if(callbacks, [](const CallbackEntry<Callback>& entry) { return !entry.connection->connected; });
}

CallbackSubscription Renderer::add_cursor_pos_callback(CursorPosCB cb) {
    return add_callback(m_cursorPosCallbacks, std::move(cb));
}
CallbackSubscription Renderer::add_scroll_callback(ScrollCB cb) {
    return add_callback(m_scrollCallbacks, std::move(cb));
}
CallbackSubscription Renderer::add_mouse_button_callback(MouseBtnCB cb) {
    return add_callback(m_mouseButtonCallbacks, std::move(cb));
}
CallbackSubscription Renderer::add_key_callback(KeyCB cb) {
    return add_callback(m_keyCallbacks, std::move(cb));
}

void Renderer::update_scene_viewport() {
    const auto windowSize = m_window.get_window_size();
    // Precedence: an explicit application request wins; otherwise the active overlay's
    // reserved region; otherwise the whole window.
    const LogicalViewportRect fullWindow{0.0,
                                         0.0,
                                         valid_logical_dimension(windowSize.first),
                                         valid_logical_dimension(windowSize.second)};
    const LogicalViewportRect logicalRect =
        m_requestedSceneViewport.value_or(m_overlaySceneViewport.value_or(fullWindow));
    m_sceneViewport = Renderer::calculate_scene_viewport(windowSize, m_window.get_framebuffer_size(), logicalRect);
    m_camera->set_viewport(0,
                           0,
                           valid_framebuffer_dimension(m_sceneViewport.framebuffer.width),
                           valid_framebuffer_dimension(m_sceneViewport.framebuffer.height));
}

void Renderer::set_scene_viewport(std::optional<LogicalViewportRect> logicalRect) {
    m_requestedSceneViewport = logicalRect;
    update_scene_viewport();
}

std::optional<std::pair<double, double>> Renderer::current_scene_framebuffer_coordinates() const {
    return Renderer::to_scene_framebuffer_coordinates(m_sceneViewport,
                                                      m_lastWindowCursorPos.xpos,
                                                      m_lastWindowCursorPos.ypos);
}

} // namespace renderer
