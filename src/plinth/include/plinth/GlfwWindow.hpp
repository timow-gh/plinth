#ifndef RENDERER_GLFWWINDOW_HPP
#define RENDERER_GLFWWINDOW_HPP

#include <plinth/InputState.hpp>
#include <plinth/WindowSettings.hpp>
#include <memory>
#include <optional>

namespace renderer {

class GlfwWindow {
  public:
    GlfwWindow();
    GlfwWindow(const GlfwWindow&) = delete;
    GlfwWindow& operator=(const GlfwWindow&) = delete;
    GlfwWindow(GlfwWindow&&) noexcept;
    GlfwWindow& operator=(GlfwWindow&&) noexcept = delete;
    ~GlfwWindow();

    /// Only one live plinth GlfwWindow may exist at a time. A second call to
    /// create() while another GlfwWindow is alive returns std::nullopt. A new
    /// instance may be created after the previous owner is destroyed.
    [[nodiscard]]
    static std::optional<GlfwWindow> create(const WindowSettings& settings);

    [[nodiscard]]
    bool is_initialized() const;
    [[nodiscard]]
    /// Returns the underlying GLFWwindow pointer. Must be called on the creating thread.
    void* get_native_handle() const;

    /// GLFW and OpenGL operations must occur on the creating thread. This makes
    /// this window's context current on that thread.
    void make_context_current() const;
    /// Dispatch pending GLFW events and invoke installed callbacks.
    static void poll_events();
    /// Swaps the front and back buffers. Must be called on the creating thread.
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
    /// Reports whether the default framebuffer is sRGB-capable. Must be called
    /// on the creating thread with this window's context current and the
    /// default framebuffer bound.
    [[nodiscard]]
    bool is_srgb_capable() const;

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

    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace renderer

#endif // RENDERER_GLFWWINDOW_HPP
