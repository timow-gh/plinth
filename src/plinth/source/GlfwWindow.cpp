#include <plinth/Assert.hpp>
#include <plinth/GlfwWindow.hpp>
#include <print>

namespace renderer {

namespace {

void* load_glfw_proc(const char* procName) {
    // GLAD v1 expects void* while GLFW returns an opaque function pointer.
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    return reinterpret_cast<void*>(glfwGetProcAddress(procName));
}

} // namespace

GlfwWindow::~GlfwWindow() {
    if (m_glfwWindow == nullptr) {
        return;
    }

    clear_callbacks(m_glfwWindow);
    glfwDestroyWindow(m_glfwWindow);
    m_glfwWindow = nullptr;
    glfwTerminate();
}

std::optional<GlfwWindow> GlfwWindow::create(const WindowSettings& settings) {
    if (glfwInit() == 0) {
        const char* description = nullptr;
        glfwGetError(&description);
        std::print("Error: {}\n", description);
        return std::nullopt;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    set_window_hints(settings);

    GLFWwindow* glfwWindow = glfwCreateWindow(static_cast<int>(settings.width),
                                              static_cast<int>(settings.height),
                                              settings.title,
                                              nullptr,
                                              nullptr);

    if (glfwWindow == nullptr) {
        const char* description = nullptr;
        glfwGetError(&description);
        std::print("Error: {}\n", description);
        glfwTerminate();
        return std::nullopt;
    }

    GlfwWindow window;
    window.m_glfwWindow = glfwWindow;

    window.make_context_current();
    if (gladLoadGLLoader(load_glfw_proc) == 0) {
        std::print("Failed to initialize OpenGL context\n");
        return std::nullopt;
    }

    return window;
}

void GlfwWindow::make_context_current() const {
    RENDERER_ASSERT(m_glfwWindow);
    glfwMakeContextCurrent(m_glfwWindow);
}

void GlfwWindow::poll_events() {
    glfwPollEvents();
}

void GlfwWindow::swap_buffers() const {
    RENDERER_ASSERT(m_glfwWindow);
    glfwSwapBuffers(m_glfwWindow);
}

bool GlfwWindow::should_close() const {
    RENDERER_ASSERT(m_glfwWindow);
    return glfwWindowShouldClose(m_glfwWindow) != 0;
}

bool GlfwWindow::is_escape_pressed() const {
    RENDERER_ASSERT(m_glfwWindow);
    return glfwGetKey(m_glfwWindow, GLFW_KEY_ESCAPE) == GLFW_PRESS;
}

std::pair<int, int> GlfwWindow::get_window_size() const {
    RENDERER_ASSERT(m_glfwWindow);

    int windowWidth{0};
    int windowHeight{0};
    glfwGetWindowSize(m_glfwWindow, &windowWidth, &windowHeight);

    return {windowWidth, windowHeight};
}

std::pair<int, int> GlfwWindow::get_framebuffer_size() const {
    RENDERER_ASSERT(m_glfwWindow);

    int framebufferWidth{0};
    int framebufferHeight{0};
    glfwGetFramebufferSize(m_glfwWindow, &framebufferWidth, &framebufferHeight);

    return {framebufferWidth, framebufferHeight};
}

std::pair<double, double> GlfwWindow::get_framebuffer_scale() const {
    const auto [windowWidth, windowHeight] = get_window_size();
    const auto [framebufferWidth, framebufferHeight] = get_framebuffer_size();

    const double xScale = windowWidth > 0 ? static_cast<double>(framebufferWidth) / windowWidth : 1.0;
    const double yScale = windowHeight > 0 ? static_cast<double>(framebufferHeight) / windowHeight : 1.0;
    return {xScale, yScale};
}

InputState& GlfwWindow::get_input_state() const {
    RENDERER_ASSERT(m_glfwWindow);
    return *renderer::get_input_state(m_glfwWindow);
}

void GlfwWindow::set_key_callback(KeyCB cb) {
    RENDERER_ASSERT(m_glfwWindow);
    renderer::set_key_callback(m_glfwWindow, std::move(cb));
}

void GlfwWindow::set_char_callback(CharCB cb) {
    RENDERER_ASSERT(m_glfwWindow);
    renderer::set_char_callback(m_glfwWindow, std::move(cb));
}

void GlfwWindow::set_cursor_pos_callback(CursorPosCB cb) {
    RENDERER_ASSERT(m_glfwWindow);
    renderer::set_cursor_pos_callback(m_glfwWindow, std::move(cb));
}

void GlfwWindow::set_scroll_callback(ScrollCB cb) {
    RENDERER_ASSERT(m_glfwWindow);
    renderer::set_scroll_callback(m_glfwWindow, std::move(cb));
}

void GlfwWindow::set_mouse_button_callback(MouseBtnCB cb) {
    RENDERER_ASSERT(m_glfwWindow);
    renderer::set_mouse_button_callback(m_glfwWindow, std::move(cb));
}

void GlfwWindow::set_framebuffer_size_callback(FramebufferSizeCB cb) {
    RENDERER_ASSERT(m_glfwWindow);
    renderer::set_framebuffer_size_callback(m_glfwWindow, std::move(cb));
}

void GlfwWindow::set_window_hints(const WindowSettings& hints) {
    glfwWindowHint(GLFW_RED_BITS, hints.red_bits);
    glfwWindowHint(GLFW_GREEN_BITS, hints.green_bits);
    glfwWindowHint(GLFW_BLUE_BITS, hints.blue_bits);
    glfwWindowHint(GLFW_ALPHA_BITS, hints.alpha_bits);
    glfwWindowHint(GLFW_DEPTH_BITS, hints.depth_bits);
    glfwWindowHint(GLFW_STENCIL_BITS, hints.stencil_bits);
    glfwWindowHint(GLFW_ACCUM_RED_BITS, hints.accum_red_bits);
    glfwWindowHint(GLFW_ACCUM_GREEN_BITS, hints.accum_green_bits);
    glfwWindowHint(GLFW_ACCUM_BLUE_BITS, hints.accum_blue_bits);
    glfwWindowHint(GLFW_ACCUM_ALPHA_BITS, hints.accum_alpha_bits);
    glfwWindowHint(GLFW_AUX_BUFFERS, hints.aux_buffers);
    glfwWindowHint(GLFW_SAMPLES, hints.samples);
    glfwWindowHint(GLFW_REFRESH_RATE, hints.refresh_rate);
    glfwWindowHint(GLFW_STEREO, window_hint_to_glfw_hint(hints.stereo));
    glfwWindowHint(GLFW_SRGB_CAPABLE, window_hint_to_glfw_hint(hints.srgb_capable));
    glfwWindowHint(GLFW_DOUBLEBUFFER, window_hint_to_glfw_hint(hints.double_buffer));
    glfwWindowHint(GLFW_RESIZABLE, window_hint_to_glfw_hint(hints.resizable));
    glfwWindowHint(GLFW_VISIBLE, window_hint_to_glfw_hint(hints.visible));
    glfwWindowHint(GLFW_DECORATED, window_hint_to_glfw_hint(hints.decorated));
    glfwWindowHint(GLFW_FOCUSED, window_hint_to_glfw_hint(hints.focused));
    glfwWindowHint(GLFW_AUTO_ICONIFY, window_hint_to_glfw_hint(hints.auto_iconify));
    glfwWindowHint(GLFW_FLOATING, window_hint_to_glfw_hint(hints.floating));
    glfwWindowHint(GLFW_MAXIMIZED, window_hint_to_glfw_hint(hints.maximized));
    glfwWindowHint(GLFW_CENTER_CURSOR, window_hint_to_glfw_hint(hints.center_cursor));
    glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, window_hint_to_glfw_hint(hints.transparent_framebuffer));
    glfwWindowHint(GLFW_FOCUS_ON_SHOW, window_hint_to_glfw_hint(hints.focus_on_show));
    glfwWindowHint(GLFW_SCALE_TO_MONITOR, window_hint_to_glfw_hint(hints.scale_to_monitor));
}

} // namespace renderer
