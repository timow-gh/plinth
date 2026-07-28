#include "plinth/loader/StlLoader.hpp"
#include "plinth/loader/MeshLoader.hpp"
#include <array>
#include <cstdint>
#include <cstring>
#include <gtest/gtest.h>
#include <string>
#include <string_view>

namespace {

using renderer::LoadError;
using renderer::StlLoader;

void append_float(std::string& out, float value) {
    std::array<char, sizeof(float)> bytes{};
    std::memcpy(bytes.data(), &value, sizeof(float));
    out.append(bytes.data(), bytes.size());
}

void append_uint32(std::string& out, std::uint32_t value) {
    std::array<char, sizeof(std::uint32_t)> bytes{};
    std::memcpy(bytes.data(), &value, sizeof(std::uint32_t));
    out.append(bytes.data(), bytes.size());
}

// Builds a minimal single-triangle binary STL blob.
std::string make_binary_stl_one_triangle() {
    std::string blob(80, '\0'); // 80-byte header
    append_uint32(blob, 1U);    // triangle count
    // normal
    append_float(blob, 0.0F);
    append_float(blob, 0.0F);
    append_float(blob, 1.0F);
    // three vertices
    const std::array<std::array<float, 3>, 3> verts{{{0.0F, 0.0F, 0.0F}, {1.0F, 0.0F, 0.0F}, {0.0F, 1.0F, 0.0F}}};
    for (const auto& v : verts) {
        append_float(blob, v[0]);
        append_float(blob, v[1]);
        append_float(blob, v[2]);
    }
    blob.append(2, '\0'); // attribute byte count
    return blob;
}

TEST(StlLoaderTest, ParsesBinarySingleTriangle) {
    const std::string blob = make_binary_stl_one_triangle();
    const auto mesh = StlLoader{}.parse(blob);
    ASSERT_TRUE(mesh.has_value());
    EXPECT_EQ(mesh->vertices.size(), 3U * 3U);
    ASSERT_EQ(mesh->normals.size(), 3U * 3U);
    ASSERT_EQ(mesh->triangleIndices.size(), 3U);
    // Facet normal replicated to each vertex.
    EXPECT_FLOAT_EQ(mesh->normals[2], 1.0F);
    EXPECT_FLOAT_EQ(mesh->normals[5], 1.0F);
    EXPECT_FLOAT_EQ(mesh->normals[8], 1.0F);
    // Sequential indices.
    EXPECT_EQ(mesh->triangleIndices[0], 0U);
    EXPECT_EQ(mesh->triangleIndices[1], 1U);
    EXPECT_EQ(mesh->triangleIndices[2], 2U);
}

TEST(StlLoaderTest, ParsesAsciiSingleTriangle) {
    constexpr std::string_view stl = "solid test\n"
                                     "  facet normal 0 0 1\n"
                                     "    outer loop\n"
                                     "      vertex 0 0 0\n"
                                     "      vertex 1 0 0\n"
                                     "      vertex 0 1 0\n"
                                     "    endloop\n"
                                     "  endfacet\n"
                                     "endsolid test\n";
    const auto mesh = StlLoader{}.parse(stl);
    ASSERT_TRUE(mesh.has_value());
    EXPECT_EQ(mesh->vertices.size(), 3U * 3U);
    ASSERT_EQ(mesh->normals.size(), 3U * 3U);
    EXPECT_EQ(mesh->triangleIndices.size(), 3U);
    EXPECT_FLOAT_EQ(mesh->normals[2], 1.0F);
}

TEST(StlLoaderTest, EmptyInputIsEmptyError) {
    const auto mesh = StlLoader{}.parse("");
    ASSERT_FALSE(mesh.has_value());
    EXPECT_EQ(mesh.error(), LoadError::empty);
}

TEST(StlLoaderTest, RejectsAsciiWithMalformedVertex) {
    constexpr std::string_view stl = "solid test\n"
                                     "  facet normal 0 0 1\n"
                                     "    outer loop\n"
                                     "      vertex 0 0\n" // missing component
                                     "    endloop\n"
                                     "  endfacet\n"
                                     "endsolid test\n";
    const auto mesh = StlLoader{}.parse(stl);
    ASSERT_FALSE(mesh.has_value());
    EXPECT_EQ(mesh.error(), LoadError::parseError);
}

} // namespace
