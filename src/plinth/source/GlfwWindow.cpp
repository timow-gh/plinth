#include "InputStateInternal.hpp"
#include <GLFW/glfw3.h>
#include <OpenGL/ErrorReporting.hpp>
#include <glad/glad.h>
#include <plinth/Assert.hpp>
#include <plinth/GlfwWindow.hpp>
#include <print>

namespace renderer {

struct GlfwWindow::Impl {
    GLFWwindow* window{nullptr};
};

namespace {

void* load_glfw_proc(const char* procName) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    return reinterpret_cast<void*>(glfwGetProcAddress(procName));
}

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
bool hasLivePlinthWindow{false};

} // namespace

GlfwWindow::GlfwWindow() = default;
GlfwWindow::GlfwWindow(GlfwWindow&&) noexcept = default;

GlfwWindow& GlfwWindow::operator=(GlfwWindow&& other) noexcept {
    if (this != &other) {
        if (m_impl && m_impl->window != nullptr) {
            detail::clear_callbacks(m_impl->window);
            glfwDestroyWindow(m_impl->window);
            glfwTerminate();
            hasLivePlinthWindow = false;
        }
        m_impl = std::move(other.m_impl);
    }
    return *this;
}

GlfwWindow::~GlfwWindow() {
    if (m_impl && m_impl->window != nullptr) {
        detail::clear_callbacks(m_impl->window);
        glfwDestroyWindow(m_impl->window);
        glfwTerminate();
        hasLivePlinthWindow = false;
    }
}

bool GlfwWindow::is_initialized() const {
    return m_impl && m_impl->window != nullptr;
}

void* GlfwWindow::get_native_handle() const {
    if (!m_impl) {
        return nullptr;
    }
    return static_cast<void*>(m_impl->window);
}

std::optional<GlfwWindow> GlfwWindow::create(const WindowSettings& settings) {
    if (hasLivePlinthWindow) {
        std::print("Error: A plinth GlfwWindow is already active; only one live window is supported.\n");
        return std::nullopt;
    }

    if (glfwInit() == 0) {
        const char* description = nullptr;
        glfwGetError(&description);
        std::print("Error: {}\n", description);
        return std::nullopt;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, settings.debug_context ? 3 : 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    if (settings.debug_context) {
        glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
    }
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
    window.m_impl = std::make_unique<Impl>();
    window.m_impl->window = glfwWindow;

    window.make_context_current();
    if (gladLoadGLLoader(load_glfw_proc) == 0) {
        std::print("Failed to initialize OpenGL context\n");
        return std::nullopt;
    }

    if (settings.debug_context) {
        if (!opengl::install_debug_callback()) {
            opengl::report_warning(
                "debug_context requested but GL_VERSION_4_3 (or higher) is not available on this "
                "context; debug messages will not be reported via glDebugMessageCallback. "
                "glGetError() checks remain active regardless.");
        }
    }

    hasLivePlinthWindow = true;
    return window;
}

void GlfwWindow::make_context_current() const {
    RENDERER_ASSERT(m_impl && m_impl->window);
    glfwMakeContextCurrent(m_impl->window);
}

void GlfwWindow::poll_events() {
    glfwPollEvents();
}

void GlfwWindow::swap_buffers() const {
    RENDERER_ASSERT(m_impl && m_impl->window);
    glfwSwapBuffers(m_impl->window);
}

bool GlfwWindow::should_close() const {
    RENDERER_ASSERT(m_impl && m_impl->window);
    return glfwWindowShouldClose(m_impl->window) != 0;
}

bool GlfwWindow::is_escape_pressed() const {
    RENDERER_ASSERT(m_impl && m_impl->window);
    return glfwGetKey(m_impl->window, GLFW_KEY_ESCAPE) == GLFW_PRESS;
}

bool GlfwWindow::is_key_pressed(Key key) const {
    RENDERER_ASSERT(m_impl && m_impl->window);
    return glfwGetKey(m_impl->window, static_cast<int>(key)) == GLFW_PRESS;
}

std::pair<int, int> GlfwWindow::get_window_size() const {
    RENDERER_ASSERT(m_impl && m_impl->window);

    int windowWidth{0};
    int windowHeight{0};
    glfwGetWindowSize(m_impl->window, &windowWidth, &windowHeight);

    return {windowWidth, windowHeight};
}

std::pair<int, int> GlfwWindow::get_framebuffer_size() const {
    RENDERER_ASSERT(m_impl && m_impl->window);

    int framebufferWidth{0};
    int framebufferHeight{0};
    glfwGetFramebufferSize(m_impl->window, &framebufferWidth, &framebufferHeight);

    return {framebufferWidth, framebufferHeight};
}

std::pair<double, double> GlfwWindow::get_framebuffer_scale() const {
    const auto [windowWidth, windowHeight] = get_window_size();
    const auto [framebufferWidth, framebufferHeight] = get_framebuffer_size();

    const double xScale = windowWidth > 0 ? static_cast<double>(framebufferWidth) / windowWidth : 1.0;
    const double yScale = windowHeight > 0 ? static_cast<double>(framebufferHeight) / windowHeight : 1.0;
    return {xScale, yScale};
}

bool GlfwWindow::is_srgb_capable() const {
    RENDERER_ASSERT(m_impl && m_impl->window);
    return glfwGetWindowAttrib(m_impl->window, GLFW_SRGB_CAPABLE) == GLFW_TRUE;
}

InputState& GlfwWindow::get_input_state() const {
    RENDERER_ASSERT(m_impl && m_impl->window);
    return *detail::get_input_state(m_impl->window);
}

void GlfwWindow::set_key_callback(KeyCB cb) {
    RENDERER_ASSERT(m_impl && m_impl->window);
    detail::set_key_callback(m_impl->window, std::move(cb));
}

void GlfwWindow::set_char_callback(CharCB cb) {
    RENDERER_ASSERT(m_impl && m_impl->window);
    detail::set_char_callback(m_impl->window, std::move(cb));
}

void GlfwWindow::set_cursor_pos_callback(CursorPosCB cb) {
    RENDERER_ASSERT(m_impl && m_impl->window);
    detail::set_cursor_pos_callback(m_impl->window, std::move(cb));
}

void GlfwWindow::set_scroll_callback(ScrollCB cb) {
    RENDERER_ASSERT(m_impl && m_impl->window);
    detail::set_scroll_callback(m_impl->window, std::move(cb));
}

void GlfwWindow::set_mouse_button_callback(MouseBtnCB cb) {
    RENDERER_ASSERT(m_impl && m_impl->window);
    detail::set_mouse_button_callback(m_impl->window, std::move(cb));
}

void GlfwWindow::set_framebuffer_size_callback(FramebufferSizeCB cb) {
    RENDERER_ASSERT(m_impl && m_impl->window);
    detail::set_framebuffer_size_callback(m_impl->window, std::move(cb));
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
