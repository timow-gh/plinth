#include "OpenGL/VertexBuffer.hpp"
#include "OpenGL/UpdateBuffer.hpp"
#include <plinth/Assert.hpp>
#include <limits>
#include <print>
#include <utility>

namespace opengl {

VertexBuffer::VertexBuffer(VertexBuffer&& other) noexcept
    : m_bufferId{std::exchange(other.m_bufferId, std::nullopt)}
    , m_vectorDimension{std::exchange(other.m_vectorDimension, 0)}
    , m_stride{std::exchange(other.m_stride, 0)}
    , m_bufferLocation{other.m_bufferLocation} {
}

VertexBuffer& VertexBuffer::operator=(VertexBuffer&& other) noexcept {
    if (this != &other) {
        reset();
        m_bufferId = std::exchange(other.m_bufferId, std::nullopt);
        m_vectorDimension = std::exchange(other.m_vectorDimension, 0);
        m_stride = std::exchange(other.m_stride, 0);
        m_bufferLocation = other.m_bufferLocation;
    }
    return *this;
}

VertexBuffer::~VertexBuffer() {
    reset();
}

void VertexBuffer::reset() noexcept {
    if (m_bufferId.has_value()) {
        const auto id = m_bufferId->get_value();
        glDeleteBuffers(1, &id);
        m_bufferId = std::nullopt;
    }
    m_vectorDimension = 0;
    m_stride = 0;
}

std::optional<VertexBuffer> VertexBuffer::create(std::span<const float> vectors,
                                                 GLsizei vectorDimension,
                                                 Location bufferLocation,
                                                 BufferAccessPattern accessPattern) {
    GLuint bufferId{0};
    glGenBuffers(1, &bufferId);
    if (bufferId == 0) {
        std::print(stderr, "Error: glGenBuffers failed to allocate a vertex buffer\n");
        RENDERER_ASSERT(false);
        return std::nullopt;
    }

    glBindBuffer(GL_ARRAY_BUFFER, bufferId);
    const auto size = vectors.size() * sizeof(float);
    RENDERER_ASSERT(size <= static_cast<std::size_t>(std::numeric_limits<GLsizeiptr>::max()));
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(size), vectors.data(), get_enum_value(accessPattern));
    glEnableVertexAttribArray(bufferLocation.get_as_unsigned());
    const GLsizei stride = vectorDimension * static_cast<GLsizei>(sizeof(float));
    glVertexAttribPointer(bufferLocation.get_as_unsigned(), vectorDimension, GL_FLOAT, GL_FALSE, stride, nullptr);

    return VertexBuffer{BufferId{bufferId}, vectorDimension, stride, bufferLocation};
}

const BufferId& VertexBuffer::get_buffer_id() const {
    RENDERER_ASSERT(m_bufferId.has_value());
    return m_bufferId.value();
}

void VertexBuffer::bind() const {
    RENDERER_ASSERT(m_bufferId.has_value());
    glBindBuffer(GL_ARRAY_BUFFER, m_bufferId.value().get_value());
    glEnableVertexAttribArray(m_bufferLocation.get_as_unsigned());
    glVertexAttribPointer(m_bufferLocation.get_as_unsigned(), m_vectorDimension, GL_FLOAT, GL_FALSE, m_stride, nullptr);
}

void VertexBuffer::unbind() {
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void VertexBuffer::update_buffer(std::span<const float> vertices, BufferAccessPattern accessPattern) {
    ::opengl::update_buffer(*this, vertices, accessPattern);
}

VertexBuffer::VertexBuffer(BufferId bufferId, GLsizei vectorDimension, GLsizei stride, Location bufferLocation)
    : m_bufferId{bufferId}
    , m_vectorDimension{vectorDimension}
    , m_stride{stride}
    , m_bufferLocation{bufferLocation} {
}

} // namespace opengl
