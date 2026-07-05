#include "Buffer.hpp"
#include <cmath>
#include <gtest/gtest.h>

class BufferTest : public ::testing::Test {};

TEST_F(BufferTest, CreateEmptyBuffer) {
    opengl::Buffer<float> buffer(10);
    EXPECT_EQ(buffer.size(), 0);
    EXPECT_EQ(buffer.capacity(), 10);
    EXPECT_EQ(buffer.free_capacity(), 10);
    EXPECT_TRUE(buffer.is_empty());
    EXPECT_FALSE(buffer.is_full());
}

TEST_F(BufferTest, PushBackElements_NonMove) {
    opengl::Buffer<int> buffer(5);
    buffer.push_back(1);

    EXPECT_EQ(buffer.size(), 1);
    EXPECT_EQ(buffer.capacity(), 5);
    EXPECT_EQ(buffer.free_capacity(), 4);
    EXPECT_FALSE(buffer.is_empty());
    EXPECT_FALSE(buffer.is_full());
}

struct UserType {
    int value{0};
    bool operator==(const UserType&) const = default;
};

TEST_F(BufferTest, PushBackElements_Move) {
    opengl::Buffer<UserType> buffer(5);
    buffer.emplace_back(UserType{1});

    EXPECT_EQ(buffer.size(), 1);
    EXPECT_EQ(buffer.capacity(), 5);
    EXPECT_EQ(buffer.free_capacity(), 4);
    EXPECT_FALSE(buffer.is_empty());
    EXPECT_FALSE(buffer.is_full());
}

TEST_F(BufferTest, CreateFromBuffer) {
    opengl::Buffer<int> original(5);
    for (int i = 0; i < 5; ++i) {
        original.push_back(i);
    }

    auto newBuffer = opengl::Buffer<int>::create_from(original, 3);

    EXPECT_EQ(newBuffer.size(), 5);
    EXPECT_EQ(newBuffer.capacity(), 8); // original capacity + additional size
    EXPECT_EQ(newBuffer.free_capacity(), 3);
    EXPECT_FALSE(newBuffer.is_empty());
    EXPECT_FALSE(newBuffer.is_full());

    for (std::size_t i = 0; i < 5; ++i) {
        EXPECT_EQ(newBuffer.get_as_span()[i], static_cast<int>(i));
    }
}

TEST_F(BufferTest, get_as_span) {
    opengl::Buffer<float> buffer(5);
    buffer.push_back(1.0f);
    buffer.push_back(2.0f);

    auto span = buffer.get_as_span();
    EXPECT_EQ(span.size(), 2);
    EXPECT_EQ(span[0], 1.0f);
    EXPECT_EQ(span[1], 2.0f);

    // Modify the buffer and check the span again
    buffer.push_back(3.0f);
    span = buffer.get_as_span();
    EXPECT_EQ(span.size(), 3);
    EXPECT_EQ(span[2], 3.0f);
}

TEST_F(BufferTest, data) {
    opengl::Buffer<float> buffer(5);
    buffer.push_back(1.0f);
    buffer.push_back(2.0f);

    auto data = buffer.data();
    EXPECT_EQ(data[0], 1.0f);
    EXPECT_EQ(data[1], 2.0f);
}

TEST_F(BufferTest, begin_end) {
    opengl::Buffer<const char*> buffer(5);
    buffer.push_back("Hello");
    buffer.push_back("World");

    auto it = buffer.begin();
    EXPECT_STREQ(*it, "Hello");
    ++it;
    EXPECT_STREQ(*it, "World");
    ++it;
    EXPECT_EQ(it, buffer.end());
}

TEST_F(BufferTest, operator_brackets) {
    opengl::Buffer<double> buffer(5);
    buffer.push_back(1.1);
    buffer.push_back(2.2);
    buffer.push_back(3.3);

    EXPECT_EQ(buffer[0], 1.1);
    EXPECT_EQ(buffer[1], 2.2);
    EXPECT_EQ(buffer[2], 3.3);
}

TEST_F(BufferTest, size_capacity_free_capacity) {
    opengl::Buffer<char> buffer(10);
    EXPECT_EQ(buffer.size(), 0);
    EXPECT_EQ(buffer.capacity(), 10);
    EXPECT_EQ(buffer.free_capacity(), 10);

    buffer.push_back('a');
    EXPECT_EQ(buffer.size(), 1);
    EXPECT_EQ(buffer.free_capacity(), 9);

    for (int i = 0; i < 9; ++i) {
        buffer.push_back('b');
    }

    EXPECT_EQ(buffer.size(), 10);
    EXPECT_EQ(buffer.free_capacity(), 0);
    EXPECT_TRUE(buffer.is_full());
}

TEST_F(BufferTest, is_full_is_empty) {
    opengl::Buffer<char> buffer(3);
    EXPECT_TRUE(buffer.is_empty());
    EXPECT_FALSE(buffer.is_full());

    buffer.push_back('a');
    EXPECT_FALSE(buffer.is_empty());
    EXPECT_FALSE(buffer.is_full());

    buffer.push_back('b');
    buffer.push_back('c');
    EXPECT_FALSE(buffer.is_empty());
    EXPECT_TRUE(buffer.is_full());

    buffer.pop_back();
    EXPECT_FALSE(buffer.is_full());
}

TEST_F(BufferTest, has_space_for) {
    opengl::Buffer<int> buffer(5);
    EXPECT_TRUE(buffer.has_space_for(3));
    EXPECT_TRUE(buffer.has_space_for(5));
    EXPECT_FALSE(buffer.has_space_for(6)); // More than free capacity
}

TEST_F(BufferTest, pop_back) {
    opengl::Buffer<int> buffer(5);
    buffer.push_back(1);
    buffer.push_back(2);
    buffer.push_back(3);

    EXPECT_EQ(buffer.size(), 3);
    buffer.pop_back();
    EXPECT_EQ(buffer.size(), 2);
    EXPECT_EQ(buffer[0], 1);
    EXPECT_EQ(buffer[1], 2);

    // Pop until empty
    buffer.pop_back();
    buffer.pop_back();
    EXPECT_TRUE(buffer.is_empty());
}

TEST_F(BufferTest, remove_shifts_elements_left) {
    opengl::Buffer<int> buffer(5);
    buffer.push_back(1);
    buffer.push_back(2);
    buffer.push_back(3);
    buffer.push_back(4);

    buffer.remove(1, 2); // remove elements at index 1,2 (values 2,3)

    EXPECT_EQ(buffer.size(), 2);
    EXPECT_EQ(buffer[0], 1);
    EXPECT_EQ(buffer[1], 4);
}

TEST_F(BufferTest, reset) {
    opengl::Buffer<float> buffer(5);
    buffer.push_back(1.0f);
    buffer.push_back(2.0f);

    EXPECT_EQ(buffer.size(), 2);
    buffer.reset();
    EXPECT_EQ(buffer.size(), 0);
    EXPECT_TRUE(buffer.is_empty());
    EXPECT_EQ(buffer.free_capacity(), 5);
}
