#ifndef BUFFER_HPP
#define BUFFER_HPP

#include <plinth/Assert.hpp>
#include <cstring>
#include <memory>
#include <span>
#include <type_traits>

namespace opengl {

template <typename T>
    requires std::is_trivially_copyable_v<T>
class Buffer {
  public:
    using value_type = T;
    using size_type = std::size_t;
    using reference = value_type&;
    using const_reference = const value_type&;
    using iterator = value_type*;
    using const_iterator = const value_type*;

    [[nodiscard]]
    static Buffer create_from(const Buffer& other, size_type additionalCapacity) {
        auto newBuffer = Buffer(other.capacity() + additionalCapacity);
        RENDERER_ASSERT(newBuffer.capacity() == other.capacity() + additionalCapacity);
        RENDERER_ASSERT(newBuffer.is_empty());

        std::uninitialized_copy(other.begin(), other.end(), newBuffer.data());
        newBuffer.m_pos = other.size();

        RENDERER_ASSERT(newBuffer.size() == other.size());
        RENDERER_ASSERT(newBuffer.free_capacity() == other.free_capacity() + additionalCapacity);
        return newBuffer;
    }

    Buffer(size_type capacity)
        : m_capacity(capacity)
        , m_data(std::make_unique<value_type[]>(capacity)) {}

    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;
    Buffer(Buffer&&) = default;
    Buffer& operator=(Buffer&&) = default;
    ~Buffer() = default;

    [[nodiscard]]
    std::span<const value_type> get_as_span() const {
        return std::span<const value_type>(m_data.get(), m_pos);
    }
    [[nodiscard]]
    std::span<value_type> get_as_span() {
        return std::span<value_type>(m_data.get(), m_pos);
    }

    [[nodiscard]]
    value_type* data() {
        return m_data.get();
    }
    [[nodiscard]]
    const value_type* data() const {
        return m_data.get();
    }

    [[nodiscard]]
    iterator begin() {
        return m_data.get();
    }
    [[nodiscard]]
    iterator end() {
        return m_data.get() + m_pos;
    }

    [[nodiscard]]
    const_iterator begin() const {
        return m_data.get();
    }
    [[nodiscard]]
    const_iterator end() const {
        return m_data.get() + m_pos;
    }

    [[nodiscard]]
    const_iterator cbegin() const {
        return m_data.get();
    }
    [[nodiscard]]
    const_iterator cend() const {
        return m_data.get() + m_pos;
    }

    [[nodiscard]]
    reference operator[](size_type index) {
        RENDERER_ASSERT(index < m_pos);
        return m_data[index];
    }

    [[nodiscard]]
    const_reference operator[](size_type index) const {
        RENDERER_ASSERT(index < m_pos);
        return m_data[index];
    }

    // Size of the buffer (number of elements currently stored)
    [[nodiscard]]
    size_type size() const {
        return m_pos;
    }

    // Maximum number of elements the buffer can hold
    [[nodiscard]]
    size_type capacity() const {
        return m_capacity;
    }
    // Number of free elements in the buffer
    [[nodiscard]]
    size_type free_capacity() const {
        return m_capacity - m_pos;
    }
    [[nodiscard]]
    bool is_full() const {
        return m_pos == m_capacity;
    }
    [[nodiscard]]
    bool is_empty() const {
        return m_pos == 0;
    }

    // Check if the buffer has enough space for the given number of elements
    [[nodiscard]]
    bool has_space_for(size_type count) const {
        return count <= free_capacity();
    }

    constexpr void push_back(const value_type& value) noexcept {
        RENDERER_ASSERT(m_pos < m_capacity);
        m_data[m_pos] = value;
        m_pos++;
    }

    constexpr void emplace_back(value_type&& value) noexcept {
        RENDERER_ASSERT(m_pos < m_capacity);
        m_data[m_pos] = std::move(value);
        m_pos++;
    }

    void pop_back() noexcept {
        RENDERER_ASSERT(m_pos > 0);
        m_pos--;
    }

    void reset() { m_pos = 0; }

    void remove(size_type start, size_type size) noexcept {
        RENDERER_ASSERT(start + size <= m_pos);
        // Shift elements to the left to fill the gap
        std::memmove(&m_data[start], &m_data[start + size], (m_pos - (start + size)) * sizeof(value_type));
        m_pos -= size;
    }

  private:
    std::size_t m_capacity{0};
    std::size_t m_pos{0};
    std::unique_ptr<value_type[]> m_data;
};

} // namespace opengl

#endif // BUFFER_HPP
