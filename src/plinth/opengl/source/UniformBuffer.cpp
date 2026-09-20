#include "OpenGL/UniformBuffer.hpp"

#include "OpenGL/ErrorReporting.hpp"
#include "plinth/Assert.hpp"

#include <limits>
#include <utility>

namespace opengl {

UniformBuffer::UniformBuffer(UniformBuffer&& other) noexcept
    : m_bufferId{std::exchange(other.m_bufferId, std::nullopt)}
    , m_bindingPoint{std::exchange(other.m_bindingPoint, 0)} {
}

UniformBuffer& UniformBuffer::operator=(UniformBuffer&& other) noexcept {
    if (this != &other) {
        reset();
        m_bufferId = std::exchange(other.m_bufferId, std::nullopt);
        m_bindingPoint = std::exchange(other.m_bindingPoint, 0);
    }
    return *this;
}

UniformBuffer::~UniformBuffer() {
    reset();
}

void UniformBuffer::reset() noexcept {
    if (m_bufferId.has_value()) {
        const GLuint id = m_bufferId->get_value();
        glDeleteBuffers(1, &id);
        m_bufferId = std::nullopt;
    }
    m_bindingPoint = 0;
}

std::optional<UniformBuffer> UniformBuffer::create(std::span<const float> data, GLuint bindingPoint) {
    GLuint bufferId{0};
    glGenBuffers(1, &bufferId);
    if (bufferId == 0) {
        report_error("Error: glGenBuffers failed to allocate a uniform buffer");
        return std::nullopt;
    }

    glBindBuffer(GL_UNIFORM_BUFFER, bufferId);
    const auto size = data.size() * sizeof(float);
    RENDERER_ASSERT(size <= static_cast<std::size_t>(std::numeric_limits<GLsizeiptr>::max()));
    glBufferData(GL_UNIFORM_BUFFER, static_cast<GLsizeiptr>(size), data.data(), GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, bindingPoint, bufferId);

    return UniformBuffer{BufferId{bufferId}, bindingPoint};
}

const BufferId& UniformBuffer::get_buffer_id() const {
    RENDERER_ASSERT(m_bufferId.has_value());
    return m_bufferId.value();
}

void UniformBuffer::bind() const {
    RENDERER_ASSERT(m_bufferId.has_value());
    glBindBufferBase(GL_UNIFORM_BUFFER, m_bindingPoint, m_bufferId->get_value());
}

void UniformBuffer::update(std::span<const float> data) {
    RENDERER_ASSERT(m_bufferId.has_value());
    glBindBuffer(GL_UNIFORM_BUFFER, m_bufferId->get_value());
    const auto size = data.size() * sizeof(float);
    RENDERER_ASSERT(size <= static_cast<std::size_t>(std::numeric_limits<GLsizeiptr>::max()));
    const auto bufferSize = static_cast<GLsizeiptr>(size);
    // Orphan then refill, matching opengl::update_buffer's strategy.
    glBufferData(GL_UNIFORM_BUFFER, bufferSize, nullptr, GL_DYNAMIC_DRAW);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, bufferSize, data.data());
    glBindBufferBase(GL_UNIFORM_BUFFER, m_bindingPoint, m_bufferId->get_value());
}

UniformBuffer::UniformBuffer(BufferId bufferId, GLuint bindingPoint)
    : m_bufferId{bufferId}
    , m_bindingPoint{bindingPoint} {
}

} // namespace opengl
