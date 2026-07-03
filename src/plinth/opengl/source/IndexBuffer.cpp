#include "OpenGL/IndexBuffer.hpp"
#include <Renderer/Assert.hpp>
#include <cstdint>
#include <utility>

namespace opengl {

IndexBuffer::IndexBuffer(IndexBuffer&& other) noexcept
    : m_id(std::exchange(other.m_id, std::nullopt))
    , m_indexCount(std::exchange(other.m_indexCount, 0)) {
}

IndexBuffer& IndexBuffer::operator=(IndexBuffer&& other) noexcept {
    if (this != &other) {
        reset();
        m_id = std::exchange(other.m_id, std::nullopt);
        m_indexCount = std::exchange(other.m_indexCount, 0);
    }
    return *this;
}

IndexBuffer::~IndexBuffer() {
    reset();
}

void IndexBuffer::reset() noexcept {
    if (m_id.has_value()) {
        const auto id = m_id->get_value();
        glDeleteBuffers(1, &id);
        m_id = std::nullopt;
    }
    m_indexCount = 0;
}

std::optional<IndexBuffer> IndexBuffer::create(std::span<const std::uint32_t> indices,
                                               BufferAccessPattern accessPattern) {
    BufferId id;
    glGenBuffers(1, &id.get_value());
    if (id.get_value() == 0) {
        RENDERER_ASSERT(false);
        return std::nullopt;
    }

    IndexBuffer buffer{id, static_cast<GLsizei>(indices.size())};
    buffer.bind();
    const auto size = static_cast<GLsizeiptr>(indices.size() * sizeof(std::uint32_t));
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, indices.data(), get_enum_value(accessPattern));

    return std::optional<IndexBuffer>{std::move(buffer)};
}

const BufferId& IndexBuffer::get_buffer_id() const {
    RENDERER_ASSERT(m_id.has_value());
    return m_id.value();
}

void IndexBuffer::bind() const {
    RENDERER_ASSERT(m_id.has_value());
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_id.value().get_value());
}

void IndexBuffer::unbind() {
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void IndexBuffer::update_indices_buffer(std::span<const std::uint32_t> indices, BufferAccessPattern accessPattern) {
    bind();
    const auto size = static_cast<GLsizeiptr>(indices.size() * sizeof(std::uint32_t));
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, nullptr, get_enum_value(accessPattern));
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, indices.data(), get_enum_value(accessPattern));
    set_index_count(static_cast<GLsizei>(indices.size()));
}

IndexBuffer::IndexBuffer(BufferId id, GLsizei indexCount)
    : m_id(id)
    , m_indexCount(indexCount) {
}

} // namespace opengl
