#include "OpenGL/InstanceBuffer.hpp"

#include "OpenGL/ErrorReporting.hpp"
#include "plinth/Assert.hpp"

#include <cstdint>
#include <limits>
#include <utility>

namespace opengl {

namespace {

GLsizei compute_instance_count(std::size_t floatCount, GLsizei strideBytes) {
    if (strideBytes <= 0) {
        return 0;
    }
    const auto floatsPerInstance = static_cast<std::size_t>(strideBytes) / sizeof(float);
    if (floatsPerInstance == 0) {
        return 0;
    }
    return static_cast<GLsizei>(floatCount / floatsPerInstance);
}

void apply_attribs(std::span<const InstanceAttribSpec> attribs, GLsizei strideBytes) {
    for (const auto& attrib: attribs) {
        glEnableVertexAttribArray(attrib.location.get_as_unsigned());
        glVertexAttribPointer(attrib.location.get_as_unsigned(),
                              attrib.componentCount,
                              GL_FLOAT,
                              GL_FALSE,
                              strideBytes,
                              reinterpret_cast<const void*>(static_cast<std::uintptr_t>(attrib.offsetBytes)));
        // Advance this attribute once per instance rather than once per vertex.
        glVertexAttribDivisor(attrib.location.get_as_unsigned(), 1);
    }
}

} // namespace

InstanceBuffer::InstanceBuffer(InstanceBuffer&& other) noexcept
    : m_bufferId{std::exchange(other.m_bufferId, std::nullopt)}
    , m_strideBytes{std::exchange(other.m_strideBytes, 0)}
    , m_instanceCount{std::exchange(other.m_instanceCount, 0)}
    , m_attribs{std::move(other.m_attribs)} {
    other.m_attribs.clear();
}

InstanceBuffer& InstanceBuffer::operator=(InstanceBuffer&& other) noexcept {
    if (this != &other) {
        reset();
        m_bufferId = std::exchange(other.m_bufferId, std::nullopt);
        m_strideBytes = std::exchange(other.m_strideBytes, 0);
        m_instanceCount = std::exchange(other.m_instanceCount, 0);
        m_attribs = std::move(other.m_attribs);
        other.m_attribs.clear();
    }
    return *this;
}

InstanceBuffer::~InstanceBuffer() {
    reset();
}

void InstanceBuffer::reset() noexcept {
    if (m_bufferId.has_value()) {
        const auto id = m_bufferId->get_value();
        glDeleteBuffers(1, &id);
        m_bufferId = std::nullopt;
    }
    m_strideBytes = 0;
    m_instanceCount = 0;
    m_attribs.clear();
}

std::optional<InstanceBuffer> InstanceBuffer::create(std::span<const float> data,
                                                     GLsizei strideBytes,
                                                     std::span<const InstanceAttribSpec> attribs,
                                                     BufferAccessPattern accessPattern) {
    GLuint bufferId{0};
    glGenBuffers(1, &bufferId);
    if (bufferId == 0) {
        report_error("Error: glGenBuffers failed to allocate an instance buffer");
        RENDERER_ASSERT(false);
        return std::nullopt;
    }

    glBindBuffer(GL_ARRAY_BUFFER, bufferId);
    const auto size = data.size() * sizeof(float);
    RENDERER_ASSERT(size <= static_cast<std::size_t>(std::numeric_limits<GLsizeiptr>::max()));
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(size), data.data(), get_gl_buffer_usage(accessPattern));

    apply_attribs(attribs, strideBytes);

    std::vector<InstanceAttribSpec> storedAttribs(attribs.begin(), attribs.end());
    const GLsizei instanceCount = compute_instance_count(data.size(), strideBytes);
    return InstanceBuffer{BufferId{bufferId}, strideBytes, instanceCount, std::move(storedAttribs)};
}

const BufferId& InstanceBuffer::get_buffer_id() const {
    RENDERER_ASSERT(m_bufferId.has_value());
    return m_bufferId.value();
}

void InstanceBuffer::bind() const {
    RENDERER_ASSERT(m_bufferId.has_value());
    glBindBuffer(GL_ARRAY_BUFFER, m_bufferId.value().get_value());
    apply_attribs(m_attribs, m_strideBytes);
}

void InstanceBuffer::unbind() {
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void InstanceBuffer::update(std::span<const float> data, BufferAccessPattern accessPattern) {
    RENDERER_ASSERT(m_bufferId.has_value());
    glBindBuffer(GL_ARRAY_BUFFER, m_bufferId.value().get_value());
    const auto size = data.size() * sizeof(float);
    RENDERER_ASSERT(size <= static_cast<std::size_t>(std::numeric_limits<GLsizeiptr>::max()));
    const auto bufferSize = static_cast<GLsizeiptr>(size);
    // Orphan then refill, matching opengl::update_buffer's buffer-orphaning strategy.
    glBufferData(GL_ARRAY_BUFFER, bufferSize, nullptr, get_gl_buffer_usage(accessPattern));
    glBufferSubData(GL_ARRAY_BUFFER, 0, bufferSize, data.data());
    m_instanceCount = compute_instance_count(data.size(), m_strideBytes);
}

InstanceBuffer::InstanceBuffer(BufferId bufferId,
                               GLsizei strideBytes,
                               GLsizei instanceCount,
                               std::vector<InstanceAttribSpec> attribs)
    : m_bufferId{bufferId}
    , m_strideBytes{strideBytes}
    , m_instanceCount{instanceCount}
    , m_attribs{std::move(attribs)} {
}

} // namespace opengl
