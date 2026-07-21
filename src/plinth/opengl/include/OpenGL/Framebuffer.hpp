#ifndef OPENGL_FRAMEBUFFER_HPP
#define OPENGL_FRAMEBUFFER_HPP

#include <OpenGL/OpenGL.hpp>
#include <OpenGL/opengl_export.h>
#include <optional>

namespace opengl {

class OPENGL_EXPORT Framebuffer {
  public:
    struct HdrConfig {
        int width{0};
        int height{0};
        int samples{1};
        bool useDepthTexture{false};
    };

    Framebuffer(const Framebuffer&) = delete;
    Framebuffer& operator=(const Framebuffer&) = delete;
    Framebuffer(Framebuffer&& other) noexcept;
    Framebuffer& operator=(Framebuffer&& other) noexcept;

    ~Framebuffer();

    [[nodiscard]]
    static std::optional<Framebuffer> create(int width, int height, int samples, bool srgb);

    [[nodiscard]]
    static std::optional<Framebuffer> create_hdr(const HdrConfig& config);

    [[nodiscard]]
    static std::optional<Framebuffer> create_ldr_intermediate(int width, int height);

    void reset() noexcept;

    [[nodiscard]]
    bool resize(int width, int height);

    void bind() const;
    static void unbind();

    [[nodiscard]]
    bool resolve_to(Framebuffer& destination, GLbitfield mask = GL_COLOR_BUFFER_BIT) const;

    [[nodiscard]]
    bool is_valid() const noexcept;
    [[nodiscard]]
    GLuint get_id() const noexcept;
    [[nodiscard]]
    GLuint get_color_texture() const noexcept;
    [[nodiscard]]
    GLuint get_depth_texture() const noexcept;
    [[nodiscard]]
    int get_width() const noexcept;
    [[nodiscard]]
    int get_height() const noexcept;
    [[nodiscard]]
    int get_samples() const noexcept;

  private:
    Framebuffer(GLuint framebuffer, GLuint colorTexture, GLuint colorRenderbuffer,
                GLuint depthStencilRenderbuffer, GLuint depthTexture, bool hasDepthTexture,
                int width, int height, int samples, bool srgb);

    GLuint m_framebuffer{0};
    GLuint m_colorTexture{0};
    GLuint m_colorRenderbuffer{0};
    GLuint m_depthStencilRenderbuffer{0};
    GLuint m_depthTexture{0};
    bool m_hasDepthTexture{false};
    int m_width{0};
    int m_height{0};
    int m_samples{0};
    bool m_srgb{false};
};

} // namespace opengl

#endif // OPENGL_FRAMEBUFFER_HPP
