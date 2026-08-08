#include "OpenGL/Drawable/DrawablesManager.hpp"
#include "OpenGL/Drawable/SphereImpostorDrawable.hpp"
#include "OpenGL/Drawable/SphereInstanceData.hpp"
#include "OpenGL/OpenGL.hpp"
#include "OpenGL/Programs/ProgramManager.hpp"

#include <GLFW/glfw3.h>
#include <array>
#include <gtest/gtest.h>
#include <optional>
#include <span>
#include <vector>

namespace {

void* load_glfw_proc(const char* procName) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    return reinterpret_cast<void*>(glfwGetProcAddress(procName));
}

class SphereImpostorDrawableTest : public ::testing::Test {
  protected:
    static void SetUpTestSuite() { ASSERT_EQ(GLFW_TRUE, glfwInit()); }
    static void TearDownTestSuite() { glfwTerminate(); }

    void SetUp() override {
        glfwDefaultWindowHints();
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        m_window = glfwCreateWindow(64, 64, "sphere impostor drawable test", nullptr, nullptr);
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

// Three unit spheres arranged along the x-axis.
const std::vector<float> kCenters{0.0F, 0.0F, 0.0F,  1.0F, 0.0F, 0.0F,  2.0F, 0.0F, 0.0F};
const std::vector<float> kRadii{0.5F, 0.5F, 0.5F};
const std::vector<float> kOpaqueColors{1.0F, 0.0F, 0.0F, 1.0F,  0.0F, 1.0F, 0.0F, 1.0F,  0.0F, 0.0F, 1.0F, 1.0F};
const std::vector<float> kTranslucentColors{1.0F, 0.0F, 0.0F, 0.5F,  0.0F, 1.0F, 0.0F, 0.5F,  0.0F, 0.0F, 1.0F, 0.5F};
const std::vector<float> kMixedColors{1.0F, 0.0F, 0.0F, 1.0F,  0.0F, 1.0F, 0.0F, 0.5F,  0.0F, 0.0F, 1.0F, 1.0F};

// ──────────────────────────────────────────────────────────────────────────────
// make_sphere_instance_data (no GL required)
// ──────────────────────────────────────────────────────────────────────────────

TEST(SphereInstanceDataTest, ValidInputProducesCorrectLayout) {
    const auto result = opengl::make_sphere_instance_data(kCenters, kRadii, kOpaqueColors);
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(3U * opengl::kSphereInstanceFloats, result->size());

    // Sphere 0: cx,cy,cz,r, r,g,b,a
    EXPECT_FLOAT_EQ(0.0F, (*result)[0]);
    EXPECT_FLOAT_EQ(0.0F, (*result)[1]);
    EXPECT_FLOAT_EQ(0.0F, (*result)[2]);
    EXPECT_FLOAT_EQ(0.5F, (*result)[3]);
    EXPECT_FLOAT_EQ(1.0F, (*result)[4]);
    EXPECT_FLOAT_EQ(0.0F, (*result)[5]);
    EXPECT_FLOAT_EQ(0.0F, (*result)[6]);
    EXPECT_FLOAT_EQ(1.0F, (*result)[7]);
}

TEST(SphereInstanceDataTest, MismatchedCenterSizeReturnsNullopt) {
    // 4 floats is not a multiple of 3 xyz triples
    const std::vector<float> badCenters{0.0F, 0.0F, 0.0F, 1.0F};
    const auto result = opengl::make_sphere_instance_data(badCenters, kRadii, kOpaqueColors);
    EXPECT_FALSE(result.has_value());
}

TEST(SphereInstanceDataTest, MismatchedCountReturnsNullopt) {
    const std::vector<float> twoRadii{0.5F, 0.5F};
    const auto result = opengl::make_sphere_instance_data(kCenters, twoRadii, kOpaqueColors);
    EXPECT_FALSE(result.has_value());
}

TEST(SphereInstanceDataTest, EmptyCentersReturnsNullopt) {
    const auto result = opengl::make_sphere_instance_data({}, {}, {});
    EXPECT_FALSE(result.has_value());
}

// ──────────────────────────────────────────────────────────────────────────────
// make_sphere_impostor_drawable (requires GL context)
// ──────────────────────────────────────────────────────────────────────────────

TEST_F(SphereImpostorDrawableTest, ValidInputReturnsDrawable) {
    opengl::ProgramManager mgr;
    mgr.compile();
    ASSERT_TRUE(mgr.is_compiled());

    auto drawable = opengl::make_sphere_impostor_drawable(
        mgr.get_sphere_impostor_program(), kCenters, kRadii, kOpaqueColors, opengl::BufferAccessPattern::Static);
    EXPECT_TRUE(drawable.has_value());
}

TEST_F(SphereImpostorDrawableTest, MismatchedSizesReturnsNullopt) {
    opengl::ProgramManager mgr;
    mgr.compile();

    const std::vector<float> twoRadii{0.5F, 0.5F};
    auto drawable = opengl::make_sphere_impostor_drawable(
        mgr.get_sphere_impostor_program(), kCenters, twoRadii, kOpaqueColors, opengl::BufferAccessPattern::Static);
    EXPECT_FALSE(drawable.has_value());
}

TEST_F(SphereImpostorDrawableTest, OpaqueColorsHasOpaqueOnly) {
    opengl::ProgramManager mgr;
    mgr.compile();

    auto drawable = opengl::make_sphere_impostor_drawable(
        mgr.get_sphere_impostor_program(), kCenters, kRadii, kOpaqueColors, opengl::BufferAccessPattern::Static);
    ASSERT_TRUE(drawable.has_value());
    EXPECT_TRUE(drawable->has_opaque_primitives());
    EXPECT_FALSE(drawable->has_translucent_primitives());
    EXPECT_FALSE(drawable->is_translucent());
}

TEST_F(SphereImpostorDrawableTest, TranslucentColorsHasTranslucentOnly) {
    opengl::ProgramManager mgr;
    mgr.compile();

    auto drawable = opengl::make_sphere_impostor_drawable(
        mgr.get_sphere_impostor_program(), kCenters, kRadii, kTranslucentColors, opengl::BufferAccessPattern::Static);
    ASSERT_TRUE(drawable.has_value());
    EXPECT_FALSE(drawable->has_opaque_primitives());
    EXPECT_TRUE(drawable->has_translucent_primitives());
    EXPECT_TRUE(drawable->is_translucent());
}

TEST_F(SphereImpostorDrawableTest, MixedColorsHasBoth) {
    opengl::ProgramManager mgr;
    mgr.compile();

    auto drawable = opengl::make_sphere_impostor_drawable(
        mgr.get_sphere_impostor_program(), kCenters, kRadii, kMixedColors, opengl::BufferAccessPattern::Static);
    ASSERT_TRUE(drawable.has_value());
    EXPECT_TRUE(drawable->has_opaque_primitives());
    EXPECT_TRUE(drawable->has_translucent_primitives());
}

TEST_F(SphereImpostorDrawableTest, GetVertexPositionsMatchesCenters) {
    opengl::ProgramManager mgr;
    mgr.compile();

    auto drawable = opengl::make_sphere_impostor_drawable(
        mgr.get_sphere_impostor_program(), kCenters, kRadii, kOpaqueColors, opengl::BufferAccessPattern::Static);
    ASSERT_TRUE(drawable.has_value());
    const auto positions = drawable->get_vertex_positions();
    ASSERT_EQ(kCenters.size(), positions.size());
    for (std::size_t i = 0; i < kCenters.size(); ++i) {
        EXPECT_FLOAT_EQ(kCenters[i], positions[i]);
    }
}

TEST_F(SphereImpostorDrawableTest, DrawOpaqueDoesNotCrash) {
    opengl::ProgramManager mgr;
    mgr.compile();

    auto drawable = opengl::make_sphere_impostor_drawable(
        mgr.get_sphere_impostor_program(), kCenters, kRadii, kOpaqueColors, opengl::BufferAccessPattern::Static);
    ASSERT_TRUE(drawable.has_value());

    const linal::hmatf identity = linal::hmatf::identity();
    const linal::float2 viewport{64.0F, 64.0F};
    const renderer::LightingConfig lighting;
    drawable->draw_opaque(identity, identity, identity, identity, viewport, false, lighting);
    EXPECT_EQ(GL_NO_ERROR, glGetError());
}

TEST_F(SphereImpostorDrawableTest, DrawPickDoesNotCrash) {
    opengl::ProgramManager mgr;
    mgr.compile();

    auto drawable = opengl::make_sphere_impostor_drawable(
        mgr.get_sphere_impostor_program(), kCenters, kRadii, kOpaqueColors, opengl::BufferAccessPattern::Static);
    ASSERT_TRUE(drawable.has_value());

    const linal::hmatf identity = linal::hmatf::identity();
    const linal::float2 viewport{64.0F, 64.0F};
    drawable->draw_pick(identity, identity, identity, identity, viewport, false, {1.0F, 0.0F, 0.0F});
    EXPECT_EQ(GL_NO_ERROR, glGetError());
}

TEST_F(SphereImpostorDrawableTest, DistanceSquaredToIsCorrect) {
    opengl::ProgramManager mgr;
    mgr.compile();

    // Single sphere at origin.
    const std::vector<float> center{0.0F, 0.0F, 0.0F};
    const std::vector<float> radius{1.0F};
    const std::vector<float> color{1.0F, 1.0F, 1.0F, 1.0F};
    auto drawable = opengl::make_sphere_impostor_drawable(
        mgr.get_sphere_impostor_program(), center, radius, color, opengl::BufferAccessPattern::Static);
    ASSERT_TRUE(drawable.has_value());

    const linal::double3 viewPos{0.0, 0.0, 3.0};
    EXPECT_DOUBLE_EQ(9.0, drawable->distance_squared_to(viewPos));
}

} // namespace
