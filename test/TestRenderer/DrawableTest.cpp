#include <GLFW/glfw3.h>
#include <OpenGL/Drawable/DrawablesManager.hpp>
#include <OpenGL/Drawable/LineDrawable.hpp>
#include <OpenGL/Drawable/MeshDrawable.hpp>
#include <OpenGL/Drawable/PointDrawable.hpp>
#include <OpenGL/OpenGL.hpp>
#include <array>
#include <cstdint>
#include <gtest/gtest.h>
#include <optional>
#include <span>
#include <vector>

namespace {

void* load_glfw_proc(const char* procName) {
    // GLAD v1 expects void* while GLFW returns an opaque function pointer.
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    return reinterpret_cast<void*>(glfwGetProcAddress(procName));
}

class OpenGLDrawableTest : public ::testing::Test {
  protected:
    static void SetUpTestSuite() { ASSERT_EQ(GLFW_TRUE, glfwInit()); }

    static void TearDownTestSuite() { glfwTerminate(); }

    void SetUp() override {
        glfwDefaultWindowHints();
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        m_window = glfwCreateWindow(64, 64, "plinth drawable test", nullptr, nullptr);
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

linal::hmatf matrix() {
    linal::hmatf result;
    return result;
}

linal::hmatf make_translation(float x, float y, float z) {
    linal::hmatf result = linal::hmatf::identity();
    result.set_translation(linal::vec3<float>{x, y, z});
    return result;
}

opengl::PointDrawable make_point_drawable_with_alpha(opengl::PointProgram& program, const std::vector<float>& colors) {
    const std::vector<float> vertices = {
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
    const std::vector<std::uint32_t> indices = {0U, 1U, 2U};

    std::optional<opengl::PointDrawable> drawable =
        opengl::make_point_drawable(program,
                                    vertices,
                                    3,
                                    colors,
                                    4,
                                    indices,
                                    3.0F,
                                    opengl::BufferAccessPattern::STATIC_DRAW);
    EXPECT_TRUE(drawable.has_value());
    return std::move(drawable.value());
}

opengl::LineDrawable
make_line_drawable_with_alpha(opengl::LineProgram& program, const std::vector<float>& colors, float pointSize) {
    const std::vector<float> vertices = {
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
    const std::vector<std::uint32_t> indices = {0U, 1U, 2U, 3U};

    std::optional<opengl::LineDrawable> drawable = opengl::make_line_drawable(program,
                                                                              vertices,
                                                                              3,
                                                                              indices,
                                                                              colors,
                                                                              4,
                                                                              opengl::LineType::lines(),
                                                                              2.0F,
                                                                              pointSize,
                                                                              opengl::BufferAccessPattern::STATIC_DRAW);
    EXPECT_TRUE(drawable.has_value());
    return std::move(drawable.value());
}

opengl::MeshDrawable make_mesh_drawable_with_alpha(opengl::MeshProgram& program, const std::vector<float>& colors) {
    const std::vector<float> vertices = {
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
    const std::vector<float> normals = {
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
    const std::vector<std::uint32_t> indices = {0U, 1U, 2U};

    std::optional<opengl::MeshDrawable> drawable = opengl::make_mesh_soup(program,
                                                                          vertices,
                                                                          3,
                                                                          normals,
                                                                          colors,
                                                                          4,
                                                                          indices,
                                                                          opengl::BufferAccessPattern::STATIC_DRAW);
    EXPECT_TRUE(drawable.has_value());
    return std::move(drawable.value());
}

opengl::DrawablesManager::DrawableId
add_mesh_drawable_with_alpha(opengl::DrawablesManager& manager, const std::vector<float>& colors) {
    const std::vector<float> vertices = {
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
    const std::vector<float> normals = {
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
    const std::vector<std::uint32_t> indices = {0U, 1U, 2U};

    const auto id =
        manager.add_mesh_drawable(vertices, 3, normals, colors, 4, indices, opengl::BufferAccessPattern::STATIC_DRAW);
    EXPECT_TRUE(id.has_value());
    return *id;
}

} // namespace

TEST(DrawableTransparencyInfoTest, HandlesEdgeCasesAndSortableCollections) {
    const std::vector<float> translucentColors = {
        1.0F,
        0.0F,
        0.0F,
        1.0F,
        0.0F,
        1.0F,
        0.0F,
        0.5F,
    };
    const std::vector<float> rgbColors = {
        1.0F,
        0.0F,
        0.0F,
        0.0F,
        1.0F,
        0.0F,
    };
    EXPECT_TRUE(opengl::contains_translucent_alpha(translucentColors, 4));
    EXPECT_FALSE(opengl::contains_translucent_alpha(rgbColors, 3));
    EXPECT_TRUE(opengl::make_vertex_translucency_flags(rgbColors, 3).empty());

    const opengl::DrawableTransparencyInfo info{true, linal::float3{1.0F, 2.0F, 3.0F}};
    EXPECT_DOUBLE_EQ(14.0, info.distance_squared_to(linal::double3{2.0, 4.0, 6.0}));

    const std::vector<float> vertices = {
        0.0F,
        0.0F,
        0.0F,
        2.0F,
        4.0F,
        6.0F,
    };
    const linal::float3 center = opengl::calc_sort_center(vertices, 3);
    EXPECT_FLOAT_EQ(1.0F, center[0]);
    EXPECT_FLOAT_EQ(2.0F, center[1]);
    EXPECT_FLOAT_EQ(3.0F, center[2]);

    const std::vector<float> incompleteVertex = {1.0F, 2.0F};
    EXPECT_FLOAT_EQ(0.0F, opengl::calc_sort_center(vertices, 2)[0]);
    EXPECT_FLOAT_EQ(0.0F, opengl::calc_sort_center(std::span<const float>{}, 3)[0]);
    EXPECT_FLOAT_EQ(0.0F, opengl::calc_sort_center(incompleteVertex, 3)[0]);
    EXPECT_TRUE(opengl::make_vertex_sort_positions(vertices, 2).empty());

    const std::vector<linal::float3> positions = {
        linal::float3{0.0F, 0.0F, 0.0F},
        linal::float3{1.0F, 0.0F, 0.0F},
    };
    EXPECT_FLOAT_EQ(0.0F, opengl::get_sort_position_or_origin(positions, 7U)[0]);

    const std::vector<std::uint32_t> pointIndices = {0U, 1U, 7U};
    const std::vector<std::uint8_t> vertexTranslucency = {0U, 1U};
    const opengl::PointTransparencyIndexSplit pointSplit =
        opengl::split_point_indices_by_transparency(pointIndices, positions, vertexTranslucency);
    ASSERT_EQ(2U, pointSplit.opaqueIndices.size());
    ASSERT_EQ(1U, pointSplit.translucentIndices.size());
    EXPECT_EQ(1U, opengl::flatten_translucent_point_indices(pointSplit.translucentIndices).front());

    const std::vector<std::uint32_t> lineIndices = {0U, 1U, 7U};
    const opengl::LineTransparencyIndexSplit lineSplit =
        opengl::split_line_indices_by_transparency(lineIndices, positions, vertexTranslucency);
    EXPECT_TRUE(lineSplit.opaqueIndices.empty());
    ASSERT_EQ(1U, lineSplit.translucentSegments.size());
    const std::vector<std::uint32_t> flatLines =
        opengl::flatten_translucent_line_indices(lineSplit.translucentSegments);
    ASSERT_EQ(2U, flatLines.size());
    EXPECT_EQ(0U, flatLines[0]);
    EXPECT_EQ(1U, flatLines[1]);

    const std::vector<opengl::SortablePointIndex> sortablePoints = {
        {1U, linal::float3{1.0F, 0.0F, 0.0F}},
        {2U, linal::float3{4.0F, 0.0F, 0.0F}},
    };
    const std::vector<std::uint32_t> sortedPoints =
        opengl::sort_translucent_point_indices_back_to_front(sortablePoints, linal::double3{0.0, 0.0, 0.0});
    EXPECT_EQ(2U, sortedPoints.front());

    const std::vector<opengl::SortableLineSegment> sortableLines = {
        {1U, 2U, linal::float3{1.0F, 0.0F, 0.0F}},
        {3U, 4U, linal::float3{4.0F, 0.0F, 0.0F}},
    };
    const std::vector<std::uint32_t> sortedLines =
        opengl::sort_translucent_line_indices_back_to_front(sortableLines, linal::double3{0.0, 0.0, 0.0});
    EXPECT_EQ(3U, sortedLines.front());
}

TEST_F(OpenGLDrawableTest, PointDrawableUpdatesDrawsAndMoveAssigns) {
    opengl::PointProgram program = opengl::make_point_program();
    const std::vector<float> mixedColors = {
        1.0F,
        0.0F,
        0.0F,
        1.0F,
        0.0F,
        1.0F,
        0.0F,
        0.5F,
        0.0F,
        0.0F,
        1.0F,
        1.0F,
    };
    opengl::PointDrawable drawable = make_point_drawable_with_alpha(program, mixedColors);
    EXPECT_TRUE(drawable.has_opaque_primitives());
    EXPECT_TRUE(drawable.has_translucent_primitives());

    const std::vector<float> updatedVertices = {
        -1.0F,
        0.0F,
        0.0F,
        0.0F,
        0.0F,
        0.0F,
        1.0F,
        0.0F,
        0.0F,
    };
    const std::vector<float> opaqueColors = {
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
    const std::vector<std::uint32_t> emptyIndices;
    drawable.update_point_drawable(updatedVertices,
                                   opaqueColors,
                                   emptyIndices,
                                   opengl::BufferAccessPattern::DYNAMIC_DRAW);
    EXPECT_FALSE(drawable.has_opaque_primitives());
    EXPECT_FALSE(drawable.has_translucent_primitives());
    drawable.draw_opaque(matrix(), linal::hmatf::identity());

    const std::vector<std::uint32_t> translucentIndices = {1U};
    drawable.update_point_drawable(updatedVertices,
                                   mixedColors,
                                   translucentIndices,
                                   opengl::BufferAccessPattern::DYNAMIC_DRAW);
    EXPECT_FALSE(drawable.has_opaque_primitives());
    EXPECT_TRUE(drawable.has_translucent_primitives());
    drawable.draw(matrix(), linal::hmatf::identity());
    drawable.draw_translucent(matrix(), linal::hmatf::identity(), linal::double3{0.0, 0.0, 2.0});

    opengl::PointDrawable destination = make_point_drawable_with_alpha(program, opaqueColors);
    destination = std::move(drawable);
    EXPECT_TRUE(destination.has_translucent_primitives());
}

TEST_F(OpenGLDrawableTest, LineDrawableUpdatesDrawsWithoutPointPassAndMoveAssigns) {
    opengl::LineProgram program = opengl::make_line_program();
    const std::vector<float> mixedColors = {
        1.0F,
        0.0F,
        0.0F,
        1.0F,
        1.0F,
        0.0F,
        0.0F,
        1.0F,
        0.0F,
        1.0F,
        0.0F,
        1.0F,
        0.0F,
        1.0F,
        0.0F,
        0.5F,
    };
    opengl::LineDrawable drawable = make_line_drawable_with_alpha(program, mixedColors, 0.0F);
    EXPECT_TRUE(drawable.has_opaque_primitives());
    EXPECT_TRUE(drawable.has_translucent_primitives());

    const std::vector<float> updatedVertices = {
        -1.0F,
        0.0F,
        0.0F,
        0.0F,
        0.0F,
        0.0F,
        1.0F,
        0.0F,
        0.0F,
        2.0F,
        0.0F,
        0.0F,
    };
    const std::vector<float> opaqueColors = {
        1.0F,
        0.0F,
        0.0F,
        1.0F,
        1.0F,
        0.0F,
        0.0F,
        1.0F,
        0.0F,
        1.0F,
        0.0F,
        1.0F,
        0.0F,
        1.0F,
        0.0F,
        1.0F,
    };
    const std::vector<std::uint32_t> emptyIndices;
    drawable.update_line_drawable(updatedVertices,
                                  opaqueColors,
                                  emptyIndices,
                                  opengl::BufferAccessPattern::DYNAMIC_DRAW);
    EXPECT_FALSE(drawable.has_opaque_primitives());
    EXPECT_FALSE(drawable.has_translucent_primitives());
    drawable.draw_opaque(matrix(), linal::hmatf::identity());

    const std::vector<std::uint32_t> translucentIndices = {2U, 3U};
    drawable.update_line_drawable(updatedVertices,
                                  mixedColors,
                                  translucentIndices,
                                  opengl::BufferAccessPattern::DYNAMIC_DRAW);
    EXPECT_FALSE(drawable.has_opaque_primitives());
    EXPECT_TRUE(drawable.has_translucent_primitives());
    drawable.draw(matrix(), linal::hmatf::identity());
    drawable.draw_translucent(matrix(), linal::hmatf::identity(), linal::double3{0.0, 0.0, 2.0});

    opengl::LineDrawable destination = make_line_drawable_with_alpha(program, opaqueColors, 0.0F);
    destination = std::move(drawable);
    EXPECT_TRUE(destination.has_translucent_primitives());
}

TEST_F(OpenGLDrawableTest, MeshDrawableUpdatesDrawsAndMoveAssigns) {
    opengl::MeshProgram program = opengl::make_mesh_program();
    const std::vector<float> opaqueColors = {
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
    const std::vector<float> translucentColors = {
        1.0F,
        0.0F,
        0.0F,
        1.0F,
        0.0F,
        1.0F,
        0.0F,
        0.5F,
        0.0F,
        0.0F,
        1.0F,
        1.0F,
    };

    opengl::MeshDrawable drawable = make_mesh_drawable_with_alpha(program, opaqueColors);
    EXPECT_FALSE(drawable.is_translucent());
    drawable.update_color_buffer(translucentColors, opengl::BufferAccessPattern::DYNAMIC_DRAW);
    EXPECT_TRUE(drawable.is_translucent());
    drawable.draw(matrix(),
                  matrix(),
                  matrix(),
                  matrix(),
                  linal::float3{0.0F, 0.0F, 1.0F},
                  linal::float3{0.0F, 0.0F, 2.0F},
                  linal::float3{1.0F, 1.0F, 1.0F},
                  linal::float3{-0.45F, 0.60F, 0.35F},
                  linal::float3{0.2F, 0.2F, 0.3F},
                  linal::float3{0.1F, 0.1F, 0.1F},
                  8.0F);

    opengl::MeshDrawable destination = make_mesh_drawable_with_alpha(program, opaqueColors);
    destination = std::move(drawable);
    EXPECT_TRUE(destination.is_translucent());
}

TEST_F(OpenGLDrawableTest, DrawablesManagerUpdatesClearsAndDrawsTransparentPointsAndLines) {
    auto manager = opengl::DrawablesManager::create();

    EXPECT_FALSE(manager->has_drawables());
    const std::vector<float> emptyFloatData;
    const std::vector<std::uint32_t> emptyIndices;
    manager->update_last_point_drawable(emptyFloatData,
                                       emptyFloatData,
                                       emptyIndices,
                                       opengl::BufferAccessPattern::DYNAMIC_DRAW);
    manager->update_last_line_drawable(emptyFloatData,
                                      emptyFloatData,
                                      emptyIndices,
                                      opengl::BufferAccessPattern::DYNAMIC_DRAW);

    const std::vector<float> pointVertices = {
        0.0F,
        0.0F,
        0.0F,
        3.0F,
        0.0F,
        0.0F,
        0.0F,
        1.0F,
        0.0F,
    };
    const std::vector<float> pointColors = {
        1.0F,
        0.0F,
        0.0F,
        1.0F,
        0.0F,
        1.0F,
        0.0F,
        0.5F,
        0.0F,
        0.0F,
        1.0F,
        1.0F,
    };
    const std::vector<std::uint32_t> pointIndices = {0U, 1U, 2U};
    const auto firstPointIdOpt = manager->add_point_drawable(pointVertices,
                                                            pointColors,
                                                            pointIndices,
                                                            3.0F,
                                                            opengl::BufferAccessPattern::STATIC_DRAW);
    ASSERT_TRUE(firstPointIdOpt.has_value());
    const opengl::DrawablesManager::DrawableId firstPointId = *firstPointIdOpt;
    const auto removedPointIdOpt = manager->add_point_drawable(pointVertices,
                                                               pointColors,
                                                               pointIndices,
                                                               3.0F,
                                                               opengl::BufferAccessPattern::STATIC_DRAW);
    ASSERT_TRUE(removedPointIdOpt.has_value());
    const opengl::DrawablesManager::DrawableId removedPointId = *removedPointIdOpt;
    const auto lastPointIdOpt = manager->add_point_drawable(pointVertices,
                                                           pointColors,
                                                           pointIndices,
                                                           3.0F,
                                                           opengl::BufferAccessPattern::STATIC_DRAW);
    ASSERT_TRUE(lastPointIdOpt.has_value());
    const opengl::DrawablesManager::DrawableId lastPointId = *lastPointIdOpt;
    EXPECT_TRUE(manager->remove_point_drawable(removedPointId));
    EXPECT_FALSE(manager->remove_point_drawable(removedPointId));
    EXPECT_FALSE(manager->remove_point_drawable(0U));
    EXPECT_TRUE(manager->has_point_drawables());

    const std::vector<float> lineVertices = {
        0.0F,
        0.0F,
        0.0F,
        1.0F,
        0.0F,
        0.0F,
        5.0F,
        0.0F,
        0.0F,
        6.0F,
        0.0F,
        0.0F,
    };
    const std::vector<float> lineColors = {
        1.0F,
        0.0F,
        0.0F,
        1.0F,
        1.0F,
        0.0F,
        0.0F,
        1.0F,
        0.0F,
        1.0F,
        0.0F,
        1.0F,
        0.0F,
        1.0F,
        0.0F,
        0.5F,
    };
    const std::vector<std::uint32_t> lineIndices = {0U, 1U, 2U, 3U};
    const auto removedLineIdOpt = manager->add_line_drawable(lineVertices,
                                                            lineIndices,
                                                            lineColors,
                                                            opengl::LineType::lines(),
                                                            2.0F,
                                                            1.0F,
                                                            opengl::BufferAccessPattern::STATIC_DRAW);
    ASSERT_TRUE(removedLineIdOpt.has_value());
    const opengl::DrawablesManager::DrawableId removedLineId = *removedLineIdOpt;
    const auto lastLineIdOpt = manager->add_line_drawable(lineVertices,
                                                         lineIndices,
                                                         lineColors,
                                                         opengl::LineType::lines(),
                                                         2.0F,
                                                         1.0F,
                                                         opengl::BufferAccessPattern::STATIC_DRAW);
    ASSERT_TRUE(lastLineIdOpt.has_value());
    const opengl::DrawablesManager::DrawableId lastLineId = *lastLineIdOpt;
    EXPECT_TRUE(manager->remove_line_drawable(removedLineId));
    EXPECT_FALSE(manager->remove_line_drawable(removedLineId));
    EXPECT_TRUE(manager->has_line_drawables());

    EXPECT_TRUE(manager->has_drawables());
    EXPECT_TRUE(manager->has_point_drawables());
    EXPECT_TRUE(manager->has_line_drawables());
    EXPECT_FALSE(manager->has_mesh_drawables());

    manager->draw_points(matrix());
    manager->draw_lines(matrix());

    glDepthMask(GL_TRUE);
    manager->draw_lines_and_points(matrix(), linal::double3{0.0, 0.0, 0.0});
    GLboolean depthMask = GL_FALSE;
    glGetBooleanv(GL_DEPTH_WRITEMASK, &depthMask);
    EXPECT_EQ(GL_TRUE, depthMask);

    manager->update_last_point_drawable(pointVertices,
                                       pointColors,
                                       emptyIndices,
                                       opengl::BufferAccessPattern::DYNAMIC_DRAW);
    manager->update_last_line_drawable(lineVertices,
                                      lineColors,
                                      emptyIndices,
                                      opengl::BufferAccessPattern::DYNAMIC_DRAW);
    manager->clear_point_drawables();
    manager->clear_line_drawables();
    EXPECT_FALSE(manager->remove_point_drawable(firstPointId));
    EXPECT_FALSE(manager->remove_point_drawable(lastPointId));
    EXPECT_FALSE(manager->remove_line_drawable(lastLineId));
    EXPECT_FALSE(manager->has_drawables());
}

TEST_F(OpenGLDrawableTest, DrawablesManagerDrawsTransparentMeshesAndRestoresCullFaceAndDepthMask) {
    auto manager = opengl::DrawablesManager::create();

    const std::vector<float> opaqueColors = {
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
    const std::vector<float> translucentColors = {
        1.0F,
        0.0F,
        0.0F,
        1.0F,
        0.0F,
        1.0F,
        0.0F,
        0.5F,
        0.0F,
        0.0F,
        1.0F,
        1.0F,
    };
    const opengl::DrawablesManager::DrawableId removedMeshId = add_mesh_drawable_with_alpha(*manager, opaqueColors);
    const opengl::DrawablesManager::DrawableId lastMeshId = add_mesh_drawable_with_alpha(*manager, translucentColors);
    EXPECT_TRUE(manager->remove_mesh_drawable(removedMeshId));
    EXPECT_FALSE(manager->remove_mesh_drawable(removedMeshId));
    EXPECT_TRUE(manager->has_mesh_drawables());

    glEnable(GL_CULL_FACE);
    glDepthMask(GL_TRUE);
    opengl::LightingConfig lighting;
    lighting.lightPosition = linal::float3{0.0F, 0.0F, 1.0F};
    lighting.lightColor = linal::float3{1.0F, 1.0F, 1.0F};
    lighting.fillLightDir = linal::float3{-0.45F, 0.60F, 0.35F};
    lighting.fillLightColor = linal::float3{0.2F, 0.2F, 0.3F};
    lighting.ambientColor = linal::float3{0.1F, 0.1F, 0.1F};
    lighting.shininess = 8.0F;
    manager->draw_meshes(matrix(), matrix(), linal::float3{0.0F, 0.0F, 0.0F}, lighting);

    GLboolean depthMask = GL_FALSE;
    glGetBooleanv(GL_DEPTH_WRITEMASK, &depthMask);
    EXPECT_EQ(GL_TRUE, depthMask);
    EXPECT_EQ(GL_TRUE, glIsEnabled(GL_CULL_FACE));

    manager->clear_mesh_drawables();
    EXPECT_FALSE(manager->remove_mesh_drawable(lastMeshId));
    EXPECT_FALSE(manager->has_mesh_drawables());
    manager->clear_drawables();
    EXPECT_FALSE(manager->has_drawables());
}

TEST_F(OpenGLDrawableTest, PointDrawableExposesVertexPositions) {
    opengl::PointProgram program = opengl::make_point_program();
    const std::vector<float> colors = {
        1.0F, 0.0F, 0.0F, 1.0F, 0.0F, 1.0F, 0.0F, 1.0F, 0.0F, 0.0F, 1.0F, 1.0F,
    };
    opengl::PointDrawable drawable = make_point_drawable_with_alpha(program, colors);

    const std::span<const float> positions = drawable.get_vertex_positions();
    const std::vector<float> expected = {0.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 0.0F, 1.0F, 0.0F};
    ASSERT_EQ(expected.size(), positions.size());
    for (std::size_t i = 0; i < expected.size(); ++i) {
        EXPECT_FLOAT_EQ(expected[i], positions[i]);
    }
}

TEST_F(OpenGLDrawableTest, LineDrawableExposesVertexPositions) {
    opengl::LineProgram program = opengl::make_line_program();
    const std::vector<float> colors = {
        1.0F, 0.0F, 0.0F, 1.0F, 1.0F, 0.0F, 0.0F, 1.0F, 0.0F, 1.0F, 0.0F, 1.0F, 0.0F, 1.0F, 0.0F, 1.0F,
    };
    opengl::LineDrawable drawable = make_line_drawable_with_alpha(program, colors, 0.0F);

    const std::span<const float> positions = drawable.get_vertex_positions();
    const std::vector<float> expected = {0.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 0.0F, 1.0F, 0.0F, 1.0F, 1.0F, 0.0F};
    ASSERT_EQ(expected.size(), positions.size());
    for (std::size_t i = 0; i < expected.size(); ++i) {
        EXPECT_FLOAT_EQ(expected[i], positions[i]);
    }
}

TEST_F(OpenGLDrawableTest, MeshDrawableExposesVertexPositions) {
    opengl::MeshProgram program = opengl::make_mesh_program();
    const std::vector<float> colors = {
        1.0F, 0.0F, 0.0F, 1.0F, 0.0F, 1.0F, 0.0F, 1.0F, 0.0F, 0.0F, 1.0F, 1.0F,
    };
    opengl::MeshDrawable drawable = make_mesh_drawable_with_alpha(program, colors);

    const std::span<const float> positions = drawable.get_vertex_positions();
    const std::vector<float> expected = {0.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 0.0F, 1.0F, 0.0F};
    ASSERT_EQ(expected.size(), positions.size());
    for (std::size_t i = 0; i < expected.size(); ++i) {
        EXPECT_FLOAT_EQ(expected[i], positions[i]);
    }
}

TEST_F(OpenGLDrawableTest, DrawablesManagerCollectsVertexPositionBuffersAcrossAllDrawableKinds) {
    auto manager = opengl::DrawablesManager::create();

    const std::vector<float> pointVertices = {0.0F, 0.0F, 0.0F, 3.0F, 0.0F, 0.0F, 0.0F, 1.0F, 0.0F};
    const std::vector<float> pointColors = {
        1.0F, 0.0F, 0.0F, 1.0F, 0.0F, 1.0F, 0.0F, 1.0F, 0.0F, 0.0F, 1.0F, 1.0F,
    };
    const std::vector<std::uint32_t> pointIndices = {0U, 1U, 2U};
    ASSERT_TRUE(manager
                    ->add_point_drawable(pointVertices,
                                       pointColors,
                                       pointIndices,
                                       3.0F,
                                       opengl::BufferAccessPattern::STATIC_DRAW)
                    .has_value());

    const std::vector<float> lineVertices = {0.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 5.0F, 0.0F, 0.0F, 6.0F, 0.0F, 0.0F};
    const std::vector<float> lineColors = {
        1.0F, 0.0F, 0.0F, 1.0F, 1.0F, 0.0F, 0.0F, 1.0F, 0.0F, 1.0F, 0.0F, 1.0F, 0.0F, 1.0F, 0.0F, 1.0F,
    };
    const std::vector<std::uint32_t> lineIndices = {0U, 1U, 2U, 3U};
    ASSERT_TRUE(manager
                    ->add_line_drawable(lineVertices,
                                       lineIndices,
                                       lineColors,
                                       opengl::LineType::lines(),
                                       2.0F,
                                       1.0F,
                                       opengl::BufferAccessPattern::STATIC_DRAW)
                    .has_value());

    const std::vector<float> meshColors = {
        1.0F, 0.0F, 0.0F, 1.0F, 0.0F, 1.0F, 0.0F, 1.0F, 0.0F, 0.0F, 1.0F, 1.0F,
    };
    add_mesh_drawable_with_alpha(*manager, meshColors); // hardcoded 3 vertices (9 floats), see the helper above

    const auto buffers = manager->collect_vertex_position_buffers();
    ASSERT_EQ(3U, buffers.size());
    std::size_t totalFloats = 0;
    for (const auto& buffer: buffers) {
        totalFloats += buffer.size();
    }
    constexpr std::size_t meshVertexFloatCount = 9U;
    EXPECT_EQ(pointVertices.size() + lineVertices.size() + meshVertexFloatCount, totalFloats);
}

TEST_F(OpenGLDrawableTest, DrawablesManagerCollectsEmptyWhenNoDrawablesAdded) {
    auto manager = opengl::DrawablesManager::create();
    EXPECT_TRUE(manager->collect_vertex_position_buffers().empty());
}

TEST_F(OpenGLDrawableTest, PointAndLineDrawableDrawWithNonIdentityModelMatrix) {
    opengl::PointProgram pointProgram = opengl::make_point_program();
    opengl::LineProgram lineProgram = opengl::make_line_program();
    const std::vector<float> pointColors = {
        1.0F, 0.0F, 0.0F, 1.0F, 0.0F, 1.0F, 0.0F, 0.5F, 0.0F, 0.0F, 1.0F, 1.0F,
    };
    const std::vector<float> lineColors = {
        1.0F, 0.0F, 0.0F, 1.0F, 1.0F, 0.0F, 0.0F, 1.0F, 0.0F, 1.0F, 0.0F, 1.0F, 0.0F, 1.0F, 0.0F, 0.5F,
    };

    opengl::PointDrawable pointDrawable = make_point_drawable_with_alpha(pointProgram, pointColors);
    opengl::LineDrawable lineDrawable = make_line_drawable_with_alpha(lineProgram, lineColors, 0.0F);

    const linal::hmatf translation = make_translation(1.0F, 2.0F, 3.0F);

    // Smoke test only: confirm passing a real, non-identity model matrix through the whole
    // draw path doesn't crash/assert.
    pointDrawable.draw(matrix(), translation);
    pointDrawable.draw_opaque(matrix(), translation);
    pointDrawable.draw_translucent(matrix(), translation, linal::double3{0.0, 0.0, 5.0});

    lineDrawable.draw(matrix(), translation);
    lineDrawable.draw_opaque(matrix(), translation);
    lineDrawable.draw_translucent(matrix(), translation, linal::double3{0.0, 0.0, 5.0});
}

TEST_F(OpenGLDrawableTest, MeshDrawableDrawsWithNonIdentityModelMatrix) {
    opengl::MeshProgram program = opengl::make_mesh_program();
    const std::vector<float> colors = {
        1.0F, 0.0F, 0.0F, 1.0F, 0.0F, 1.0F, 0.0F, 1.0F, 0.0F, 0.0F, 1.0F, 1.0F,
    };
    opengl::MeshDrawable drawable = make_mesh_drawable_with_alpha(program, colors);

    const linal::hmatf translation = make_translation(1.0F, 2.0F, 3.0F);
    drawable.draw(translation,
                  matrix(),
                  matrix(),
                  translation,
                  linal::float3{0.0F, 0.0F, 1.0F},
                  linal::float3{0.0F, 0.0F, 2.0F},
                  linal::float3{1.0F, 1.0F, 1.0F},
                  linal::float3{-0.45F, 0.60F, 0.35F},
                  linal::float3{0.2F, 0.2F, 0.3F},
                  linal::float3{0.1F, 0.1F, 0.1F},
                  8.0F);
}

TEST(DrawableTransparencyInfoTransformTest, DistanceSquaredToTransformsSortCenterBeforeComparing) {
    const opengl::DrawableTransparencyInfo info{true, linal::float3{1.0F, 0.0F, 0.0F}};
    const linal::hmatf translation = make_translation(10.0F, 0.0F, 0.0F);

    // Untransformed: sortCenter is (1,0,0); transformed by `translation`, it becomes (11,0,0).
    const linal::double3 viewPosition{0.0, 0.0, 0.0};
    const double expected = 11.0 * 11.0;
    EXPECT_DOUBLE_EQ(expected, info.distance_squared_to(viewPosition, translation));
    // Sanity: the untransformed overload should NOT match the transformed distance.
    EXPECT_NE(expected, info.distance_squared_to(viewPosition));
}

TEST_F(OpenGLDrawableTest, DrawablesManagerTransformAccessorsRoundTripAndRejectUnknownIds) {
    auto manager = opengl::DrawablesManager::create();

    const std::vector<float> pointVertices = {0.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 0.0F, 1.0F, 0.0F};
    const std::vector<float> pointColors = {
        1.0F, 0.0F, 0.0F, 1.0F, 0.0F, 1.0F, 0.0F, 1.0F, 0.0F, 0.0F, 1.0F, 1.0F,
    };
    const std::vector<std::uint32_t> pointIndices = {0U, 1U, 2U};
    const auto pointIdOpt = manager->add_point_drawable(pointVertices,
                                                       pointColors,
                                                       pointIndices,
                                                       3.0F,
                                                       opengl::BufferAccessPattern::STATIC_DRAW);
    ASSERT_TRUE(pointIdOpt.has_value());
    const opengl::DrawablesManager::DrawableId pointId = *pointIdOpt;

    // Landmine test: a freshly-added drawable's transform must read back as identity, not a
    // zero-initialized matrix.
    const std::optional<linal::hmatf> initialTransform = manager->get_point_drawable_transform(pointId);
    ASSERT_TRUE(initialTransform.has_value());
    EXPECT_TRUE(initialTransform->is_identity());

    const linal::hmatf translation = make_translation(1.0F, 2.0F, 3.0F);
    EXPECT_TRUE(manager->set_point_drawable_transform(pointId, translation));
    const std::optional<linal::hmatf> roundTripped = manager->get_point_drawable_transform(pointId);
    ASSERT_TRUE(roundTripped.has_value());
    for (int i = 0; i < linal::hmatf::size; ++i) {
        EXPECT_FLOAT_EQ(translation[i], (*roundTripped)[i]);
    }

    constexpr opengl::DrawablesManager::DrawableId unknownId = 999999U;
    EXPECT_FALSE(manager->set_point_drawable_transform(unknownId, translation));
    EXPECT_FALSE(manager->get_point_drawable_transform(unknownId).has_value());

    const std::vector<float> lineVertices = {0.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 5.0F, 0.0F, 0.0F, 6.0F, 0.0F, 0.0F};
    const std::vector<float> lineColors = {
        1.0F, 0.0F, 0.0F, 1.0F, 1.0F, 0.0F, 0.0F, 1.0F, 0.0F, 1.0F, 0.0F, 1.0F, 0.0F, 1.0F, 0.0F, 1.0F,
    };
    const std::vector<std::uint32_t> lineIndices = {0U, 1U, 2U, 3U};
    const auto lineIdOpt = manager->add_line_drawable(lineVertices,
                                                     lineIndices,
                                                     lineColors,
                                                     opengl::LineType::lines(),
                                                     2.0F,
                                                     1.0F,
                                                     opengl::BufferAccessPattern::STATIC_DRAW);
    ASSERT_TRUE(lineIdOpt.has_value());
    const opengl::DrawablesManager::DrawableId lineId = *lineIdOpt;
    EXPECT_TRUE(manager->get_line_drawable_transform(lineId)->is_identity());
    EXPECT_TRUE(manager->set_line_drawable_transform(lineId, translation));
    EXPECT_FALSE(manager->get_line_drawable_transform(lineId)->is_identity());

    const std::vector<float> meshColors = {
        1.0F, 0.0F, 0.0F, 1.0F, 0.0F, 1.0F, 0.0F, 1.0F, 0.0F, 0.0F, 1.0F, 1.0F,
    };
    const opengl::DrawablesManager::DrawableId meshId = add_mesh_drawable_with_alpha(*manager, meshColors);
    EXPECT_TRUE(manager->get_mesh_drawable_transform(meshId)->is_identity());
    EXPECT_TRUE(manager->set_mesh_drawable_transform(meshId, translation));
    EXPECT_FALSE(manager->get_mesh_drawable_transform(meshId)->is_identity());

    // Overlay kinds (meshSegment/meshVertex) share the same DrawableEntry template - verify at
    // least one representative each to confirm they're wired up too.
    const std::vector<float> segmentPositions = {0.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F};
    const std::vector<std::uint32_t> segmentIndices = {0U, 1U};
    const std::vector<float> segmentColor = {1.0F, 1.0F, 1.0F, 1.0F};
    const auto segmentIdOpt = manager->add_mesh_segment_drawable(segmentPositions, segmentIndices, segmentColor, 1.0F);
    ASSERT_TRUE(segmentIdOpt.has_value());
    const opengl::DrawablesManager::DrawableId segmentId = *segmentIdOpt;
    EXPECT_TRUE(manager->get_mesh_segment_drawable_transform(segmentId)->is_identity());
    EXPECT_TRUE(manager->set_mesh_segment_drawable_transform(segmentId, translation));
    EXPECT_FALSE(manager->get_mesh_segment_drawable_transform(segmentId)->is_identity());

    const std::vector<float> vertexPositions = {0.0F, 0.0F, 0.0F};
    const std::vector<float> vertexColor = {1.0F, 1.0F, 1.0F, 1.0F};
    const auto vertexIdOpt = manager->add_mesh_vertex_drawable(vertexPositions, vertexColor, 2.0F);
    ASSERT_TRUE(vertexIdOpt.has_value());
    const opengl::DrawablesManager::DrawableId vertexId = *vertexIdOpt;
    EXPECT_TRUE(manager->get_mesh_vertex_drawable_transform(vertexId)->is_identity());
    EXPECT_TRUE(manager->set_mesh_vertex_drawable_transform(vertexId, translation));
    EXPECT_FALSE(manager->get_mesh_vertex_drawable_transform(vertexId)->is_identity());
}

TEST_F(OpenGLDrawableTest, CollectVertexPositionBuffersAppliesNonIdentityTransform) {
    auto manager = opengl::DrawablesManager::create();

    const std::vector<float> pointVertices = {0.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F};
    const std::vector<float> pointColors = {1.0F, 0.0F, 0.0F, 1.0F, 0.0F, 1.0F, 0.0F, 1.0F};
    const std::vector<std::uint32_t> pointIndices = {0U, 1U};
    const auto pointIdOpt = manager->add_point_drawable(pointVertices,
                                                       pointColors,
                                                       pointIndices,
                                                       3.0F,
                                                       opengl::BufferAccessPattern::STATIC_DRAW);
    ASSERT_TRUE(pointIdOpt.has_value());

    const linal::hmatf translation = make_translation(10.0F, 20.0F, 30.0F);
    ASSERT_TRUE(manager->set_point_drawable_transform(*pointIdOpt, translation));

    const auto buffers = manager->collect_vertex_position_buffers();
    ASSERT_EQ(1U, buffers.size());
    ASSERT_EQ(6U, buffers.front().size());
    const std::vector<float> expected = {10.0F, 20.0F, 30.0F, 11.0F, 20.0F, 30.0F};
    for (std::size_t i = 0; i < expected.size(); ++i) {
        EXPECT_FLOAT_EQ(expected[i], buffers.front()[i]);
    }
}
