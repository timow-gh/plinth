#include <OpenGL/Drawable/DrawablesManager.hpp>
#include <OpenGL/ErrorReporting.hpp>
#include <OpenGL/FXAAPass.hpp>
#include <OpenGL/FrameState.hpp>
#include <OpenGL/Framebuffer.hpp>
#include <OpenGL/GpuCapabilities.hpp>
#include <OpenGL/OpenGL.hpp>
#include <OpenGL/PostProcessingPass.hpp>
#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <plinth/Renderer.hpp>
#include <string>
#include <string_view>

namespace renderer {

struct CallbackConnection {
    bool connected{true};
};

CallbackSubscription::CallbackSubscription(std::shared_ptr<CallbackConnection> connection)
    : m_connection(std::move(connection)) {}

CallbackSubscription::CallbackSubscription(CallbackSubscription&& other) noexcept = default;
CallbackSubscription& CallbackSubscription::operator=(CallbackSubscription&& other) noexcept {
    if (this != &other) {
        disconnect();
        m_connection = std::move(other.m_connection);
    }
    return *this;
}
CallbackSubscription::~CallbackSubscription() { disconnect(); }

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
    m_imgui.reset();
    m_imguiLifetime.reset();
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
constexpr double maxFrameDeltaSeconds = 0.1;
constexpr CameraAutoFitSettings defaultAutoFitSettings{};

constexpr float maxFxaaEdgeThreshold = 0.5F;
constexpr float maxFxaaEdgeThresholdMin = 0.25F;

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

bool is_valid_fog_mode(FogMode mode) {
    return mode == FogMode::Linear || mode == FogMode::Exponential;
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

} // namespace

SceneViewport Renderer::calculate_scene_viewport(std::pair<int, int> windowSize,
                                                 std::pair<int, int> framebufferSize,
                                                 double reservedLogicalWidth) {
    const double logicalWindowWidth = valid_logical_dimension(windowSize.first);
    const double logicalWindowHeight = valid_logical_dimension(windowSize.second);
    const int framebufferWidth = std::max(1, framebufferSize.first);
    const int framebufferHeight = std::max(1, framebufferSize.second);

    const double sceneX = std::clamp(reservedLogicalWidth, 0.0, logicalWindowWidth - 1.0);
    const double sceneWidth = std::max(1.0, logicalWindowWidth - sceneX);
    const double xScale = static_cast<double>(framebufferWidth) / logicalWindowWidth;

    const int framebufferX = std::clamp(round_to_framebuffer_pixel(sceneX * xScale), 0, framebufferWidth - 1);

    SceneViewport viewport;
    viewport.logical = LogicalViewportRect{sceneX, 0.0, sceneWidth, logicalWindowHeight};
    viewport.framebuffer =
        renderer::ViewportRect{framebufferX, 0, std::max(1, framebufferWidth - framebufferX), framebufferHeight};
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
    const auto [fbWidth, fbHeight] = window->get_framebuffer_size();
    CameraSettings cameraSettings;
    cameraSettings.m_defaultPosition = defaultCameraPosition;
    cameraSettings.m_defaultTarget = linal::double3{0.0, 0.0, 0.0};
    cameraSettings.m_defaultUp = linal::double3{0.0, 0.0, 1.0};
    cameraSettings.m_camera.set_viewport(0,
                                         0,
                                         valid_framebuffer_dimension(fbWidth),
                                         valid_framebuffer_dimension(fbHeight));
    auto camera = std::make_shared<CameraInteractor>(window->get_input_state(), cameraSettings);

    auto imgui = std::make_unique<ImGuiOverlay>(window->get_native_handle());

    auto drawablesManager = opengl::DrawablesManager::create();
    if (!drawablesManager) {
        opengl::report_error(
            "Error: Renderer::create failed - GL program/drawables-manager creation failed (see above)");
        return nullptr;
    }

    GLint maxSamples{0};
    glGetIntegerv(GL_MAX_SAMPLES, &maxSamples);
    if (maxSamples < 1) {
        opengl::report_error("Error: Renderer::create failed - GL_MAX_SAMPLES is less than one");
        return nullptr;
    }
    const int sceneSamples = std::clamp(settings.samples, 1, maxSamples);
    const int framebufferWidth = static_cast<int>(valid_framebuffer_dimension(fbWidth));
    const int framebufferHeight = static_cast<int>(valid_framebuffer_dimension(fbHeight));

    opengl::Framebuffer::HdrConfig hdrConfig{framebufferWidth, framebufferHeight, sceneSamples, true};
    auto hdrSceneFb = opengl::Framebuffer::create_hdr(hdrConfig);
    if (!hdrSceneFb.has_value()) {
        opengl::report_error("Error: Renderer::create failed - HDR scene framebuffer creation failed");
        return nullptr;
    }

    std::unique_ptr<opengl::Framebuffer> hdrResolveFb;
    if (sceneSamples > 1) {
        opengl::Framebuffer::HdrConfig resolveConfig{framebufferWidth, framebufferHeight, 1, true};
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

    std::unique_ptr<Renderer> renderer(new Renderer(std::move(window.value()),
                                                       std::move(drawablesManager),
                                                       std::move(camera),
                                                       std::move(imgui),
                                                       std::make_unique<opengl::Framebuffer>(std::move(*hdrSceneFb)),
                                                       std::move(hdrResolveFb),
                                                        std::make_unique<opengl::Framebuffer>(std::move(*ldrFb)),
                                                        std::make_unique<opengl::PostProcessingPass>(std::move(*postProcess)),
                                                       std::make_unique<opengl::FXAAPass>(std::move(*fxaa)),
                                                       sceneSamples,
                                                        capabilities.maxTextureSize,
                                                        *rendererInstance));

    renderer->update_scene_viewport();
    renderer->wire_callbacks();

    return renderer;
}

Renderer::Renderer(GlfwWindow window,
                    std::unique_ptr<opengl::DrawablesManager> drawables,
                    std::shared_ptr<CameraInteractor> camera,
                    std::unique_ptr<ImGuiOverlay> imgui,
                    std::unique_ptr<opengl::Framebuffer> sceneFramebuffer,
                    std::unique_ptr<opengl::Framebuffer> hdrResolveFramebuffer,
                     std::unique_ptr<opengl::Framebuffer> ldrIntermediate,
                     std::unique_ptr<opengl::PostProcessingPass> postProcessingPass,
                     std::unique_ptr<opengl::FXAAPass> fxaaPass,
                     int sceneSamples,
                     int maxTextureSize,
                     std::uint64_t rendererInstance)
    : m_window(std::move(window))
    , m_drawablesManager(std::move(drawables))
    , m_camera(std::move(camera))
    , m_imgui(std::move(imgui))
    , m_sceneFramebuffer(std::move(sceneFramebuffer))
    , m_hdrResolveFramebuffer(std::move(hdrResolveFramebuffer))
    , m_ldrIntermediate(std::move(ldrIntermediate))
    , m_postProcessingPass(std::move(postProcessingPass))
    , m_fxaaPass(std::move(fxaaPass))
    , m_sceneSamples(sceneSamples)
    , m_lastFrameTime(std::chrono::steady_clock::now())
    , m_lastCameraInteractionTime(m_lastFrameTime)
    , m_maxTextureSize(maxTextureSize)
    , m_rendererInstance(rendererInstance) {
}

void Renderer::on_cursor_pos(double xpos, double ypos) {
    m_lastWindowCursorPos = CursorPosState{xpos, ypos};
    if (!m_imgui->handle_cursor_position(xpos, ypos)) {
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
    if (!m_imgui->handle_scroll(xoff, yoff)) {
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
    if (!m_imgui->handle_mouse_button(button, action, mods)) {
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
    if (action == Action::PRESS) {
        m_cameraMouseInteractionActive =
            sceneCoordinates.has_value() && (button == 1 || button == 2);
    }
    m_camera->on_mouse_button(button, action, mods);
    if (action == Action::RELEASE) {
        m_cameraMouseInteractionActive = false;
    }
    dispatch_callbacks(m_mouseButtonCallbacks, button, action, mods);
}

void Renderer::wire_callbacks() {
    m_window.set_cursor_pos_callback([this](double xpos, double ypos) { on_cursor_pos(xpos, ypos); });

    m_window.set_scroll_callback([this](double xoff, double yoff) { on_scroll(xoff, yoff); });

    m_window.set_mouse_button_callback(
        [this](int button, Action action, Mods mods) { on_mouse_button(button, action, mods); });

    m_window.set_key_callback([this](Key key, Scancode scancode, Action action, Mods mods) {
        const bool forward = m_imgui->handle_key(key, scancode, action, mods);
        if (!forward) {
            return;
        }
        if (key == Key::KEY_H && action == Action::PRESS && !m_imgui->wants_keyboard()) {
            go_to_home_view();
        }
        dispatch_callbacks(m_keyCallbacks, key, scancode, action, mods);
    });

    m_window.set_char_callback([this](std::uint32_t codepoint) { m_imgui->handle_char(codepoint); });

    m_window.set_framebuffer_size_callback([this]([[maybe_unused]] std::uint32_t width,
                                                   [[maybe_unused]]
                                                   std::uint32_t height) { update_scene_viewport(); });
}

// --- Geometry ---

DrawableHandle Renderer::add_point_drawable(std::span<const float> vertices,
                                            std::span<const float> colors,
                                            std::span<const std::uint32_t> indices,
                                            float pointSize,
                                            renderer::BufferAccessPattern accessPattern) {
    const auto id = m_drawablesManager->add_point_drawable(vertices, colors, indices, pointSize, accessPattern);
    if (!id.has_value()) {
        return DrawableHandle{};
    }
    return DrawableHandle{DrawableKind::point, *id, m_rendererInstance};
}

DrawableHandle Renderer::add_line_drawable(std::span<const float> vertices,
                                           std::span<const std::uint32_t> indices,
                                           std::span<const float> colors,
                                           renderer::LineType lineType,
                                           float lineWidth,
                                           float pointSize,
                                           renderer::BufferAccessPattern accessPattern) {
    const auto id =
        m_drawablesManager->add_line_drawable(vertices, indices, colors, lineType, lineWidth, pointSize, accessPattern);
    if (!id.has_value()) {
        return DrawableHandle{};
    }
    return DrawableHandle{DrawableKind::line, *id, m_rendererInstance};
}

DrawableHandle Renderer::add_mesh_drawable(std::span<const float> vertices,
                                           std::span<const float> normals,
                                           std::span<const float> colors,
                                           std::span<const std::uint32_t> triangleIndices,
                                           renderer::BufferAccessPattern accessPattern) {
    const auto id =
        m_drawablesManager->add_mesh_drawable(vertices, 3, normals, colors, 4, triangleIndices, accessPattern);
    if (!id.has_value()) {
        return DrawableHandle{};
    }
    return DrawableHandle{DrawableKind::mesh, *id, m_rendererInstance};
}

TextureHandle Renderer::create_texture_2d(TextureData data) {
    const auto id = m_drawablesManager->create_texture_2d(data, m_maxTextureSize);
    return id ? TextureHandle{*id, m_rendererInstance} : TextureHandle{};
}

bool Renderer::remove_texture(TextureHandle texture) {
    return texture.is_valid() && texture.rendererInstance == m_rendererInstance &&
           m_drawablesManager->remove_texture(texture.id);
}

DrawableHandle Renderer::add_textured_mesh_drawable(std::span<const float> vertices, std::span<const float> normals,
                                                     std::span<const float> textureCoordinates, std::span<const float> colors,
                                                     std::span<const std::uint32_t> triangleIndices, TextureHandle texture,
                                                     renderer::BufferAccessPattern accessPattern) {
    if (!texture.is_valid() || texture.rendererInstance != m_rendererInstance) {
        return {};
    }
    const auto id = m_drawablesManager->add_textured_mesh_drawable(vertices, 3, normals, textureCoordinates, colors, 4,
                                                                      triangleIndices, texture.id, accessPattern);
    return id ? DrawableHandle{DrawableKind::mesh, *id, m_rendererInstance} : DrawableHandle{};
}

DrawableHandle Renderer::add_mesh_segment_drawable(std::span<const float> positions,
                                                   std::span<const std::uint32_t> indices,
                                                   std::span<const float> color,
                                                   float lineWidth) {
    const auto id = m_drawablesManager->add_mesh_segment_drawable(positions, indices, color, lineWidth);
    if (!id.has_value()) {
        return DrawableHandle{};
    }
    return DrawableHandle{DrawableKind::meshSegment, *id, m_rendererInstance};
}

DrawableHandle
Renderer::add_mesh_vertex_drawable(std::span<const float> positions, std::span<const float> color, float pointSize) {
    const auto id = m_drawablesManager->add_mesh_vertex_drawable(positions, color, pointSize);
    if (!id.has_value()) {
        return DrawableHandle{};
    }
    return DrawableHandle{DrawableKind::meshVertex, *id, m_rendererInstance};
}

void Renderer::set_mesh_drawable_cull_mode(DrawableHandle handle, renderer::MeshCullFaceMode mode) {
    if (handle.kind == DrawableKind::mesh && handle.is_valid() && handle.rendererInstance == m_rendererInstance) {
        m_drawablesManager->set_mesh_drawable_cull_mode(handle.id, mode);
    }
}

bool Renderer::remove_drawable(DrawableHandle handle) {
    if (!handle.is_valid() || handle.rendererInstance != m_rendererInstance) {
        return false;
    }

    switch (handle.kind) {
    case DrawableKind::point:       return m_drawablesManager->remove_point_drawable(handle.id);
    case DrawableKind::line:        return m_drawablesManager->remove_line_drawable(handle.id);
    case DrawableKind::mesh:        return m_drawablesManager->remove_mesh_drawable(handle.id);
    case DrawableKind::meshSegment: return m_drawablesManager->remove_mesh_segment_drawable(handle.id);
    case DrawableKind::meshVertex:  return m_drawablesManager->remove_mesh_vertex_drawable(handle.id);
    case DrawableKind::invalid:     return false;
    }

    return false;
}

bool Renderer::set_drawable_transform(DrawableHandle handle, const linal::hmatf& transform) {
    if (!handle.is_valid() || handle.rendererInstance != m_rendererInstance) {
        return false;
    }

    switch (handle.kind) {
    case DrawableKind::point:       return m_drawablesManager->set_point_drawable_transform(handle.id, transform);
    case DrawableKind::line:        return m_drawablesManager->set_line_drawable_transform(handle.id, transform);
    case DrawableKind::mesh:        return m_drawablesManager->set_mesh_drawable_transform(handle.id, transform);
    case DrawableKind::meshSegment: return m_drawablesManager->set_mesh_segment_drawable_transform(handle.id, transform);
    case DrawableKind::meshVertex:  return m_drawablesManager->set_mesh_vertex_drawable_transform(handle.id, transform);
    case DrawableKind::invalid:     return false;
    }

    return false;
}

std::optional<linal::hmatf> Renderer::get_drawable_transform(DrawableHandle handle) const {
    if (!handle.is_valid() || handle.rendererInstance != m_rendererInstance) {
        return std::nullopt;
    }

    switch (handle.kind) {
    case DrawableKind::point:       return m_drawablesManager->get_point_drawable_transform(handle.id);
    case DrawableKind::line:        return m_drawablesManager->get_line_drawable_transform(handle.id);
    case DrawableKind::mesh:        return m_drawablesManager->get_mesh_drawable_transform(handle.id);
    case DrawableKind::meshSegment: return m_drawablesManager->get_mesh_segment_drawable_transform(handle.id);
    case DrawableKind::meshVertex:  return m_drawablesManager->get_mesh_vertex_drawable_transform(handle.id);
    case DrawableKind::invalid:     return std::nullopt;
    }

    return std::nullopt;
}

bool Renderer::reset_drawable_transform(DrawableHandle handle) {
    return set_drawable_transform(handle, linal::hmatf::identity());
}

void Renderer::update_last_point_drawable(std::span<const float> vertices,
                                          std::span<const float> colors,
                                          std::span<const std::uint32_t> indices,
                                          renderer::BufferAccessPattern accessPattern) {
    m_drawablesManager->update_last_point_drawable(vertices, colors, indices, accessPattern);
}

void Renderer::update_last_line_drawable(std::span<const float> vertices,
                                         std::span<const float> colors,
                                         std::span<const std::uint32_t> indices,
                                         renderer::BufferAccessPattern accessPattern) {
    m_drawablesManager->update_last_line_drawable(vertices, colors, indices, accessPattern);
}

void Renderer::clear_point_drawables() {
    m_drawablesManager->clear_point_drawables();
}
void Renderer::clear_line_drawables() {
    m_drawablesManager->clear_line_drawables();
}
void Renderer::clear_mesh_drawables() {
    m_drawablesManager->clear_mesh_drawables();
}
void Renderer::clear_drawables() {
    m_drawablesManager->clear_drawables();
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
        return !m_imgui->wants_keyboard() && m_window.is_key_pressed(key);
    });

    maybe_update_auto_fit(now);

    update_scene_viewport();

    const auto [windowFramebufferWidth, windowFramebufferHeight] = m_window.get_framebuffer_size();
    const int framebufferWidth = static_cast<int>(valid_framebuffer_dimension(windowFramebufferWidth));
    const int framebufferHeight = static_cast<int>(valid_framebuffer_dimension(windowFramebufferHeight));
    if (m_sceneFramebuffer->get_width() != framebufferWidth || m_sceneFramebuffer->get_height() != framebufferHeight) {
        opengl::Framebuffer::HdrConfig hdrConfig{framebufferWidth, framebufferHeight, m_sceneSamples, true};
        auto scene = opengl::Framebuffer::create_hdr(hdrConfig);
        std::optional<opengl::Framebuffer> resolve;
        std::optional<opengl::Framebuffer> ldr;
        if (scene.has_value()) {
            if (m_sceneSamples > 1) {
                opengl::Framebuffer::HdrConfig resolveConfig{framebufferWidth, framebufferHeight, 1, true};
                resolve = opengl::Framebuffer::create_hdr(resolveConfig);
            }
            ldr = opengl::Framebuffer::create_ldr_intermediate(framebufferWidth, framebufferHeight);
        }
        if (!scene.has_value() || (m_sceneSamples > 1 && !resolve.has_value()) || !ldr.has_value()) {
            opengl::report_error("Error: Renderer::begin_frame failed to resize framebuffer targets");
            return;
        }
        m_sceneFramebuffer = std::make_unique<opengl::Framebuffer>(std::move(*scene));
        if (m_sceneSamples > 1) {
            m_hdrResolveFramebuffer = std::make_unique<opengl::Framebuffer>(std::move(*resolve));
        }
        m_ldrIntermediate = std::make_unique<opengl::Framebuffer>(std::move(*ldr));
    }

    if (m_sceneFramebuffer->get_width() != framebufferWidth || m_sceneFramebuffer->get_height() != framebufferHeight) {
        return;
    }
    m_sceneFramebuffer->bind();
    opengl::begin_frame(clearColor, m_sceneViewport.framebuffer, false);
}

void Renderer::draw() {
    const renderer::LightingConfig lighting;
    draw(lighting);
}

void Renderer::draw(const renderer::LightingConfig& lighting) {
    make_context_current();
    const auto [windowFramebufferWidth, windowFramebufferHeight] = m_window.get_framebuffer_size();
    if (m_sceneFramebuffer->get_width() != static_cast<int>(valid_framebuffer_dimension(windowFramebufferWidth)) ||
        m_sceneFramebuffer->get_height() != static_cast<int>(valid_framebuffer_dimension(windowFramebufferHeight))) {
        return;
    }

    m_drawablesManager->draw_lines_and_points(m_camera->get_current_MVP(), m_camera->get_position());

    if (m_drawablesManager->has_mesh_drawables()) {
        const linal::float3 viewPosF{static_cast<float>(m_camera->get_position()[0]),
                                     static_cast<float>(m_camera->get_position()[1]),
                                     static_cast<float>(m_camera->get_position()[2])};

        renderer::LightingConfig effectiveLighting = lighting;
        effectiveLighting.lightPosition = viewPosF;
        m_drawablesManager->draw_meshes(m_camera->get_view_matrix(),
                                       m_camera->get_projection_matrix(),
                                       viewPosF,
                                       effectiveLighting);
    }

    if (m_drawablesManager->has_mesh_segment_drawables()) {
        m_drawablesManager->draw_mesh_segment_overlays(m_camera->get_current_MVP());
    }

    if (m_drawablesManager->has_mesh_vertex_drawables()) {
        m_drawablesManager->draw_mesh_vertex_overlays(m_camera->get_current_MVP());
    }
}

void Renderer::end_frame() {
    bool homeRequested = false;
    end_frame(m_autoFitEnabled, homeRequested);
}

void Renderer::end_frame(bool& autoFitEnabled) {
    bool homeRequested = false;
    end_frame(autoFitEnabled, homeRequested);
}

void Renderer::end_frame(bool& autoFitEnabled, bool& homeRequested) {
    make_context_current();
    const auto [windowFramebufferWidth, windowFramebufferHeight] = m_window.get_framebuffer_size();
    if (m_sceneFramebuffer->get_width() != static_cast<int>(valid_framebuffer_dimension(windowFramebufferWidth)) ||
        m_sceneFramebuffer->get_height() != static_cast<int>(valid_framebuffer_dimension(windowFramebufferHeight))) {
        return;
    }

    m_imgui->new_frame();
    CameraProjectionType projectionType = m_camera->get_projection_type();
    m_imgui->add_camera_controls(autoFitEnabled, projectionType, homeRequested);

    std::array<float, 3> fogColorArr{m_fogColorR, m_fogColorG, m_fogColorB};
    m_imgui->add_post_processing_controls(
        m_exposureStops,
        m_toneMapMode,
        m_fogEnabled, m_fogMode, m_fogStart, m_fogEnd, m_fogDensity,
        fogColorArr,
        m_visualizationMode, m_hdrDisplayMax,
        m_grayscale,
        m_fxaaEnabled, m_fxaaEdgeThreshold, m_fxaaEdgeThresholdMin, m_fxaaSubpixelAmount);
    m_fogColorR = fogColorArr[0];
    m_fogColorG = fogColorArr[1];
    m_fogColorB = fogColorArr[2];

    present_scene();
    m_imgui->render();
    m_imgui->end_frame();

    if (projectionType != m_camera->get_projection_type()) {
        m_camera->set_projection_type(projectionType);
    }

    m_autoFitEnabled = autoFitEnabled;

    if (homeRequested) {
        go_to_home_view();
        homeRequested = false;
    }

    m_window.swap_buffers();
}

void Renderer::make_context_current() const {
    m_window.make_context_current();
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

    const linal::hmatf projMat = m_camera->get_projection_matrix();
    const linal::hmatf invProjMat = linal::hmatf::inverse(projMat);
    m_postProcessingPass->set_inv_projection(invProjMat.data());

    m_postProcessingPass->set_fog_enabled(m_fogEnabled);
    m_postProcessingPass->set_fog_mode(static_cast<int>(m_fogMode));
    m_postProcessingPass->set_fog_start(m_fogStart);
    m_postProcessingPass->set_fog_end(m_fogEnd);
    m_postProcessingPass->set_fog_density(m_fogDensity);
    m_postProcessingPass->set_fog_color(m_fogColorR, m_fogColorG, m_fogColorB);
    m_postProcessingPass->set_exposure_stops(m_exposureStops);
    m_postProcessingPass->set_tone_map_mode(static_cast<int>(m_toneMapMode));
    m_postProcessingPass->set_visualization_mode(static_cast<int>(m_visualizationMode));
    m_postProcessingPass->set_hdr_display_max(m_hdrDisplayMax);
    m_postProcessingPass->set_grayscale(m_grayscale);

    const int w = m_sceneFramebuffer->get_width();
    const int h = m_sceneFramebuffer->get_height();

    m_ldrIntermediate->bind();
    m_postProcessingPass->process(hdrColorTex, depthTex, w, h);

    opengl::Framebuffer::unbind();
    glDisable(GL_FRAMEBUFFER_SRGB);

    m_fxaaPass->set_enabled(m_fxaaEnabled);
    m_fxaaPass->set_edge_threshold(m_fxaaEdgeThreshold);
    m_fxaaPass->set_edge_threshold_min(m_fxaaEdgeThresholdMin);
    m_fxaaPass->set_subpixel_amount(m_fxaaSubpixelAmount);
    m_fxaaPass->process(m_ldrIntermediate->get_color_texture(), w, h);
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

void Renderer::set_fog_enabled(bool enabled) {
    m_fogEnabled = enabled;
}

void Renderer::set_fog_mode(renderer::FogMode mode) {
    if (!is_valid_fog_mode(mode)) {
        report_invalid_argument("set_fog_mode");
        return;
    }
    m_fogMode = mode;
}

void Renderer::set_fog_start(float start) {
    if (!is_finite(start) || start >= m_fogEnd) {
        report_invalid_argument("set_fog_start");
        return;
    }
    m_fogStart = start;
}

void Renderer::set_fog_end(float end) {
    if (!is_finite(end) || end <= m_fogStart) {
        report_invalid_argument("set_fog_end");
        return;
    }
    m_fogEnd = end;
}

void Renderer::set_fog_density(float density) {
    if (!is_finite(density) || density < 0.0F) {
        report_invalid_argument("set_fog_density");
        return;
    }
    m_fogDensity = density;
}

void Renderer::set_fog_color(float r, float g, float b) {
    if (!is_finite(r) || !is_finite(g) || !is_finite(b)) {
        report_invalid_argument("set_fog_color");
        return;
    }
    m_fogColorR = r;
    m_fogColorG = g;
    m_fogColorB = b;
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

// --- Camera navigation (geometry-fit aware) ---

CameraAutoFitResult Renderer::compute_fit_destination(const linal::double3& direction,
                                                      const linal::double3& up,
                                                      const linal::double3& targetHint,
                                                      double currentDistance) const {
    const double distance = std::max(currentDistance, minimumFitDistance);

    CameraAutoFitInput input;
    input.position = targetHint + direction * distance;
    input.target = targetHint;
    input.vertical = up;
    input.projectionType = m_camera->get_projection_type();
    input.verticalFovDegrees = m_camera->get_fov();
    input.aspectRatio = m_camera->get_viewport().get_aspect_ratio();
    input.nearPlane = m_camera->get_near_plane();
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
        m_autoFitPending = true;
        m_camera->reset_was_blocking();
        return;
    }
    if (m_autoFitEnabled && m_autoFitPending && !m_camera->is_transitioning_view() &&
        (now - m_lastCameraInteractionTime) >= defaultAutoFitSettings.suppressAfterUserCameraInteraction) {
        const linal::double3 currentPosition = m_camera->get_position();
        const linal::double3 currentTarget = m_camera->get_target();
        const double currentDistance = linal::length(currentPosition - currentTarget);
        const linal::double3 direction =
            currentDistance > 1.0e-9 ? linal::normalize(currentPosition - currentTarget) : linal::double3{0.0, -1.0, 0.0};

        const CameraAutoFitResult result =
            compute_fit_destination(direction, m_camera->get_vertical(), currentTarget, currentDistance);
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
    m_camera->set_far_plane(result.farPlane);
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
    double reservedWidth = 0.0;
    if (m_imgui) {
        reservedWidth = static_cast<double>(m_imgui->get_reserved_control_panel_width());
    }
    m_sceneViewport =
        Renderer::calculate_scene_viewport(m_window.get_window_size(), m_window.get_framebuffer_size(), reservedWidth);
    m_camera->set_viewport(0,
                           0,
                           valid_framebuffer_dimension(m_sceneViewport.framebuffer.width),
                           valid_framebuffer_dimension(m_sceneViewport.framebuffer.height));
}

std::optional<std::pair<double, double>> Renderer::current_scene_framebuffer_coordinates() const {
    return Renderer::to_scene_framebuffer_coordinates(m_sceneViewport,
                                                      m_lastWindowCursorPos.xpos,
                                                      m_lastWindowCursorPos.ypos);
}

} // namespace renderer
