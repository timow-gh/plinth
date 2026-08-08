#include "plinth/Renderer.hpp"
#include "plinth/WindowSettings.hpp"
#include "plinth/loader/MeshData.hpp"

#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <gtest/gtest.h>
#include <memory>

namespace {

class MeshDataToDrawableTest : public ::testing::Test {
  protected:
    static void SetUpTestSuite() { ASSERT_EQ(GLFW_TRUE, glfwInit()); }
    static void TearDownTestSuite() { glfwTerminate(); }

    void SetUp() override {
        renderer::WindowSettings settings;
        settings.title = "plinth mesh data test";
        settings.width = 64;
        settings.height = 64;
        settings.visible = false;
        settings.resizable = false;
        settings.double_buffer = true;
        m_renderer = renderer::Renderer::create(settings);
        ASSERT_NE(nullptr, m_renderer);
    }

    void TearDown() override { m_renderer.reset(); }

    std::unique_ptr<renderer::Renderer> m_renderer;
};

renderer::MeshData make_triangle() {
    renderer::MeshData mesh;
    mesh.vertices = {0.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 0.0F, 1.0F, 0.0F};
    mesh.triangleIndices = {0U, 1U, 2U};
    return mesh;
}

TEST_F(MeshDataToDrawableTest, AutoFillsNormalsAndColors) {
    // Omitted normals and colors are filled in by the overload.
    const renderer::MeshData mesh = make_triangle();
    const renderer::DrawableHandle handle = m_renderer->add_mesh_drawable(mesh);
    EXPECT_TRUE(handle.is_valid());
    EXPECT_TRUE(m_renderer->has_mesh_drawables());
}

TEST_F(MeshDataToDrawableTest, GeneratesSequentialIndicesWhenMissing) {
    renderer::MeshData mesh = make_triangle();
    mesh.triangleIndices.clear(); // force sequential index generation
    const renderer::DrawableHandle handle = m_renderer->add_mesh_drawable(mesh);
    EXPECT_TRUE(handle.is_valid());
}

TEST_F(MeshDataToDrawableTest, EmptyMeshYieldsInvalidHandle) {
    const renderer::MeshData empty;
    const renderer::DrawableHandle handle = m_renderer->add_mesh_drawable(empty);
    EXPECT_FALSE(handle.is_valid());
    EXPECT_FALSE(m_renderer->has_mesh_drawables());
}

} // namespace
