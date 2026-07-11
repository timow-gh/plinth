#ifndef RENDERER_GLFWWINDOW_HPP
#define RENDERER_GLFWWINDOW_HPP

#include <OpenGL/GpuCapabilities.hpp>
#include <plinth/InputState.hpp>
#include <plinth/WindowSettings.hpp>
#include <optional>
#include <utility>

namespace renderer {

class GlfwWindow {
    GLFWwindow* m_glfwWindow{nullptr};
    opengl::GpuCapabilities m_capabilities;

  public:
    GlfwWindow() = default;
    GlfwWindow(const GlfwWindow&) = delete;
    GlfwWindow& operator=(const GlfwWindow&) = delete;
    GlfwWindow(GlfwWindow&& other) noexcept {
        m_glfwWindow = other.m_glfwWindow;
        other.m_glfwWindow = nullptr;
        m_capabilities = std::move(other.m_capabilities);
    }
    GlfwWindow& operator=(GlfwWindow&& other) noexcept {
        if (this != &other) {
            m_glfwWindow = other.m_glfwWindow;
            other.m_glfwWindow = nullptr;
            m_capabilities = std::move(other.m_capabilities);
        }
        return *this;
    }
    ~GlfwWindow();

    [[nodiscard]]
    static std::optional<GlfwWindow> create(const WindowSettings& settings);

    [[nodiscard]]
    bool is_initialized() const {
        return m_glfwWindow != nullptr;
    }
    [[nodiscard]]
    GLFWwindow* get_native_handle() const {
        return m_glfwWindow;
    }
    [[nodiscard]]
    const opengl::GpuCapabilities& capabilities() const {
        return m_capabilities;
    }

    void make_context_current() const;
    static void poll_events();
    void swap_buffers() const;

    [[nodiscard]]
    bool should_close() const;
    [[nodiscard]]
    bool is_escape_pressed() const;
    [[nodiscard]]
    bool is_key_pressed(Key key) const;
    [[nodiscard]]
    std::pair<int, int> get_window_size() const;
    [[nodiscard]]
    std::pair<int, int> get_framebuffer_size() const;
    [[nodiscard]]
    std::pair<double, double> get_framebuffer_scale() const;

    [[nodiscard]]
    InputState& get_input_state() const;

    void set_key_callback(KeyCB cb);
    void set_char_callback(CharCB cb);
    void set_cursor_pos_callback(CursorPosCB cb);
    void set_scroll_callback(ScrollCB cb);
    void set_mouse_button_callback(MouseBtnCB cb);
    void set_framebuffer_size_callback(FramebufferSizeCB cb);

  private:
    [[nodiscard]]
    static int window_hint_to_glfw_hint(bool hint) {
        return hint ? 1 : 0;
    }
    static void set_window_hints(const WindowSettings& hints);
};

} // namespace renderer

#endif // RENDERER_GLFWWINDOW_HPP
