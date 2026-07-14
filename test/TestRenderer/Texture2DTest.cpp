#include <GLFW/glfw3.h>
#include <OpenGL/OpenGL.hpp>
#include <OpenGL/Texture2D.hpp>
#include <array>
#include <gtest/gtest.h>

namespace {
void* load_glfw_proc(const char* name) { return reinterpret_cast<void*>(glfwGetProcAddress(name)); }
class Texture2DTest : public ::testing::Test {
  protected:
    static void SetUpTestSuite() { ASSERT_EQ(GLFW_TRUE, glfwInit()); }
    static void TearDownTestSuite() { glfwTerminate(); }
    void SetUp() override {
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
        m_window = glfwCreateWindow(64, 64, "texture test", nullptr, nullptr);
        ASSERT_NE(nullptr, m_window);
        glfwMakeContextCurrent(m_window);
        ASSERT_NE(0, gladLoadGLLoader(load_glfw_proc));
    }
    void TearDown() override { glfwDestroyWindow(m_window); }
    GLFWwindow* m_window{nullptr};
};

TEST_F(Texture2DTest, ValidatesUploadsMipmapsAndMoves) {
    const std::array<std::uint8_t, 16> pixels{255, 0, 0, 255, 0, 255, 0, 255, 0, 0, 255, 255, 255, 255, 255, 255};
    EXPECT_FALSE(opengl::Texture2D::create({0, 2, pixels}, 16));
    EXPECT_FALSE(opengl::Texture2D::create({2, 2, std::span<const std::uint8_t>{pixels.data(), 15}}, 16));
    EXPECT_FALSE(opengl::Texture2D::create({17, 2, pixels}, 16));
    auto texture = opengl::Texture2D::create({2, 2, pixels}, 16);
    ASSERT_TRUE(texture);
    const GLuint id = texture->get_id();
    EXPECT_TRUE(texture->is_valid());
    EXPECT_EQ(2, texture->get_width());
    glBindTexture(GL_TEXTURE_2D, id);
    GLint format = 0;
    GLint mipWidth = 0;
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_INTERNAL_FORMAT, &format);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 1, GL_TEXTURE_WIDTH, &mipWidth);
    EXPECT_EQ(GL_SRGB8_ALPHA8, format);
    EXPECT_EQ(1, mipWidth);
    texture->bind(0);
    GLint binding = 0;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &binding);
    EXPECT_EQ(static_cast<GLint>(id), binding);
    EXPECT_EQ(GL_NO_ERROR, glGetError());
    opengl::Texture2D moved{std::move(*texture)};
    EXPECT_FALSE(texture->is_valid());
    EXPECT_EQ(id, moved.get_id());
    auto other = opengl::Texture2D::create({2, 2, pixels}, 16);
    ASSERT_TRUE(other);
    *other = std::move(moved);
    EXPECT_FALSE(moved.is_valid());
    EXPECT_EQ(id, other->get_id());
}
} // namespace
