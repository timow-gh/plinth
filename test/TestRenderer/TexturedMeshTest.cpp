#include <GLFW/glfw3.h>
#include <OpenGL/Drawable/DrawablesManager.hpp>
#include <OpenGL/Framebuffer.hpp>
#include <OpenGL/OpenGL.hpp>
#include <array>
#include <cmath>
#include <cstdint>
#include <gtest/gtest.h>
#include <plinth/LightingConfig.hpp>
#include <plinth/Renderer.hpp>
#include <plinth/Texture.hpp>
#include <plinth/WindowSettings.hpp>

namespace {

void* load_glfw_proc(const char* procName) {
    // GLAD v1 expects void* while GLFW returns an opaque function pointer.
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    return reinterpret_cast<void*>(glfwGetProcAddress(procName));
}

class TexturedMeshTest : public ::testing::Test {
  protected:
    static void SetUpTestSuite() { ASSERT_EQ(GLFW_TRUE, glfwInit()); }
    static void TearDownTestSuite() { glfwTerminate(); }

    void SetUp() override {
        glfwDefaultWindowHints();
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        m_window = glfwCreateWindow(64, 64, "plinth textured mesh test", nullptr, nullptr);
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

constexpr std::array<float, 9> triangleVertices{-1.0F, -1.0F, 0.0F, 1.0F, -1.0F, 0.0F, -1.0F, 1.0F, 0.0F};
constexpr std::array<float, 9> triangleNormals{0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 1.0F};
// Offset coordinates so the interior pixel at (16, 16) samples the red texel center (0.25, 0.25).
constexpr std::array<float, 6> triangleUvs{0.125F, 0.125F, 0.625F, 0.125F, 0.125F, 0.625F};
constexpr std::array<std::uint32_t, 3> triangleIndices{0U, 1U, 2U};
constexpr std::array<std::uint8_t, 16> texturePixels{
    255U, 0U, 0U, 17U, 0U, 255U, 0U, 34U, 0U, 0U, 255U, 51U, 255U, 255U, 0U, 68U};

std::array<float, 12> colors_with_alpha(float alpha) {
    return {1.0F, 1.0F, 1.0F, alpha, 1.0F, 1.0F, 1.0F, alpha, 1.0F, 1.0F, 1.0F, alpha};
}

TEST_F(TexturedMeshTest, SamplesTextureRgbAndPreservesVertexAlpha) {
    auto framebuffer = opengl::Framebuffer::create(64, 64, 1, false);
    ASSERT_TRUE(framebuffer.has_value());
    auto manager = opengl::DrawablesManager::create();
    ASSERT_NE(nullptr, manager);
    const auto textureId = manager->create_texture_2d({2U, 2U, texturePixels, renderer::TextureColorSpace::linear}, 64);
    ASSERT_TRUE(textureId.has_value());
    const auto meshId = manager->add_textured_mesh_drawable(triangleVertices, 3, triangleNormals, triangleUvs,
                                                            colors_with_alpha(0.5F), 4, triangleIndices, *textureId,
                                                             opengl::BufferAccessPattern::Static);
    ASSERT_TRUE(meshId.has_value());

    framebuffer->bind();
    glViewport(0, 0, 64, 64);
    glDisable(GL_FRAMEBUFFER_SRGB);
    glDisable(GL_BLEND);
    glClearColor(0.0F, 0.0F, 0.0F, 0.0F);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    renderer::LightingConfig lighting;
    lighting.ambientColor = linal::float3{1.0F, 1.0F, 1.0F};
    lighting.lightColor = linal::float3{0.0F, 0.0F, 0.0F};
    lighting.fillLightColor = linal::float3{0.0F, 0.0F, 0.0F};
    manager->draw_meshes(linal::hmatf::identity(), linal::hmatf::identity(), linal::float3{0.0F, 0.0F, 1.0F}, lighting);
    std::array<std::uint8_t, 4> pixel{};
    glReadPixels(16, 16, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel.data());
    opengl::Framebuffer::unbind();

    EXPECT_EQ(GL_NO_ERROR, glGetError());
    EXPECT_NEAR(255, static_cast<int>(pixel[0]), 5);
    EXPECT_NEAR(0, static_cast<int>(pixel[1]), 5);
    EXPECT_NEAR(0, static_cast<int>(pixel[2]), 5);
    EXPECT_NEAR(128, static_cast<int>(pixel[3]), 1);
}

TEST_F(TexturedMeshTest, PublicApiRejectsInvalidTexturesAndRetainsTexturesUsedByMeshes) {
    renderer::WindowSettings settings;
    settings.title = "plinth textured mesh renderer test";
    settings.width = 64;
    settings.height = 64;
    settings.visible = false;
    settings.resizable = false;
    auto renderer = renderer::Renderer::create(settings);
    ASSERT_NE(nullptr, renderer);

    const auto colors = colors_with_alpha(1.0F);
    EXPECT_FALSE(renderer->add_textured_mesh_drawable(triangleVertices, triangleNormals, triangleUvs, colors,
                                                       triangleIndices, {}).is_valid());
    const auto texture = renderer->create_texture_2d({2U, 2U, texturePixels, renderer::TextureColorSpace::linear});
    ASSERT_TRUE(texture.is_valid());
    const auto mesh = renderer->add_textured_mesh_drawable(triangleVertices, triangleNormals, triangleUvs, colors,
                                                            triangleIndices, texture);
    ASSERT_TRUE(mesh.is_valid());
    EXPECT_FALSE(renderer->remove_texture(texture));
    EXPECT_TRUE(renderer->remove_drawable(mesh));
    EXPECT_TRUE(renderer->remove_texture(texture));
}

} // namespace
