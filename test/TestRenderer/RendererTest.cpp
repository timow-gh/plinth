#include "PlinthTestMatchers.hpp"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <gtest/gtest.h>
#include <linal/hmat.hpp>
#include "plinth/ImGuiOverlay.hpp"
#include "plinth/Renderer.hpp"
#include "plinth/WindowSettings.hpp"
#include <array>
#include <cstdint>
#include <span>

namespace {

template <typename T>
concept HasQueryOnlyBufferPattern = requires { T::READ_ONLY; };

template <typename T>
concept HasTriangleLineType = requires { T::triangles(); };

static_assert(!HasQueryOnlyBufferPattern<renderer::BufferAccessPattern>);
static_assert(!HasTriangleLineType<renderer::LineType>);

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

void dispatch_cursor_pos(renderer::Renderer& renderer, double xpos, double ypos) {
    auto* window = static_cast<GLFWwindow*>(renderer.window().get_native_handle());
    ASSERT_NE(nullptr, window);
    const GLFWcursorposfun callback = glfwSetCursorPosCallback(window, nullptr);
    ASSERT_NE(nullptr, callback);
    glfwSetCursorPosCallback(window, callback);
    callback(window, xpos, ypos);
}

void dispatch_mouse_button(renderer::Renderer& renderer, int button, renderer::Action action) {
    auto* window = static_cast<GLFWwindow*>(renderer.window().get_native_handle());
    ASSERT_NE(nullptr, window);
    const GLFWmousebuttonfun callback = glfwSetMouseButtonCallback(window, nullptr);
    ASSERT_NE(nullptr, callback);
    glfwSetMouseButtonCallback(window, callback);
    callback(window, button, static_cast<int>(action), 0);
}

void dispatch_key(renderer::Renderer& renderer, renderer::Key key) {
    auto* window = static_cast<GLFWwindow*>(renderer.window().get_native_handle());
    ASSERT_NE(nullptr, window);
    const GLFWkeyfun callback = glfwSetKeyCallback(window, nullptr);
    ASSERT_NE(nullptr, callback);
    glfwSetKeyCallback(window, callback);
    callback(window, static_cast<int>(key), 0, static_cast<int>(renderer::Action::PRESS), 0);
}

constexpr std::array<float, 9> localAutoFitVertices = {
    -1.0F,
    -1.0F,
    0.0F,
    1.0F,
    -1.0F,
    0.0F,
    0.0F,
    1.0F,
    0.0F,
};
constexpr std::array<float, 9> tinyAutoFitVertices = {
    -0.001F,
    -0.001F,
    0.0F,
    0.001F,
    -0.001F,
    0.0F,
    0.0F,
    0.001F,
    0.0F,
};
constexpr std::array<float, 9> farAutoFitVertices = {
    99.0F,
    -1.0F,
    0.0F,
    101.0F,
    -1.0F,
    0.0F,
    100.0F,
    1.0F,
    0.0F,
};
constexpr std::array<float, 9> autoFitNormals = {
    0.0F,
    0.0F,
    1.0F,
    0.0F,
    0.0F,
    1.0F,
    0.0F,
    0.0F,
    1.0F,
};
constexpr std::array<float, 12> autoFitColors = {
    1.0F,
    1.0F,
    1.0F,
    1.0F,
    1.0F,
    1.0F,
    1.0F,
    1.0F,
    1.0F,
    1.0F,
    1.0F,
    1.0F,
};
constexpr std::array<std::uint32_t, 3> autoFitIndices = {0U, 1U, 2U};

std::shared_ptr<renderer::CameraInteractor> camera_for(renderer::Renderer& renderer) {
    return renderer.get_camera().lock();
}

void reset_camera_to_default(renderer::Renderer& renderer) {
    const auto camera = camera_for(renderer);
    ASSERT_NE(nullptr, camera);
    camera->look_at(camera->get_default_position(), camera->get_default_target(), camera->get_default_up());
}

void enable_immediate_auto_fit(renderer::Renderer& renderer, bool zoomInEnabled = false) {
    renderer::CameraAutoFitSettings settings = renderer.get_camera_auto_fit_settings();
    settings.enabled = true;
    settings.zoomInEnabled = zoomInEnabled;
    settings.suppressAfterUserCameraInteraction = std::chrono::milliseconds{0};
    renderer.set_camera_auto_fit_settings(settings);

    const auto camera = camera_for(renderer);
    ASSERT_NE(nullptr, camera);
    camera->set_view_transition_duration(0.0);

    // Consume the settings change while the scene is empty. The mutation under
    // test must be the operation that schedules the next fit.
    renderer.begin_frame();
}

void expect_camera_targets_far_geometry(renderer::Renderer& renderer) {
    const auto camera = camera_for(renderer);
    ASSERT_NE(nullptr, camera);
    EXPECT_GT(camera->get_target()[0], 50.0);
}

} // namespace

TEST_F(RendererTest, BeginFrameUsesSelectedDepthConvention) {
    m_renderer->begin_frame();

    GLint depthFunc = 0;
    GLdouble clearDepth = -1.0;
    glGetIntegerv(GL_DEPTH_FUNC, &depthFunc);
    glGetDoublev(GL_DEPTH_CLEAR_VALUE, &clearDepth);
    if (m_renderer->uses_reversed_depth()) {
        GLint clipDepthMode = 0;
        glGetIntegerv(GL_CLIP_DEPTH_MODE, &clipDepthMode);
        EXPECT_EQ(GL_ZERO_TO_ONE, clipDepthMode);
        EXPECT_EQ(GL_GREATER, depthFunc);
        EXPECT_DOUBLE_EQ(0.0, clearDepth);
    } else {
        EXPECT_EQ(GL_LESS, depthFunc);
        EXPECT_DOUBLE_EQ(1.0, clearDepth);
    }
    EXPECT_EQ(GL_NO_ERROR, glGetError());
}

// --- Drawable lifecycle ---

TEST_F(RendererTest, AddPointDrawableReturnsValidHandle) {
    const std::array<float, 9> vertices = {
        0.0F,
        0.0F,
        0.0F,
        1.0F,
        0.0F,
        0.0F,
        0.0F,
        1.0F,
        0.0F,
    };
    const std::array<float, 12> colors = {
        1.0F,
        0.0F,
        0.0F,
        1.0F,
        0.0F,
        1.0F,
        0.0F,
        1.0F,
        0.0F,
        0.0F,
        1.0F,
        1.0F,
    };
    const std::array<std::uint32_t, 3> indices = {0U, 1U, 2U};

    const renderer::DrawableHandle handle = m_renderer->add_point_drawable(vertices, indices, colors, 1.0F);

    EXPECT_TRUE(handle.is_valid());
    EXPECT_EQ(renderer::DrawableKind::point, handle.kind);
}

TEST_F(RendererTest, AddLineDrawableReturnsValidHandle) {
    const std::array<float, 12> vertices = {
        0.0F,
        0.0F,
        0.0F,
        1.0F,
        0.0F,
        0.0F,
        0.0F,
        1.0F,
        0.0F,
        1.0F,
        1.0F,
        0.0F,
    };
    const std::array<std::uint32_t, 4> indices = {0U, 1U, 2U, 3U};
    const std::array<float, 16> colors = {
        1.0F,
        0.0F,
        0.0F,
        1.0F,
        0.0F,
        1.0F,
        0.0F,
        1.0F,
        0.0F,
        0.0F,
        1.0F,
        1.0F,
        1.0F,
        1.0F,
        0.0F,
        1.0F,
    };

    const renderer::DrawableHandle handle =
        m_renderer->add_line_drawable(vertices, indices, colors, renderer::LineType::lines(), renderer::StrokeStyle{.lineWidth = 2.0F});

    EXPECT_TRUE(handle.is_valid());
    EXPECT_EQ(renderer::DrawableKind::line, handle.kind);
}

TEST_F(RendererTest, BufferAccessPatternsCreateBuffersWithoutGlErrors) {
    const std::array<float, 9> vertices = {
        0.0F,
        0.0F,
        0.0F,
        1.0F,
        0.0F,
        0.0F,
        0.0F,
        1.0F,
        0.0F,
    };
    const std::array<float, 12> colors = {
        1.0F,
        0.0F,
        0.0F,
        1.0F,
        0.0F,
        1.0F,
        0.0F,
        1.0F,
        0.0F,
        0.0F,
        1.0F,
        1.0F,
    };
    const std::array<std::uint32_t, 3> indices = {0U, 1U, 2U};

    for (const renderer::BufferAccessPattern accessPattern: {renderer::BufferAccessPattern::Stream,
                                                             renderer::BufferAccessPattern::Static,
                                                             renderer::BufferAccessPattern::Dynamic}) {
        while (glGetError() != GL_NO_ERROR) {}
        EXPECT_TRUE(m_renderer->add_point_drawable(vertices, indices, colors, 1.0F, accessPattern).is_valid());
        EXPECT_EQ(GL_NO_ERROR, glGetError());
    }
}

TEST_F(RendererTest, LineTypesCreateLineDrawablesWithoutGlErrors) {
    const std::array<float, 12> vertices = {
        0.0F,
        0.0F,
        0.0F,
        1.0F,
        0.0F,
        0.0F,
        0.0F,
        1.0F,
        0.0F,
        1.0F,
        1.0F,
        0.0F,
    };
    const std::array<std::uint32_t, 4> indices = {0U, 1U, 2U, 3U};
    const std::array<float, 16> colors = {
        1.0F,
        0.0F,
        0.0F,
        1.0F,
        0.0F,
        1.0F,
        0.0F,
        1.0F,
        0.0F,
        0.0F,
        1.0F,
        1.0F,
        1.0F,
        1.0F,
        0.0F,
        1.0F,
    };

    for (const renderer::LineType lineType:
         {renderer::LineType::lines(), renderer::LineType::line_strip(), renderer::LineType::line_loop()}) {
        while (glGetError() != GL_NO_ERROR) {}
        EXPECT_TRUE(m_renderer->add_line_drawable(vertices, indices, colors, lineType, renderer::StrokeStyle{.lineWidth = 2.0F}).is_valid());
        EXPECT_EQ(GL_NO_ERROR, glGetError());
    }

    m_renderer->begin_frame();
    m_renderer->draw();
    m_renderer->end_frame();
    EXPECT_EQ(GL_NO_ERROR, glGetError());
}

TEST_F(RendererTest, AddMeshDrawableReturnsValidHandle) {
    const std::array<float, 9> vertices = {
        0.0F,
        0.0F,
        0.0F,
        1.0F,
        0.0F,
        0.0F,
        0.0F,
        1.0F,
        0.0F,
    };
    const std::array<float, 9> normals = {
        0.0F,
        0.0F,
        1.0F,
        0.0F,
        0.0F,
        1.0F,
        0.0F,
        0.0F,
        1.0F,
    };
    const std::array<float, 12> colors = {
        1.0F,
        0.0F,
        0.0F,
        1.0F,
        0.0F,
        1.0F,
        0.0F,
        1.0F,
        0.0F,
        0.0F,
        1.0F,
        1.0F,
    };
    const std::array<std::uint32_t, 3> triangleIndices = {0U, 1U, 2U};

    const renderer::DrawableHandle handle = m_renderer->add_mesh_drawable(vertices, triangleIndices, normals, colors);

    EXPECT_TRUE(handle.is_valid());
    EXPECT_EQ(renderer::DrawableKind::mesh, handle.kind);
}

// --- Convenience overloads: computed defaults ---

TEST_F(RendererTest, AddPointDrawableSingleColorNoIndicesReturnsValidHandle) {
    const std::array<float, 9> vertices = {
        0.0F,
        0.0F,
        0.0F,
        1.0F,
        0.0F,
        0.0F,
        0.0F,
        1.0F,
        0.0F,
    };
    const std::array<float, 4> color = {1.0F, 0.0F, 0.0F, 1.0F};

    const renderer::DrawableHandle handle = m_renderer->add_point_drawable(vertices, color);

    EXPECT_TRUE(handle.is_valid());
    EXPECT_EQ(renderer::DrawableKind::point, handle.kind);
}

TEST_F(RendererTest, AddPointDrawablePerVertexColorNoIndicesReturnsValidHandle) {
    const std::array<float, 9> vertices = {
        0.0F,
        0.0F,
        0.0F,
        1.0F,
        0.0F,
        0.0F,
        0.0F,
        1.0F,
        0.0F,
    };
    const std::array<float, 12> colors = {
        1.0F,
        0.0F,
        0.0F,
        1.0F,
        0.0F,
        1.0F,
        0.0F,
        1.0F,
        0.0F,
        0.0F,
        1.0F,
        1.0F,
    };

    const renderer::DrawableHandle handle = m_renderer->add_point_drawable(vertices, std::span<const float>(colors));

    EXPECT_TRUE(handle.is_valid());
    EXPECT_EQ(renderer::DrawableKind::point, handle.kind);
}

TEST_F(RendererTest, AddLineDrawableSingleColorReturnsValidHandle) {
    const std::array<float, 12> vertices = {
        0.0F,
        0.0F,
        0.0F,
        1.0F,
        0.0F,
        0.0F,
        0.0F,
        1.0F,
        0.0F,
        1.0F,
        1.0F,
        0.0F,
    };
    const std::array<float, 4> color = {0.0F, 1.0F, 0.0F, 1.0F};

    const renderer::DrawableHandle handle = m_renderer->add_line_drawable(vertices, color, renderer::LineType::lines());

    EXPECT_TRUE(handle.is_valid());
    EXPECT_EQ(renderer::DrawableKind::line, handle.kind);
}

TEST_F(RendererTest, AddLineDrawablePerVertexColorNoIndicesReturnsValidHandle) {
    const std::array<float, 12> vertices = {
        0.0F,
        0.0F,
        0.0F,
        1.0F,
        0.0F,
        0.0F,
        0.0F,
        1.0F,
        0.0F,
        1.0F,
        1.0F,
        0.0F,
    };
    const std::array<float, 16> colors = {
        1.0F,
        0.0F,
        0.0F,
        1.0F,
        0.0F,
        1.0F,
        0.0F,
        1.0F,
        0.0F,
        0.0F,
        1.0F,
        1.0F,
        1.0F,
        1.0F,
        0.0F,
        1.0F,
    };

    const renderer::DrawableHandle handle =
        m_renderer->add_line_drawable(vertices, std::span<const float>(colors), renderer::LineType::lines());

    EXPECT_TRUE(handle.is_valid());
    EXPECT_EQ(renderer::DrawableKind::line, handle.kind);
}

TEST_F(RendererTest, AddMeshDrawableSingleColorComputesNormals) {
    const std::array<float, 9> vertices = {
        0.0F,
        0.0F,
        0.0F,
        1.0F,
        0.0F,
        0.0F,
        0.0F,
        1.0F,
        0.0F,
    };
    const std::array<float, 4> color = {0.5F, 0.5F, 0.5F, 1.0F};
    const std::array<std::uint32_t, 3> triangleIndices = {0U, 1U, 2U};

    while (glGetError() != GL_NO_ERROR) {}
    const renderer::DrawableHandle handle = m_renderer->add_mesh_drawable(vertices, triangleIndices, color);
    ASSERT_TRUE(handle.is_valid());
    EXPECT_EQ(renderer::DrawableKind::mesh, handle.kind);

    m_renderer->begin_frame();
    m_renderer->draw();
    m_renderer->end_frame();
    EXPECT_EQ(GL_NO_ERROR, glGetError());
}

TEST_F(RendererTest, AddMeshDrawablePerVertexColorComputesNormals) {
    const std::array<float, 9> vertices = {
        0.0F,
        0.0F,
        0.0F,
        1.0F,
        0.0F,
        0.0F,
        0.0F,
        1.0F,
        0.0F,
    };
    const std::array<float, 12> colors = {
        1.0F,
        0.0F,
        0.0F,
        1.0F,
        0.0F,
        1.0F,
        0.0F,
        1.0F,
        0.0F,
        0.0F,
        1.0F,
        1.0F,
    };
    const std::array<std::uint32_t, 3> triangleIndices = {0U, 1U, 2U};

    while (glGetError() != GL_NO_ERROR) {}
    const renderer::DrawableHandle handle =
        m_renderer->add_mesh_drawable(vertices, triangleIndices, std::span<const float>(colors));
    ASSERT_TRUE(handle.is_valid());
    EXPECT_EQ(renderer::DrawableKind::mesh, handle.kind);

    m_renderer->begin_frame();
    m_renderer->draw();
    m_renderer->end_frame();
    EXPECT_EQ(GL_NO_ERROR, glGetError());
}

TEST_F(RendererTest, AddMeshDrawableSingleColorExplicitNormals) {
    const std::array<float, 9> vertices = {
        0.0F,
        0.0F,
        0.0F,
        1.0F,
        0.0F,
        0.0F,
        0.0F,
        1.0F,
        0.0F,
    };
    const std::array<float, 9> normals = {
        0.0F,
        0.0F,
        1.0F,
        0.0F,
        0.0F,
        1.0F,
        0.0F,
        0.0F,
        1.0F,
    };
    const std::array<float, 4> color = {0.5F, 0.5F, 0.5F, 1.0F};
    const std::array<std::uint32_t, 3> triangleIndices = {0U, 1U, 2U};

    const renderer::DrawableHandle handle =
        m_renderer->add_mesh_drawable(vertices, triangleIndices, color, std::span<const float>(normals));

    EXPECT_TRUE(handle.is_valid());
    EXPECT_EQ(renderer::DrawableKind::mesh, handle.kind);
}

TEST_F(RendererTest, AddMeshDrawableComputedNormalsSingleTriangle) {
    // A single CCW triangle in the z=0 plane. Normals are computed internally and
    // are not publicly readable, so assert the drawable renders without GL errors.
    const std::array<float, 9> vertices = {
        0.0F,
        0.0F,
        0.0F,
        1.0F,
        0.0F,
        0.0F,
        0.0F,
        1.0F,
        0.0F,
    };
    const std::array<float, 4> color = {1.0F, 1.0F, 1.0F, 1.0F};
    const std::array<std::uint32_t, 3> triangleIndices = {0U, 1U, 2U};

    while (glGetError() != GL_NO_ERROR) {}
    const renderer::DrawableHandle handle = m_renderer->add_mesh_drawable(vertices, triangleIndices, color);
    ASSERT_TRUE(handle.is_valid());

    m_renderer->begin_frame();
    m_renderer->draw();
    m_renderer->end_frame();
    EXPECT_EQ(GL_NO_ERROR, glGetError());
}

TEST_F(RendererTest, AddMeshDrawableDegenerateTriangleFallsBackToZUp) {
    // Three identical vertices produce a zero-length face normal; the computed
    // normal falls back to {0,0,1}. The drawable must still be created and render.
    const std::array<float, 9> vertices = {
        0.0F,
        0.0F,
        0.0F,
        0.0F,
        0.0F,
        0.0F,
        0.0F,
        0.0F,
        0.0F,
    };
    const std::array<float, 4> color = {1.0F, 1.0F, 1.0F, 1.0F};
    const std::array<std::uint32_t, 3> triangleIndices = {0U, 1U, 2U};

    while (glGetError() != GL_NO_ERROR) {}
    const renderer::DrawableHandle handle = m_renderer->add_mesh_drawable(vertices, triangleIndices, color);
    ASSERT_TRUE(handle.is_valid());

    m_renderer->begin_frame();
    m_renderer->draw();
    m_renderer->end_frame();
    EXPECT_EQ(GL_NO_ERROR, glGetError());
}

TEST_F(RendererTest, AddPointDrawableSingleColorEmptyVerticesDoesNotError) {
    const std::span<const float> vertices;
    const std::array<float, 4> color = {1.0F, 1.0F, 1.0F, 1.0F};

    while (glGetError() != GL_NO_ERROR) {}
    // Empty vertices produce empty color/index buffers via the computed defaults.
    // The fully-parameterized path accepts this and creates an (empty) point
    // drawable of kind point; creation must not raise a GL error.
    const renderer::DrawableHandle handle = m_renderer->add_point_drawable(vertices, color);

    EXPECT_TRUE(handle.is_valid());
    EXPECT_EQ(renderer::DrawableKind::point, handle.kind);

    m_renderer->begin_frame();
    m_renderer->draw();
    m_renderer->end_frame();
    EXPECT_EQ(GL_NO_ERROR, glGetError());
}

TEST_F(RendererTest, RemoveDrawableReturnsTrueForValidHandle) {
    const std::array<float, 9> vertices = {
        0.0F,
        0.0F,
        0.0F,
        1.0F,
        0.0F,
        0.0F,
        0.0F,
        1.0F,
        0.0F,
    };
    const std::array<float, 12> colors = {
        1.0F,
        0.0F,
        0.0F,
        1.0F,
        0.0F,
        1.0F,
        0.0F,
        1.0F,
        0.0F,
        0.0F,
        1.0F,
        1.0F,
    };
    const std::array<std::uint32_t, 3> indices = {0U, 1U, 2U};

    const renderer::DrawableHandle handle = m_renderer->add_point_drawable(vertices, indices, colors, 1.0F);
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
        0.0F,
        0.0F,
        0.0F,
        1.0F,
        0.0F,
        0.0F,
        0.0F,
        1.0F,
        0.0F,
    };
    const std::array<float, 12> colors = {
        1.0F,
        0.0F,
        0.0F,
        1.0F,
        0.0F,
        1.0F,
        0.0F,
        1.0F,
        0.0F,
        0.0F,
        1.0F,
        1.0F,
    };
    const std::array<std::uint32_t, 3> indices = {0U, 1U, 2U};
    const std::array<float, 9> normals = {
        0.0F,
        0.0F,
        1.0F,
        0.0F,
        0.0F,
        1.0F,
        0.0F,
        0.0F,
        1.0F,
    };

    m_renderer->add_point_drawable(vertices, indices, colors, 1.0F);
    m_renderer->add_line_drawable(vertices, indices, colors, renderer::LineType::lines(), renderer::StrokeStyle{.lineWidth = 2.0F});
    m_renderer->add_mesh_drawable(vertices, indices, normals, colors);

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
        0.0F,
        0.0F,
        0.0F,
        1.0F,
        0.0F,
        0.0F,
        0.0F,
        1.0F,
        0.0F,
    };
    const std::array<float, 12> colors = {
        1.0F,
        0.0F,
        0.0F,
        1.0F,
        0.0F,
        1.0F,
        0.0F,
        1.0F,
        0.0F,
        0.0F,
        1.0F,
        1.0F,
    };
    const std::array<std::uint32_t, 3> indices = {0U, 1U, 2U};

    const renderer::DrawableHandle handle = m_renderer->add_point_drawable(vertices, indices, colors, 1.0F);
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
        0.0F,
        0.0F,
        0.0F,
        1.0F,
        0.0F,
        0.0F,
        0.0F,
        1.0F,
        0.0F,
    };
    const std::array<float, 12> colors = {
        1.0F,
        0.0F,
        0.0F,
        1.0F,
        0.0F,
        1.0F,
        0.0F,
        1.0F,
        0.0F,
        0.0F,
        1.0F,
        1.0F,
    };
    const std::array<std::uint32_t, 3> indices = {0U, 1U, 2U};

    const renderer::DrawableHandle handle = m_renderer->add_point_drawable(vertices, indices, colors, 1.0F);
    ASSERT_TRUE(handle.is_valid());

    const linal::hmatf translation = make_translation(1.0F, 2.0F, 3.0F);
    EXPECT_TRUE(m_renderer->set_drawable_transform(handle, translation));
    EXPECT_FALSE(m_renderer->get_drawable_transform(handle)->is_identity());

    EXPECT_TRUE(m_renderer->reset_drawable_transform(handle));
    EXPECT_TRUE(m_renderer->get_drawable_transform(handle)->is_identity());
}

TEST_F(RendererTest, DrawableKindsSupportTransformAndRemoval) {
    const std::array<float, 9> vertices = {
        0.0F,
        0.0F,
        0.0F,
        1.0F,
        0.0F,
        0.0F,
        0.0F,
        1.0F,
        0.0F,
    };
    const std::array<float, 9> normals = {
        0.0F,
        0.0F,
        1.0F,
        0.0F,
        0.0F,
        1.0F,
        0.0F,
        0.0F,
        1.0F,
    };
    const std::array<float, 12> colors = {
        1.0F,
        0.0F,
        0.0F,
        1.0F,
        0.0F,
        1.0F,
        0.0F,
        1.0F,
        0.0F,
        0.0F,
        1.0F,
        1.0F,
    };
    const std::array<std::uint32_t, 3> indices = {0U, 1U, 2U};

    const std::array handles = {
        m_renderer->add_point_drawable(vertices, indices, colors, 1.0F),
        m_renderer->add_line_drawable(vertices, indices, colors, renderer::LineType::lines(), renderer::StrokeStyle{.lineWidth = 1.0F}),
        m_renderer->add_mesh_drawable(vertices, indices, normals, colors),
    };

    for (const renderer::DrawableHandle handle: handles) {
        ASSERT_TRUE(handle.is_valid());
        EXPECT_TRUE(m_renderer->set_drawable_transform(handle, make_translation(1.0F, 2.0F, 3.0F)));
        EXPECT_TRUE(m_renderer->get_drawable_transform(handle).has_value());
        EXPECT_TRUE(m_renderer->reset_drawable_transform(handle));
        EXPECT_TRUE(m_renderer->remove_drawable(handle));
    }
}

TEST_F(RendererTest, StaleHandlesCannotAffectReplacementRenderer) {
    const std::array<float, 9> vertices = {
        0.0F,
        0.0F,
        0.0F,
        1.0F,
        0.0F,
        0.0F,
        0.0F,
        1.0F,
        0.0F,
    };
    const std::array<float, 9> normals = {
        0.0F,
        0.0F,
        1.0F,
        0.0F,
        0.0F,
        1.0F,
        0.0F,
        0.0F,
        1.0F,
    };
    const std::array<float, 12> colors = {
        1.0F,
        0.0F,
        0.0F,
        1.0F,
        0.0F,
        1.0F,
        0.0F,
        1.0F,
        0.0F,
        0.0F,
        1.0F,
        1.0F,
    };
    const std::array<std::uint32_t, 3> indices = {0U, 1U, 2U};
    const std::array<std::uint8_t, 4> pixels = {255U, 255U, 255U, 255U};

    const renderer::DrawableHandle staleDrawable = m_renderer->add_mesh_drawable(vertices, indices, normals, colors);
    const renderer::TextureHandle staleTexture = m_renderer->create_texture_2d({1U, 1U, pixels});
    ASSERT_TRUE(staleDrawable.is_valid());
    ASSERT_TRUE(staleTexture.is_valid());
    m_renderer.reset();

    renderer::WindowSettings settings;
    settings.title = "plinth replacement renderer test";
    settings.width = 64;
    settings.height = 64;
    settings.visible = false;
    settings.resizable = false;
    settings.double_buffer = true;
    m_renderer = renderer::Renderer::create(settings);
    ASSERT_NE(nullptr, m_renderer);

    const renderer::DrawableHandle replacement = m_renderer->add_mesh_drawable(vertices, indices, normals, colors);
    const renderer::TextureHandle replacementTexture = m_renderer->create_texture_2d({1U, 1U, pixels});
    ASSERT_TRUE(replacement.is_valid());
    ASSERT_TRUE(replacementTexture.is_valid());
    ASSERT_NE(staleDrawable.rendererInstance, replacement.rendererInstance);
    ASSERT_NE(staleTexture.rendererInstance, replacementTexture.rendererInstance);

    EXPECT_FALSE(m_renderer->remove_texture(staleTexture));
    EXPECT_FALSE(
        m_renderer->add_textured_mesh_drawable(vertices, normals, {}, colors, indices, staleTexture).is_valid());
    EXPECT_FALSE(m_renderer->remove_drawable(staleDrawable));
    EXPECT_FALSE(m_renderer->set_drawable_transform(staleDrawable, make_translation(1.0F, 2.0F, 3.0F)));
    EXPECT_FALSE(m_renderer->get_drawable_transform(staleDrawable).has_value());
    EXPECT_FALSE(m_renderer->reset_drawable_transform(staleDrawable));
    m_renderer->set_mesh_drawable_cull_mode(staleDrawable, renderer::MeshCullFaceMode::NONE);

    EXPECT_TRUE(m_renderer->get_drawable_transform(replacement)->is_identity());
    EXPECT_TRUE(m_renderer->remove_drawable(replacement));
    EXPECT_TRUE(m_renderer->remove_texture(replacementTexture));
}

// --- Frame lifecycle ---

TEST_F(RendererTest, BeginDrawEndFrameDoesNotCrash) {
    m_renderer->begin_frame();
    m_renderer->draw();
    m_renderer->end_frame();
}

TEST_F(RendererTest, FrameLoopWithDrawablesDoesNotCrash) {
    const std::array<float, 9> vertices = {
        0.0F,
        0.0F,
        0.0F,
        1.0F,
        0.0F,
        0.0F,
        0.0F,
        1.0F,
        0.0F,
    };
    const std::array<float, 12> colors = {
        1.0F,
        0.0F,
        0.0F,
        1.0F,
        0.0F,
        1.0F,
        0.0F,
        1.0F,
        0.0F,
        0.0F,
        1.0F,
        1.0F,
    };
    const std::array<std::uint32_t, 3> indices = {0U, 1U, 2U};
    const std::array<float, 9> normals = {
        0.0F,
        0.0F,
        1.0F,
        0.0F,
        0.0F,
        1.0F,
        0.0F,
        0.0F,
        1.0F,
    };

    m_renderer->add_point_drawable(vertices, indices, colors, 1.0F);
    m_renderer->add_line_drawable(vertices, indices, colors, renderer::LineType::lines(), renderer::StrokeStyle{.lineWidth = 2.0F});
    m_renderer->add_mesh_drawable(vertices, indices, normals, colors);

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

TEST_F(RendererTest, NoArgumentEndFrameRetainsRendererOwnedAutoFitState) {
    bool autoFit = true;
    m_renderer->begin_frame();
    m_renderer->draw();
    m_renderer->end_frame(autoFit);
    ASSERT_TRUE(m_renderer->is_auto_fit_enabled());

    m_renderer->begin_frame();
    m_renderer->draw();
    m_renderer->end_frame();
    EXPECT_TRUE(m_renderer->is_auto_fit_enabled());
}

TEST_F(RendererTest, CameraAutoFitConfigurationRoundTripsAndPreservesDisabledDefault) {
    EXPECT_FALSE(m_renderer->is_auto_fit_enabled());
    EXPECT_FALSE(m_renderer->get_camera_auto_fit_settings().enabled);
    EXPECT_DOUBLE_EQ(3.0, m_renderer->get_camera_far_plane_multiplier());

    renderer::CameraAutoFitSettings settings;
    settings.enabled = true;
    settings.zoomInEnabled = false;
    settings.zoomOutPadding = 1.4;
    settings.minViewportOccupancy = 0.15;
    settings.targetViewportOccupancy = 0.7;
    settings.suppressAfterUserCameraInteraction = std::chrono::milliseconds{275};
    m_renderer->set_camera_auto_fit_settings(settings);
    m_renderer->set_camera_far_plane_multiplier(6.5);

    const renderer::CameraAutoFitSettings actual = m_renderer->get_camera_auto_fit_settings();
    EXPECT_TRUE(actual.enabled);
    EXPECT_FALSE(actual.zoomInEnabled);
    EXPECT_DOUBLE_EQ(1.4, actual.zoomOutPadding);
    EXPECT_DOUBLE_EQ(0.15, actual.minViewportOccupancy);
    EXPECT_DOUBLE_EQ(0.7, actual.targetViewportOccupancy);
    EXPECT_EQ(std::chrono::milliseconds{275}, actual.suppressAfterUserCameraInteraction);
    EXPECT_DOUBLE_EQ(6.5, m_renderer->get_camera_far_plane_multiplier());
    EXPECT_TRUE(m_renderer->is_auto_fit_enabled());
}

TEST_F(RendererTest, ExplicitRequestRefitsExistingGeometry) {
    enable_immediate_auto_fit(*m_renderer, true);
    ASSERT_TRUE(m_renderer->add_point_drawable(tinyAutoFitVertices, autoFitColors).is_valid());
    m_renderer->begin_frame();

    const auto camera = camera_for(*m_renderer);
    ASSERT_NE(nullptr, camera);
    camera->look_at({0.0, 0.0, 100.0}, {0.0, 0.0, 0.0}, {0.0, 1.0, 0.0});
    const double distanceBefore = linal::length(camera->get_position() - camera->get_target());

    m_renderer->request_auto_fit();
    m_renderer->begin_frame();

    const double distanceAfter = linal::length(camera->get_position() - camera->get_target());
    EXPECT_LT(distanceAfter, distanceBefore);
}

TEST_F(RendererTest, PendingFitSurvivesDisabledStateUntilEndFrameEnablesIt) {
    const auto camera = camera_for(*m_renderer);
    ASSERT_NE(nullptr, camera);
    camera->set_view_transition_duration(0.0);
    ASSERT_TRUE(m_renderer->add_point_drawable(farAutoFitVertices, autoFitColors).is_valid());

    m_renderer->begin_frame();
    EXPECT_NEAR(0.0, camera->get_target()[0], 1.0e-9);

    bool enabled = true;
    m_renderer->end_frame(enabled);
    m_renderer->begin_frame();
    expect_camera_targets_far_geometry(*m_renderer);
}

TEST_F(RendererTest, HomeFitsGeometryWhileAutomaticFittingIsDisabled) {
    const auto camera = camera_for(*m_renderer);
    ASSERT_NE(nullptr, camera);
    camera->set_view_transition_duration(0.0);
    ASSERT_FALSE(m_renderer->is_auto_fit_enabled());
    ASSERT_TRUE(
        m_renderer->add_mesh_drawable(farAutoFitVertices, autoFitIndices, autoFitNormals, autoFitColors).is_valid());

    m_renderer->go_to_home_view();

    expect_camera_targets_far_geometry(*m_renderer);
    EXPECT_FALSE(m_renderer->is_auto_fit_enabled());
}

TEST_F(RendererTest, ConfiguredFarPlaneMultiplierControlsClipFitting) {
    ASSERT_TRUE(
        m_renderer->add_mesh_drawable(localAutoFitVertices, autoFitIndices, autoFitNormals, autoFitColors).is_valid());
    const auto camera = camera_for(*m_renderer);
    ASSERT_NE(nullptr, camera);

    m_renderer->set_camera_far_plane_multiplier(1.0);
    m_renderer->begin_frame();
    const double nearFarPlane = camera->get_far_plane();

    m_renderer->set_camera_far_plane_multiplier(10.0);
    m_renderer->begin_frame();
    EXPECT_GT(camera->get_far_plane(), nearFarPlane);
}

TEST_F(RendererTest, RecentCameraInteractionSuppressesOnlyZoomIn) {
    enable_immediate_auto_fit(*m_renderer, true);
    renderer::CameraAutoFitSettings settings = m_renderer->get_camera_auto_fit_settings();
    settings.minViewportOccupancy = 0.5;
    settings.targetViewportOccupancy = 0.65;
    settings.suppressAfterUserCameraInteraction = std::chrono::hours{1};
    m_renderer->set_camera_auto_fit_settings(settings);
    ASSERT_TRUE(m_renderer->add_point_drawable(tinyAutoFitVertices, autoFitColors).is_valid());
    m_renderer->begin_frame();

    const auto camera = camera_for(*m_renderer);
    ASSERT_NE(nullptr, camera);
    camera->look_at({0.0, 0.0, 100.0}, {0.0, 0.0, 0.0}, {0.0, 1.0, 0.0});
    m_renderer->window().get_input_state().cursorPosState = {32.0, 32.0};
    camera->on_scroll(0.0, 0.0); // records interaction without changing the pose
    const double distanceAfterInteraction = linal::length(camera->get_position() - camera->get_target());

    m_renderer->request_auto_fit();
    m_renderer->begin_frame();
    EXPECT_NEAR(distanceAfterInteraction, linal::length(camera->get_position() - camera->get_target()), 1.0e-9);

    settings.suppressAfterUserCameraInteraction = std::chrono::milliseconds{0};
    m_renderer->set_camera_auto_fit_settings(settings);
    m_renderer->begin_frame();
    EXPECT_LT(linal::length(camera->get_position() - camera->get_target()), distanceAfterInteraction);
}

namespace {

enum class AutoFitAddition {
    point,
    line,
    mesh,
    texturedMesh,
};

class RendererAutoFitAdditionTest
    : public RendererTest
    , public ::testing::WithParamInterface<AutoFitAddition> {};

} // namespace

TEST_P(RendererAutoFitAdditionTest, SuccessfulAdditionSchedulesFit) {
    enable_immediate_auto_fit(*m_renderer);

    renderer::DrawableHandle handle;
    switch (GetParam()) {
    case AutoFitAddition::point: handle = m_renderer->add_point_drawable(farAutoFitVertices, autoFitColors); break;
    case AutoFitAddition::line:
        handle = m_renderer->add_line_drawable(farAutoFitVertices, autoFitColors, renderer::LineType::line_strip());
        break;
    case AutoFitAddition::mesh:
        handle = m_renderer->add_mesh_drawable(farAutoFitVertices, autoFitIndices, autoFitNormals, autoFitColors);
        break;
    case AutoFitAddition::texturedMesh: {
        constexpr std::array<std::uint8_t, 4> pixel = {255U, 255U, 255U, 255U};
        constexpr std::array<float, 6> textureCoordinates = {0.0F, 0.0F, 1.0F, 0.0F, 0.5F, 1.0F};
        const renderer::TextureHandle texture = m_renderer->create_texture_2d({1U, 1U, pixel});
        ASSERT_TRUE(texture.is_valid());
        handle = m_renderer->add_textured_mesh_drawable(farAutoFitVertices,
                                                        autoFitNormals,
                                                        textureCoordinates,
                                                        autoFitColors,
                                                        autoFitIndices,
                                                        texture);
        break;
    }
    }
    ASSERT_TRUE(handle.is_valid());

    m_renderer->begin_frame();
    expect_camera_targets_far_geometry(*m_renderer);
}

INSTANTIATE_TEST_SUITE_P(AllDrawableKinds,
                         RendererAutoFitAdditionTest,
                         ::testing::Values(AutoFitAddition::point,
                                           AutoFitAddition::line,
                                           AutoFitAddition::mesh,
                                           AutoFitAddition::texturedMesh));

namespace {

enum class AutoFitMutation {
    pointUpdate,
    lineUpdate,
    remove,
    clearPoints,
    clearLines,
    clearMeshes,
    transform,
    resetTransform,
};

class RendererAutoFitMutationTest
    : public RendererTest
    , public ::testing::WithParamInterface<AutoFitMutation> {};

} // namespace

TEST_P(RendererAutoFitMutationTest, SuccessfulBoundsMutationSchedulesFit) {
    enable_immediate_auto_fit(*m_renderer);
    renderer::DrawableHandle affected;

    switch (GetParam()) {
    case AutoFitMutation::pointUpdate:
    case AutoFitMutation::transform:
        affected = m_renderer->add_point_drawable(localAutoFitVertices, autoFitColors);
        break;
    case AutoFitMutation::lineUpdate:
        affected = m_renderer->add_line_drawable(localAutoFitVertices, autoFitColors, renderer::LineType::line_strip());
        break;
    case AutoFitMutation::remove:
    case AutoFitMutation::clearPoints:
        ASSERT_TRUE(m_renderer->add_mesh_drawable(farAutoFitVertices, autoFitIndices, autoFitNormals, autoFitColors)
                        .is_valid());
        affected = m_renderer->add_point_drawable(localAutoFitVertices, autoFitColors);
        break;
    case AutoFitMutation::clearLines:
        ASSERT_TRUE(m_renderer->add_point_drawable(farAutoFitVertices, autoFitColors).is_valid());
        affected = m_renderer->add_line_drawable(localAutoFitVertices, autoFitColors, renderer::LineType::line_strip());
        break;
    case AutoFitMutation::clearMeshes:
        ASSERT_TRUE(m_renderer->add_point_drawable(farAutoFitVertices, autoFitColors).is_valid());
        affected = m_renderer->add_mesh_drawable(localAutoFitVertices, autoFitIndices, autoFitNormals, autoFitColors);
        break;
    case AutoFitMutation::resetTransform:
        affected = m_renderer->add_point_drawable(farAutoFitVertices, autoFitColors);
        ASSERT_TRUE(m_renderer->set_drawable_transform(affected, make_translation(-100.0F, 0.0F, 0.0F)));
        break;
    }
    ASSERT_TRUE(affected.is_valid());

    m_renderer->begin_frame(); // consume setup mutations
    reset_camera_to_default(*m_renderer);

    switch (GetParam()) {
    case AutoFitMutation::pointUpdate:
        m_renderer->update_last_point_drawable(farAutoFitVertices,
                                               autoFitColors,
                                               autoFitIndices,
                                               renderer::BufferAccessPattern::Dynamic);
        break;
    case AutoFitMutation::lineUpdate:
        m_renderer->update_last_line_drawable(farAutoFitVertices,
                                              autoFitColors,
                                              autoFitIndices,
                                              renderer::BufferAccessPattern::Dynamic);
        break;
    case AutoFitMutation::remove:      ASSERT_TRUE(m_renderer->remove_drawable(affected)); break;
    case AutoFitMutation::clearPoints: m_renderer->clear_point_drawables(); break;
    case AutoFitMutation::clearLines:  m_renderer->clear_line_drawables(); break;
    case AutoFitMutation::clearMeshes: m_renderer->clear_mesh_drawables(); break;
    case AutoFitMutation::transform:
        ASSERT_TRUE(m_renderer->set_drawable_transform(affected, make_translation(100.0F, 0.0F, 0.0F)));
        break;
    case AutoFitMutation::resetTransform: ASSERT_TRUE(m_renderer->reset_drawable_transform(affected)); break;
    }

    m_renderer->begin_frame();
    expect_camera_targets_far_geometry(*m_renderer);
}

INSTANTIATE_TEST_SUITE_P(AllBoundsChanges,
                         RendererAutoFitMutationTest,
                         ::testing::Values(AutoFitMutation::pointUpdate,
                                           AutoFitMutation::lineUpdate,
                                           AutoFitMutation::remove,
                                           AutoFitMutation::clearPoints,
                                           AutoFitMutation::clearLines,
                                           AutoFitMutation::clearMeshes,
                                           AutoFitMutation::transform,
                                           AutoFitMutation::resetTransform));

TEST_F(RendererTest, FailedAndEmptyGeometryCallsDoNotScheduleFit) {
    enable_immediate_auto_fit(*m_renderer, true);
    ASSERT_TRUE(
        m_renderer->add_mesh_drawable(localAutoFitVertices, autoFitIndices, autoFitNormals, autoFitColors).is_valid());
    m_renderer->begin_frame();

    const auto camera = camera_for(*m_renderer);
    ASSERT_NE(nullptr, camera);
    camera->look_at({0.0, -100.0, 100.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0});
    const linal::double3 positionBefore = camera->get_position();

    const renderer::DrawableHandle invalid;
    EXPECT_FALSE(m_renderer->remove_drawable(invalid));
    EXPECT_FALSE(m_renderer->set_drawable_transform(invalid, make_translation(100.0F, 0.0F, 0.0F)));
    m_renderer->update_last_point_drawable(farAutoFitVertices,
                                           autoFitColors,
                                           autoFitIndices,
                                           renderer::BufferAccessPattern::Dynamic);
    m_renderer->update_last_line_drawable(farAutoFitVertices,
                                          autoFitColors,
                                          autoFitIndices,
                                          renderer::BufferAccessPattern::Dynamic);
    m_renderer->clear_point_drawables();
    m_renderer->clear_line_drawables();

    m_renderer->begin_frame();
    EXPECT_TRUE(Double3Near(camera->get_position(), positionBefore, 1.0e-9));
}

TEST_F(RendererTest, CallbackSubscriptionDisconnectsOnDestruction) {
    int calls = 0;
    {
        const auto subscription = m_renderer->add_key_callback(
            [&calls](renderer::Key, renderer::Scancode, renderer::Action, renderer::Mods) { ++calls; });
        dispatch_key(*m_renderer, renderer::Key::KEY_A);
        EXPECT_EQ(1, calls);
        EXPECT_TRUE(subscription.is_connected());
    }
    dispatch_key(*m_renderer, renderer::Key::KEY_A);
    EXPECT_EQ(1, calls);
}

TEST_F(RendererTest, CallbackSubscriptionCanDisconnectExplicitlyAndDuringDispatch) {
    int explicitCalls = 0;
    auto explicitSubscription = m_renderer->add_key_callback(
        [&explicitCalls](renderer::Key, renderer::Scancode, renderer::Action, renderer::Mods) { ++explicitCalls; });
    explicitSubscription.disconnect();
    EXPECT_FALSE(explicitSubscription.is_connected());
    dispatch_key(*m_renderer, renderer::Key::KEY_A);
    EXPECT_EQ(0, explicitCalls);

    int selfCalls = 0;
    renderer::CallbackSubscription selfSubscription;
    selfSubscription = m_renderer->add_key_callback(
        [&selfCalls, &selfSubscription](renderer::Key, renderer::Scancode, renderer::Action, renderer::Mods) {
            ++selfCalls;
            selfSubscription.disconnect();
        });
    dispatch_key(*m_renderer, renderer::Key::KEY_A);
    dispatch_key(*m_renderer, renderer::Key::KEY_A);
    EXPECT_EQ(1, selfCalls);
}

TEST_F(RendererTest, InjectedOverlayIsReleasedWithRenderer) {
    // Only one live Renderer is supported, and injecting a second ImGui overlay onto a
    // renderer that already owns one would double-initialize the ImGui backend. Release the
    // fixture renderer and create one with no built-in overlay.
    m_renderer.reset();

    renderer::WindowSettings settings;
    settings.title = "plinth renderer overlay-injection test";
    settings.width = 64;
    settings.height = 64;
    settings.visible = false;
    settings.resizable = false;
    settings.overlay = renderer::OverlayKind::None;
    m_renderer = renderer::Renderer::create(settings);
    ASSERT_NE(nullptr, m_renderer);

    auto overlay = std::make_shared<renderer::ImGuiOverlay>(m_renderer->window().get_native_handle());
    const std::weak_ptr<renderer::ImGuiOverlay> view = overlay;
    m_renderer->set_overlay(std::move(overlay));
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

TEST_F(RendererTest, IsSrgbCapableLeavesNoGlfwErrorAndMatchesDriverReport) {
    // Drain any GLFW errors left over from window setup.
    while (glfwGetError(nullptr) != GLFW_NO_ERROR) {}

    const bool reported = m_renderer->window().is_srgb_capable();
    EXPECT_EQ(GLFW_NO_ERROR, glfwGetError(nullptr)) << "is_srgb_capable must not raise a GLFW error";

    GLint encoding = GL_LINEAR;
    glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER,
                                          GL_BACK_LEFT,
                                          GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING,
                                          &encoding);
    EXPECT_EQ(reported, encoding == GL_SRGB)
        << "is_srgb_capable must agree with the driver-reported default-framebuffer encoding";
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

TEST(CameraGestureRendererTest, CameraGestureSurvivesUnrelatedButtonRelease) {
    renderer::WindowSettings settings;
    settings.title = "plinth camera gesture test";
    settings.width = 800;
    settings.height = 600;
    settings.visible = false;
    auto renderer = renderer::Renderer::create(settings);
    ASSERT_NE(nullptr, renderer);
    const auto camera = renderer->get_camera().lock();
    ASSERT_NE(nullptr, camera);

    // Cursor well right of the ~336 px reserved control-panel strip so events
    // reach the scene; no ImGui frame has run yet, so nothing is captured.
    dispatch_cursor_pos(*renderer, 700.0, 300.0);
    dispatch_mouse_button(*renderer, 1, renderer::Action::PRESS); // right: orbit
    dispatch_cursor_pos(*renderer, 660.0, 300.0);
    const linal::double3 afterFirstDrag = camera->get_position();

    // Middle click mid-orbit: press is ignored; release must not cancel the
    // right-button orbit gesture.
    dispatch_mouse_button(*renderer, 2, renderer::Action::PRESS);
    dispatch_mouse_button(*renderer, 2, renderer::Action::RELEASE);
    dispatch_cursor_pos(*renderer, 620.0, 300.0);

    EXPECT_NE(camera->get_position(), afterFirstDrag)
        << "releasing the middle button must not cancel the right-button orbit";

    dispatch_mouse_button(*renderer, 1, renderer::Action::RELEASE);
}
