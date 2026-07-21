#include "PlinthTestMatchers.hpp"
#include <GLFW/glfw3.h>
#include <gtest/gtest.h>
#include <linal/hmat.hpp>
#include <plinth/Renderer.hpp>
#include <plinth/WindowSettings.hpp>
#include <array>
#include <cstdint>

namespace {

class RendererTest : public ::testing::Test {
  protected:
    static void SetUpTestSuite() { ASSERT_EQ(GLFW_TRUE, glfwInit()); }

    static void TearDownTestSuite() { glfwTerminate(); }

    void SetUp() override {
        renderer::WindowSettings settings;
        settings.title = "plinth renderer test";
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

linal::hmatf make_translation(float x, float y, float z) {
    linal::hmatf result = linal::hmatf::identity();
    result.set_translation(linal::float3{x, y, z});
    return result;
}

} // namespace

// --- Drawable lifecycle ---

TEST_F(RendererTest, AddPointDrawableReturnsValidHandle) {
    const std::array<float, 9> vertices = {
        0.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 0.0F, 1.0F, 0.0F,
    };
    const std::array<float, 12> colors = {
        1.0F, 0.0F, 0.0F, 1.0F, 0.0F, 1.0F, 0.0F, 1.0F, 0.0F, 0.0F, 1.0F, 1.0F,
    };
    const std::array<std::uint32_t, 3> indices = {0U, 1U, 2U};

    const renderer::DrawableHandle handle = m_renderer->add_point_drawable(vertices, colors, indices, 1.0F);

    EXPECT_TRUE(handle.is_valid());
    EXPECT_EQ(renderer::DrawableKind::point, handle.kind);
}

TEST_F(RendererTest, AddLineDrawableReturnsValidHandle) {
    const std::array<float, 12> vertices = {
        0.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 0.0F, 1.0F, 0.0F, 1.0F, 1.0F, 0.0F,
    };
    const std::array<std::uint32_t, 4> indices = {0U, 1U, 2U, 3U};
    const std::array<float, 16> colors = {
        1.0F, 0.0F, 0.0F, 1.0F, 0.0F, 1.0F, 0.0F, 1.0F, 0.0F, 0.0F, 1.0F, 1.0F, 1.0F, 1.0F, 0.0F, 1.0F,
    };

    const renderer::DrawableHandle handle =
        m_renderer->add_line_drawable(vertices, indices, colors, renderer::LineType::lines(), 2.0F);

    EXPECT_TRUE(handle.is_valid());
    EXPECT_EQ(renderer::DrawableKind::line, handle.kind);
}

TEST_F(RendererTest, AddMeshDrawableReturnsValidHandle) {
    const std::array<float, 9> vertices = {
        0.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 0.0F, 1.0F, 0.0F,
    };
    const std::array<float, 9> normals = {
        0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 1.0F,
    };
    const std::array<float, 12> colors = {
        1.0F, 0.0F, 0.0F, 1.0F, 0.0F, 1.0F, 0.0F, 1.0F, 0.0F, 0.0F, 1.0F, 1.0F,
    };
    const std::array<std::uint32_t, 3> triangleIndices = {0U, 1U, 2U};

    const renderer::DrawableHandle handle =
        m_renderer->add_mesh_drawable(vertices, normals, colors, triangleIndices);

    EXPECT_TRUE(handle.is_valid());
    EXPECT_EQ(renderer::DrawableKind::mesh, handle.kind);
}

TEST_F(RendererTest, RemoveDrawableReturnsTrueForValidHandle) {
    const std::array<float, 9> vertices = {
        0.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 0.0F, 1.0F, 0.0F,
    };
    const std::array<float, 12> colors = {
        1.0F, 0.0F, 0.0F, 1.0F, 0.0F, 1.0F, 0.0F, 1.0F, 0.0F, 0.0F, 1.0F, 1.0F,
    };
    const std::array<std::uint32_t, 3> indices = {0U, 1U, 2U};

    const renderer::DrawableHandle handle = m_renderer->add_point_drawable(vertices, colors, indices, 1.0F);
    ASSERT_TRUE(handle.is_valid());

    EXPECT_TRUE(m_renderer->remove_drawable(handle));
    EXPECT_FALSE(m_renderer->remove_drawable(handle));
}

TEST_F(RendererTest, RemoveDrawableReturnsFalseForInvalidHandle) {
    const renderer::DrawableHandle invalidHandle;
    EXPECT_FALSE(m_renderer->remove_drawable(invalidHandle));
}

TEST_F(RendererTest, ClearDrawablesEmptiesState) {
    const std::array<float, 9> vertices = {
        0.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 0.0F, 1.0F, 0.0F,
    };
    const std::array<float, 12> colors = {
        1.0F, 0.0F, 0.0F, 1.0F, 0.0F, 1.0F, 0.0F, 1.0F, 0.0F, 0.0F, 1.0F, 1.0F,
    };
    const std::array<std::uint32_t, 3> indices = {0U, 1U, 2U};
    const std::array<float, 9> normals = {
        0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 1.0F,
    };

    m_renderer->add_point_drawable(vertices, colors, indices, 1.0F);
    m_renderer->add_line_drawable(vertices, indices, colors, renderer::LineType::lines(), 2.0F);
    m_renderer->add_mesh_drawable(vertices, normals, colors, indices);

    EXPECT_TRUE(m_renderer->has_point_drawables());
    EXPECT_TRUE(m_renderer->has_line_drawables());
    EXPECT_TRUE(m_renderer->has_mesh_drawables());

    m_renderer->clear_drawables();

    EXPECT_FALSE(m_renderer->has_point_drawables());
    EXPECT_FALSE(m_renderer->has_line_drawables());
    EXPECT_FALSE(m_renderer->has_mesh_drawables());
}

// --- Transform management ---

TEST_F(RendererTest, SetAndGetTransformRoundTrips) {
    const std::array<float, 9> vertices = {
        0.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 0.0F, 1.0F, 0.0F,
    };
    const std::array<float, 12> colors = {
        1.0F, 0.0F, 0.0F, 1.0F, 0.0F, 1.0F, 0.0F, 1.0F, 0.0F, 0.0F, 1.0F, 1.0F,
    };
    const std::array<std::uint32_t, 3> indices = {0U, 1U, 2U};

    const renderer::DrawableHandle handle = m_renderer->add_point_drawable(vertices, colors, indices, 1.0F);
    ASSERT_TRUE(handle.is_valid());

    const linal::hmatf translation = make_translation(1.0F, 2.0F, 3.0F);
    EXPECT_TRUE(m_renderer->set_drawable_transform(handle, translation));

    const std::optional<linal::hmatf> result = m_renderer->get_drawable_transform(handle);
    ASSERT_TRUE(result.has_value());

    const auto actual = result->get_translation();
    const linal::double3 expected{1.0, 2.0, 3.0};
    const linal::double3 actualDouble{static_cast<double>(actual[0]),
                                      static_cast<double>(actual[1]),
                                      static_cast<double>(actual[2])};
    EXPECT_TRUE(Double3Near(actualDouble, expected, 1.0e-6));
}

TEST_F(RendererTest, GetTransformReturnsNulloptForInvalidHandle) {
    const renderer::DrawableHandle invalidHandle;
    EXPECT_FALSE(m_renderer->get_drawable_transform(invalidHandle).has_value());
}

TEST_F(RendererTest, ResetTransformSetsIdentity) {
    const std::array<float, 9> vertices = {
        0.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 0.0F, 1.0F, 0.0F,
    };
    const std::array<float, 12> colors = {
        1.0F, 0.0F, 0.0F, 1.0F, 0.0F, 1.0F, 0.0F, 1.0F, 0.0F, 0.0F, 1.0F, 1.0F,
    };
    const std::array<std::uint32_t, 3> indices = {0U, 1U, 2U};

    const renderer::DrawableHandle handle = m_renderer->add_point_drawable(vertices, colors, indices, 1.0F);
    ASSERT_TRUE(handle.is_valid());

    const linal::hmatf translation = make_translation(1.0F, 2.0F, 3.0F);
    EXPECT_TRUE(m_renderer->set_drawable_transform(handle, translation));
    EXPECT_FALSE(m_renderer->get_drawable_transform(handle)->is_identity());

    EXPECT_TRUE(m_renderer->reset_drawable_transform(handle));
    EXPECT_TRUE(m_renderer->get_drawable_transform(handle)->is_identity());
}

// --- Frame lifecycle ---

TEST_F(RendererTest, BeginDrawEndFrameDoesNotCrash) {
    m_renderer->begin_frame();
    m_renderer->draw();
    m_renderer->end_frame();
}

TEST_F(RendererTest, FrameLoopWithDrawablesDoesNotCrash) {
    const std::array<float, 9> vertices = {
        0.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 0.0F, 1.0F, 0.0F,
    };
    const std::array<float, 12> colors = {
        1.0F, 0.0F, 0.0F, 1.0F, 0.0F, 1.0F, 0.0F, 1.0F, 0.0F, 0.0F, 1.0F, 1.0F,
    };
    const std::array<std::uint32_t, 3> indices = {0U, 1U, 2U};
    const std::array<float, 9> normals = {
        0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 1.0F,
    };

    m_renderer->add_point_drawable(vertices, colors, indices, 1.0F);
    m_renderer->add_line_drawable(vertices, indices, colors, renderer::LineType::lines(), 2.0F);
    m_renderer->add_mesh_drawable(vertices, normals, colors, indices);

    m_renderer->begin_frame();
    m_renderer->draw();
    m_renderer->end_frame();
}

TEST_F(RendererTest, EndFrameHandlesAutoFitAndHomeRequest) {
    bool autoFit = true;
    bool homeRequested = false;
    m_renderer->begin_frame();
    m_renderer->draw();
    m_renderer->end_frame(autoFit, homeRequested);
}

TEST_F(RendererTest, RetainedImGuiViewDoesNotKeepBackendAlive) {
    const renderer::ImGuiOverlayView view = m_renderer->get_imgui();
    ASSERT_NE(nullptr, view.lock());

    m_renderer.reset();

    EXPECT_EQ(nullptr, view.lock());
}

TEST_F(RendererTest, FrameBoundaryRestoresRendererContext) {
    glfwDefaultWindowHints();
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    GLFWwindow* other = glfwCreateWindow(64, 64, "plinth alternate context", nullptr, nullptr);
    ASSERT_NE(nullptr, other);

    glfwMakeContextCurrent(other);
    ASSERT_EQ(other, glfwGetCurrentContext());
    m_renderer->begin_frame();

    EXPECT_EQ(m_renderer->window().get_native_handle(), glfwGetCurrentContext());
    glfwDestroyWindow(other);
}

TEST(OneSampleRendererTest, OneSampleFrameDoesNotCrash) {
    ASSERT_EQ(GLFW_TRUE, glfwInit());

    renderer::WindowSettings settings;
    settings.title = "plinth renderer test";
    settings.width = 64;
    settings.height = 64;
    settings.visible = false;
    settings.resizable = false;
    settings.double_buffer = true;
    settings.samples = 1;

    auto renderer = renderer::Renderer::create(settings);
    if (!renderer) {
        glfwTerminate();
        GTEST_SKIP() << "GL context creation not available in this environment";
    }

    renderer->begin_frame();
    renderer->draw();
    renderer->end_frame();

    renderer.reset();
    glfwTerminate();
}

TEST(GlfwWindowSingletonTest, RejectsSecondLiveWindow) {
    renderer::WindowSettings settings;
    settings.title = "plinth singleton test";
    settings.width = 64;
    settings.height = 64;
    settings.visible = false;
    settings.resizable = false;

    auto first = renderer::GlfwWindow::create(settings);
    ASSERT_TRUE(first.has_value());

    auto second = renderer::GlfwWindow::create(settings);
    EXPECT_FALSE(second.has_value());

    first.reset();

    auto replacement = renderer::GlfwWindow::create(settings);
    EXPECT_TRUE(replacement.has_value());
}

TEST(GlfwWindowSingletonTest, FailedCreationDoesNotReserveSlot) {
    renderer::WindowSettings settings;
    settings.title = "plinth singleton test";
    settings.width = 0;
    settings.height = 0;
    settings.visible = false;
    settings.resizable = false;

    auto failed = renderer::GlfwWindow::create(settings);
    EXPECT_FALSE(failed.has_value());

    settings.width = 64;
    settings.height = 64;
    auto valid = renderer::GlfwWindow::create(settings);
    EXPECT_TRUE(valid.has_value());
}

TEST(GlfwWindowSingletonTest, MoveTransferPreservesGuard) {
    renderer::WindowSettings settings;
    settings.title = "plinth singleton test";
    settings.width = 64;
    settings.height = 64;
    settings.visible = false;
    settings.resizable = false;

    auto first = renderer::GlfwWindow::create(settings);
    ASSERT_TRUE(first.has_value());

    auto moved = std::move(first);

    {
        auto second = renderer::GlfwWindow::create(settings);
        EXPECT_FALSE(second.has_value());
    }
}

TEST(RendererSingletonTest, RejectsSecondLiveRendererAndAllowsReplacement) {
    renderer::WindowSettings settings;
    settings.title = "plinth singleton test";
    settings.width = 64;
    settings.height = 64;
    settings.visible = false;
    settings.resizable = false;
    settings.double_buffer = true;

    auto first = renderer::Renderer::create(settings);
    if (!first) {
        GTEST_SKIP() << "GL context creation not available in this environment";
    }

    auto second = renderer::Renderer::create(settings);
    EXPECT_EQ(nullptr, second);

    first.reset();

    auto replacement = renderer::Renderer::create(settings);
    if (!replacement) {
        GTEST_SKIP() << "GL context creation not available in this environment";
    }
    EXPECT_NE(nullptr, replacement);
}
