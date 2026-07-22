#ifndef OPENGL_TEXTURE2D_HPP
#define OPENGL_TEXTURE2D_HPP

#include <OpenGL/OpenGL.hpp>
#include <OpenGL/opengl_export.h>
#include <plinth/Texture.hpp>
#include <optional>

namespace opengl {

class OPENGL_EXPORT Texture2D {
  public:
    Texture2D(const Texture2D&) = delete;
    Texture2D& operator=(const Texture2D&) = delete;
    Texture2D(Texture2D&& other) noexcept;
    Texture2D& operator=(Texture2D&& other) noexcept;
    ~Texture2D();

    [[nodiscard]] static std::optional<Texture2D> create(const renderer::TextureData& data, int maxTextureSize, int maxAnisotropy = 1);
    void reset() noexcept;
    void bind(GLuint unit) const;
    [[nodiscard]] bool is_valid() const noexcept;
    [[nodiscard]] GLuint get_id() const noexcept;
    [[nodiscard]] int get_width() const noexcept;
    [[nodiscard]] int get_height() const noexcept;

  private:
    Texture2D(GLuint id, int width, int height) : m_id(id), m_width(width), m_height(height) {}
    GLuint m_id{0};
    int m_width{0};
    int m_height{0};
};

} // namespace opengl

#endif // OPENGL_TEXTURE2D_HPP
