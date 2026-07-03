#include "OpenGL/VertexArray.hpp"
#include <plinth/Assert.hpp>
#include <utility>

namespace opengl {

VertexArray::VertexArray(GLuint id)
    : m_id(id) {
    RENDERER_ASSERT(id != 0);
}

VertexArray::VertexArray(VertexArray&& other) noexcept
    : m_id(std::exchange(other.m_id, std::nullopt)) {
}

VertexArray& VertexArray::operator=(VertexArray&& other) noexcept {
    if (this != &other) {
        reset();
        m_id = std::exchange(other.m_id, std::nullopt);
    }
    return *this;
}

VertexArray::~VertexArray() {
    reset();
}

void VertexArray::reset() noexcept {
    if (m_id.has_value()) {
        glDeleteVertexArrays(1, &m_id.value());
        m_id = std::nullopt;
    }
}

std::optional<VertexArray> VertexArray::create() {
    GLuint id{0};
    glGenVertexArrays(1, &id);
    if (id == 0) {
        RENDERER_ASSERT(false);
        return std::nullopt;
    }

    VertexArray array{id};
    array.bind();
    return std::optional<VertexArray>{std::move(array)};
}

void VertexArray::bind() const {
    RENDERER_ASSERT(m_id.has_value());
    glBindVertexArray(m_id.value());
}

void VertexArray::unbind() {
    glBindVertexArray(0);
}

} // namespace opengl
