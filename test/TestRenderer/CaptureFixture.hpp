#pragma once

// Shared test-only support for deterministic offscreen pixel capture (golden-image tests).
// Intentionally NOT part of the production opengl/plinth public API - read-back only ever
// needs to exist for tests, so there is no exported CapturedImage/read-back function for
// downstream consumers to accidentally depend on.
//
// Determinism is structural, not a convention to remember: CaptureWindow hardcodes zero
// MSAA samples and fixed GL 3.3 core context hints with no parameter to override them - a
// test author cannot construct one wrong.

#include <GLFW/glfw3.h>
#include <OpenGL/OpenGL.hpp>
#include <algorithm>
#include <cstdint>
#include <vector>

namespace plinth_test {

struct CapturedImage {
    std::uint32_t width{0};
    std::uint32_t height{0};
    std::vector<std::uint8_t> rgba; // top-down rows (row 0 = top), RGBA8
};

// Reads back width x height pixels from GL_BACK of the current GL context, top-down.
// Must be called after issuing draw calls and before any buffer swap.
inline CapturedImage read_back_rgba(int width, int height) {
    CapturedImage image;
    image.width = static_cast<std::uint32_t>(width);
    image.height = static_cast<std::uint32_t>(height);
    image.rgba.resize(static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4U);

    glReadBuffer(GL_BACK);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);

    std::vector<std::uint8_t> bottomUp(image.rgba.size());
    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, bottomUp.data());

    const std::size_t rowBytes = static_cast<std::size_t>(width) * 4U;
    for (int row = 0; row < height; ++row) {
        const std::size_t srcRow = static_cast<std::size_t>(height - 1 - row) * rowBytes;
        const std::size_t dstRow = static_cast<std::size_t>(row) * rowBytes;
        std::copy_n(bottomUp.data() + srcRow, rowBytes, image.rgba.data() + dstRow);
    }
    return image;
}

namespace detail {
inline void* load_glfw_proc(const char* procName) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    return reinterpret_cast<void*>(glfwGetProcAddress(procName));
}
} // namespace detail

// A hidden, fixed-size, zero-MSAA GLFW window + current GL context, for deterministic
// pixel-exact capture tests. Every determinism-critical GLFW hint is hardcoded here with
// NO constructor parameter to change it - in particular there is no way to ask for MSAA
// samples greater than 0. Only width/height are configurable (capture-region size is not a
// determinism concern; the golden PNG dimensions just need to match).
class CaptureWindow {
  public:
    CaptureWindow(int width, int height) {
        glfwDefaultWindowHints();
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        // Deliberately no GLFW_SAMPLES hint -> 0 samples / MSAA disabled. This is the whole
        // point of this type: there is no setter, parameter, or flag that could turn this on.

        m_window = glfwCreateWindow(width, height, "plinth capture window", nullptr, nullptr);
        if (m_window != nullptr) {
            glfwMakeContextCurrent(m_window);
            m_gladLoaded = gladLoadGLLoader(detail::load_glfw_proc) != 0;
        }
    }

    ~CaptureWindow() {
        if (m_window != nullptr) {
            glfwDestroyWindow(m_window);
        }
    }

    CaptureWindow(const CaptureWindow&) = delete;
    CaptureWindow& operator=(const CaptureWindow&) = delete;
    CaptureWindow(CaptureWindow&&) = delete;
    CaptureWindow& operator=(CaptureWindow&&) = delete;

    [[nodiscard]] bool is_valid() const { return m_window != nullptr && m_gladLoaded; }
    [[nodiscard]] GLFWwindow* handle() const { return m_window; }

  private:
    GLFWwindow* m_window{nullptr};
    bool m_gladLoaded{false};
};

} // namespace plinth_test
