#include <OpenGL/Drawable/DrawablesManager.hpp>
#include <OpenGL/ErrorReporting.hpp>
#include <OpenGL/FrameState.hpp>
#include <OpenGL/Framebuffer.hpp>
#include <OpenGL/OpenGL.hpp>
#include <OpenGL/PresentationPass.hpp>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <plinth/Renderer.hpp>

namespace renderer {

Renderer::~Renderer() = default;

namespace {

constexpr linal::double3 defaultCameraPosition{5.0, 5.0, 5.0};
// Safe floor for any hypothetical camera-to-target distance fed into a fit/transition
// computation, so CameraInteractor's degenerate-distance assertions can never be reached
// regardless of the user's prior zoom state.
constexpr double minimumFitDistance = 1.0;
// Caps the per-frame delta fed to CameraInteractor::update() so a long gap between frames (e.g.
// asset loading between Renderer construction and the first begin_frame() call, or any other
// stall) can't translate into a single huge fly-mode jump or transition skip.
constexpr double maxFrameDeltaSeconds = 0.1;
// Reuse CameraAutoFitSettings' own default suppression window rather than duplicating the
// magic number here.
constexpr CameraAutoFitSettings defaultAutoFitSettings{};

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

    auto imgui = std::make_shared<ImGuiOverlay>(window->get_native_handle());

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
    const bool srgb = window->is_srgb_capable();
    auto sceneFramebuffer = opengl::Framebuffer::create(framebufferWidth, framebufferHeight, sceneSamples, srgb);
    if (!sceneFramebuffer.has_value()) {
        opengl::report_error("Error: Renderer::create failed - scene framebuffer creation failed");
        return nullptr;
    }

    std::unique_ptr<opengl::Framebuffer> resolveFramebuffer;
    if (sceneSamples > 1) {
        auto resolve = opengl::Framebuffer::create(framebufferWidth, framebufferHeight, 1, srgb);
        if (!resolve.has_value()) {
            opengl::report_error("Error: Renderer::create failed - resolve framebuffer creation failed");
            return nullptr;
        }
        resolveFramebuffer = std::make_unique<opengl::Framebuffer>(std::move(*resolve));
    }

    auto presentationPass = opengl::PresentationPass::create();
    if (!presentationPass.has_value()) {
        opengl::report_error("Error: Renderer::create failed - presentation pass creation failed");
        return nullptr;
    }

    // Use raw new because the constructor is private and Renderer is non-movable.
    std::unique_ptr<Renderer> renderer(new Renderer(std::move(window.value()),
                                                      std::move(drawablesManager),
                                                      std::move(camera),
                                                      std::move(imgui),
                                                      std::make_unique<opengl::Framebuffer>(std::move(*sceneFramebuffer)),
                                                      std::move(resolveFramebuffer),
                                                      std::make_unique<opengl::PresentationPass>(std::move(*presentationPass)),
                                                      sceneSamples));

    renderer->update_scene_viewport();
    renderer->wire_callbacks();

    return renderer;
}

Renderer::Renderer(GlfwWindow window,
                    std::unique_ptr<opengl::DrawablesManager> drawables,
                    std::shared_ptr<CameraInteractor> camera,
                    std::shared_ptr<ImGuiOverlay> imgui,
                    std::unique_ptr<opengl::Framebuffer> sceneFramebuffer,
                    std::unique_ptr<opengl::Framebuffer> resolveFramebuffer,
                    std::unique_ptr<opengl::PresentationPass> presentationPass,
                    int sceneSamples)
    : m_window(std::move(window))
    , m_drawablesManager(std::move(drawables))
    , m_camera(std::move(camera))
    , m_imgui(std::move(imgui))
    , m_sceneFramebuffer(std::move(sceneFramebuffer))
    , m_resolveFramebuffer(std::move(resolveFramebuffer))
    , m_presentationPass(std::move(presentationPass))
    , m_sceneSamples(sceneSamples)
    , m_lastFrameTime(std::chrono::steady_clock::now())
    , m_lastCameraInteractionTime(m_lastFrameTime) {
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
    for (const auto& cb: m_cursorPosCallbacks) {
        cb(sceneX, sceneY);
    }
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
    for (const auto& cb: m_scrollCallbacks) {
        cb(xoff, yoff);
    }
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
    for (const auto& cb: m_mouseButtonCallbacks) {
        cb(button, action, mods);
    }
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
        // Built-in library shortcut: H triggers the same "go home" behavior as the ImGui Home
        // button, edge-triggered on physical key-press (not polled) so it fires once per press.
        if (key == Key::KEY_H && action == Action::PRESS && !m_imgui->wants_keyboard()) {
            go_to_home_view();
        }
        for (const auto& cb: m_keyCallbacks) {
            cb(key, scancode, action, mods);
        }
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
    return DrawableHandle{DrawableKind::point, *id};
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
    return DrawableHandle{DrawableKind::line, *id};
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
    return DrawableHandle{DrawableKind::mesh, *id};
}

DrawableHandle Renderer::add_mesh_segment_drawable(std::span<const float> positions,
                                                   std::span<const std::uint32_t> indices,
                                                   std::span<const float> color,
                                                   float lineWidth) {
    const auto id = m_drawablesManager->add_mesh_segment_drawable(positions, indices, color, lineWidth);
    if (!id.has_value()) {
        return DrawableHandle{};
    }
    return DrawableHandle{DrawableKind::meshSegment, *id};
}

DrawableHandle
Renderer::add_mesh_vertex_drawable(std::span<const float> positions, std::span<const float> color, float pointSize) {
    const auto id = m_drawablesManager->add_mesh_vertex_drawable(positions, color, pointSize);
    if (!id.has_value()) {
        return DrawableHandle{};
    }
    return DrawableHandle{DrawableKind::meshVertex, *id};
}

void Renderer::set_mesh_drawable_cull_mode(DrawableHandle handle, renderer::MeshCullFaceMode mode) {
    if (handle.kind == DrawableKind::mesh && handle.id != 0U) {
        m_drawablesManager->set_mesh_drawable_cull_mode(handle.id, mode);
    }
}

bool Renderer::remove_drawable(DrawableHandle handle) {
    if (!handle.is_valid()) {
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
    if (!handle.is_valid()) {
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
    if (!handle.is_valid()) {
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
    const auto now = std::chrono::steady_clock::now();
    const double deltaSeconds =
        std::min(maxFrameDeltaSeconds, std::chrono::duration<double>(now - m_lastFrameTime).count());
    m_lastFrameTime = now;

    m_camera->update(deltaSeconds, [this](Key key) {
        return !m_imgui->wants_keyboard() && m_window.is_key_pressed(key);
    });

    // Auto Zoom: once the camera settles after user interaction (no reorientation - just a
    // distance/pan correction along the current gaze), fit current geometry into view. Never
    // fires while actively interacting or mid-transition, and only re-triggers on the next real
    // interaction (does not re-check due to geometry being added/removed while the camera sits
    // still - no drawable-mutation hook exists for that).
    if (m_camera->get_was_blocking()) {
        m_lastCameraInteractionTime = now;
        m_autoFitPending = true;
        m_camera->reset_was_blocking();
    } else if (m_autoFitEnabled && m_autoFitPending && !m_camera->is_transitioning_view() &&
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

    update_scene_viewport();

    const auto [windowFramebufferWidth, windowFramebufferHeight] = m_window.get_framebuffer_size();
    const int framebufferWidth = static_cast<int>(valid_framebuffer_dimension(windowFramebufferWidth));
    const int framebufferHeight = static_cast<int>(valid_framebuffer_dimension(windowFramebufferHeight));
    if (m_sceneFramebuffer->get_width() != framebufferWidth || m_sceneFramebuffer->get_height() != framebufferHeight) {
        auto scene = opengl::Framebuffer::create(
            framebufferWidth, framebufferHeight, m_sceneSamples, m_window.is_srgb_capable());
        std::optional<opengl::Framebuffer> resolve;
        if (scene.has_value() && m_sceneSamples > 1) {
            resolve = opengl::Framebuffer::create(framebufferWidth, framebufferHeight, 1, m_window.is_srgb_capable());
        }
        if (!scene.has_value() || (m_sceneSamples > 1 && !resolve.has_value())) {
            opengl::report_error("Error: Renderer::begin_frame failed to resize scene framebuffer targets");
        } else {
            m_sceneFramebuffer = std::make_unique<opengl::Framebuffer>(std::move(*scene));
            if (m_sceneSamples > 1) {
                m_resolveFramebuffer = std::make_unique<opengl::Framebuffer>(std::move(*resolve));
            }
        }
    }

    m_sceneFramebuffer->bind();
    opengl::begin_frame(clearColor, m_sceneViewport.framebuffer, m_window.is_srgb_capable());
}

void Renderer::draw() {
    const renderer::LightingConfig lighting;
    draw(lighting);
}

void Renderer::draw(const renderer::LightingConfig& lighting) {
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

    // Draw segment overlays on top of meshes.
    if (m_drawablesManager->has_mesh_segment_drawables()) {
        m_drawablesManager->draw_mesh_segment_overlays(m_camera->get_current_MVP());
    }

    // Draw vertex overlays on top of meshes.
    if (m_drawablesManager->has_mesh_vertex_drawables()) {
        m_drawablesManager->draw_mesh_vertex_overlays(m_camera->get_current_MVP());
    }
}

void Renderer::end_frame() {
    bool autoFitEnabled = false;
    bool homeRequested = false;
    end_frame(autoFitEnabled, homeRequested);
}

void Renderer::end_frame(bool& autoFitEnabled) {
    bool homeRequested = false;
    end_frame(autoFitEnabled, homeRequested);
}

void Renderer::end_frame(bool& autoFitEnabled, bool& homeRequested) {
    present_scene();
    m_imgui->new_frame();
    CameraProjectionType projectionType = m_camera->get_projection_type();
    m_imgui->add_camera_controls(autoFitEnabled, projectionType, homeRequested);
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

void Renderer::present_scene() {
    GLuint colorTexture = m_sceneFramebuffer->get_color_texture();
    if (m_sceneSamples > 1) {
        if (!m_sceneFramebuffer->resolve_to(*m_resolveFramebuffer)) {
            opengl::report_error("Error: Renderer::present_scene failed to resolve scene framebuffer");
            return;
        }
        colorTexture = m_resolveFramebuffer->get_color_texture();
    }

    opengl::Framebuffer::unbind();
    const auto [framebufferWidth, framebufferHeight] = m_window.get_framebuffer_size();
    m_presentationPass->present(colorTexture,
                                static_cast<int>(valid_framebuffer_dimension(framebufferWidth)),
                                static_cast<int>(valid_framebuffer_dimension(framebufferHeight)));
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
        // No geometry in the scene: fall back to the existing geometry-agnostic behavior
        // (rotate around the current pivot at the current, clamped-safe distance).
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

void Renderer::add_cursor_pos_callback(CursorPosCB cb) {
    m_cursorPosCallbacks.push_back(std::move(cb));
}
void Renderer::add_scroll_callback(ScrollCB cb) {
    m_scrollCallbacks.push_back(std::move(cb));
}
void Renderer::add_mouse_button_callback(MouseBtnCB cb) {
    m_mouseButtonCallbacks.push_back(std::move(cb));
}
void Renderer::add_key_callback(KeyCB cb) {
    m_keyCallbacks.push_back(std::move(cb));
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
