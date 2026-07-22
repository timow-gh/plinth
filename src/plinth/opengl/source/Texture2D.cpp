#include <OpenGL/ErrorReporting.hpp>
#include <OpenGL/Texture2D.hpp>
#include <cstddef>
#include <limits>
#include <utility>

namespace opengl {

Texture2D::Texture2D(Texture2D&& other) noexcept
    : m_id(std::exchange(other.m_id, 0)), m_width(std::exchange(other.m_width, 0)), m_height(std::exchange(other.m_height, 0)) {}

Texture2D& Texture2D::operator=(Texture2D&& other) noexcept {
    if (this != &other) {
        reset();
        m_id = std::exchange(other.m_id, 0);
        m_width = std::exchange(other.m_width, 0);
        m_height = std::exchange(other.m_height, 0);
    }
    return *this;
}

Texture2D::~Texture2D() { reset(); }

std::optional<Texture2D> Texture2D::create(const renderer::TextureData& data, int maxTextureSize, int maxAnisotropy) {
    if (maxTextureSize < 1 || data.width == 0 || data.height == 0 || data.width > static_cast<std::uint32_t>(maxTextureSize) ||
        data.height > static_cast<std::uint32_t>(maxTextureSize) ||
        data.width > std::numeric_limits<std::size_t>::max() / data.height / 4U ||
        data.rgba8.size() != static_cast<std::size_t>(data.width) * data.height * 4U) {
        report_error("Texture2D::create: invalid dimensions or RGBA8 byte count");
        return std::nullopt;
    }
    GLuint id = 0;
    glGenTextures(1, &id);
    if (id == 0) {
        report_error("Texture2D::create: glGenTextures failed");
        return std::nullopt;
    }
    glBindTexture(GL_TEXTURE_2D, id);
    const GLenum format = data.colorSpace == renderer::TextureColorSpace::srgb ? GL_SRGB8_ALPHA8 : GL_RGBA8;
    glTexImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(format), static_cast<GLsizei>(data.width), static_cast<GLsizei>(data.height), 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, data.rgba8.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    const GLint magnificationFilter =
        data.magnificationFilter == renderer::TextureFilter::nearest ? GL_NEAREST : GL_LINEAR;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magnificationFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    if (maxAnisotropy > 1) {
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY, static_cast<float>(maxAnisotropy));
    }
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);
    if (check_gl_errors("Texture2D::create")) {
        glDeleteTextures(1, &id);
        return std::nullopt;
    }
    return Texture2D{id, static_cast<int>(data.width), static_cast<int>(data.height)};
}

void Texture2D::reset() noexcept {
    if (m_id != 0) {
        glDeleteTextures(1, &m_id);
    }
    m_id = 0;
    m_width = 0;
    m_height = 0;
}
void Texture2D::bind(GLuint unit) const { glActiveTexture(GL_TEXTURE0 + unit); glBindTexture(GL_TEXTURE_2D, m_id); }
bool Texture2D::is_valid() const noexcept { return m_id != 0; }
GLuint Texture2D::get_id() const noexcept { return m_id; }
int Texture2D::get_width() const noexcept { return m_width; }
int Texture2D::get_height() const noexcept { return m_height; }

} // namespace opengl
