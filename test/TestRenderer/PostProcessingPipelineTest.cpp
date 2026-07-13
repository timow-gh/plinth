#include <GLFW/glfw3.h>
#include <OpenGL/Framebuffer.hpp>
#include <OpenGL/FrameState.hpp>
#include <OpenGL/OpenGL.hpp>
#include <OpenGL/Programs/PostProcessingProgram.hpp>
#include <OpenGL/VertexArray.hpp>
#include <array>
#include <cmath>
#include <cstdint>
#include <gtest/gtest.h>

namespace {

void* load_glfw_proc(const char* procName) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    return reinterpret_cast<void*>(glfwGetProcAddress(procName));
}

class PostProcessingPipelineTest : public ::testing::Test {
  protected:
    static void SetUpTestSuite() { ASSERT_EQ(GLFW_TRUE, glfwInit()); }

    static void TearDownTestSuite() { glfwTerminate(); }

    void SetUp() override {
        glfwDefaultWindowHints();
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        m_window = glfwCreateWindow(64, 64, "plinth post-processing test", nullptr, nullptr);
        ASSERT_NE(nullptr, m_window);
        glfwMakeContextCurrent(m_window);
        ASSERT_NE(0, gladLoadGLLoader(load_glfw_proc));
    }

    void TearDown() override {
        if (m_window != nullptr) {
            glfwDestroyWindow(m_window);
            m_window = nullptr;
        }
    }

    GLFWwindow* m_window{nullptr};
};

} // namespace

TEST_F(PostProcessingPipelineTest, IdentityPassReproducesClearColor) {
    auto fbo = opengl::Framebuffer::create(64, 64, 1, false);
    ASSERT_TRUE(fbo.has_value());
    ASSERT_NE(0U, fbo->get_color_texture());

    const float expectedR = 0.25F;
    const float expectedG = 0.50F;
    const float expectedB = 0.75F;
    const float expectedA = 1.0F;

    renderer::ClearColor clearColor{expectedR, expectedG, expectedB, expectedA};
    renderer::ViewportRect viewport{0, 0, 64, 64};

    fbo->bind();
    opengl::begin_frame(clearColor, viewport, false);

    opengl::Framebuffer::unbind();

    auto program = opengl::make_post_processing_program();
    ASSERT_TRUE(program.is_valid());

    auto vao = opengl::VertexArray::create();
    ASSERT_TRUE(vao.has_value());

    program.use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, fbo->get_color_texture());
    glUniform1i(program.get_scene_texture_location().get_value(), 0);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);

    glViewport(0, 0, 64, 64);

    vao->bind();
    glDrawArrays(GL_TRIANGLES, 0, 3);
    opengl::VertexArray::unbind();

    glBindTexture(GL_TEXTURE_2D, 0);

    std::array<std::uint8_t, 4> pixel{};
    glReadPixels(32, 32, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel.data());

    ASSERT_EQ(GL_NO_ERROR, glGetError());

    constexpr float scale = 1.0F / 255.0F;
    const float readR = static_cast<float>(pixel[0]) * scale;
    const float readG = static_cast<float>(pixel[1]) * scale;
    const float readB = static_cast<float>(pixel[2]) * scale;

    EXPECT_NEAR(expectedR, readR, scale * 2.0F);
    EXPECT_NEAR(expectedG, readG, scale * 2.0F);
    EXPECT_NEAR(expectedB, readB, scale * 2.0F);

    EXPECT_EQ(GL_NO_ERROR, glGetError());

    GLint currentFbo = 0;
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &currentFbo);
    EXPECT_EQ(0, currentFbo);
}
