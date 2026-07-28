#include "plinth/loader/ObjLoader.hpp"
#include "plinth/loader/MeshLoader.hpp"
#include <gtest/gtest.h>
#include <string_view>

namespace {

using renderer::LoadError;
using renderer::ObjLoader;

TEST(ObjLoaderTest, ParsesTriangleWithoutNormals) {
    constexpr std::string_view obj = "v 0 0 0\n"
                                     "v 1 0 0\n"
                                     "v 0 1 0\n"
                                     "f 1 2 3\n";
    const auto mesh = ObjLoader{}.parse(obj);
    ASSERT_TRUE(mesh.has_value());
    EXPECT_EQ(mesh->vertices.size(), 3U * 3U);
    EXPECT_EQ(mesh->triangleIndices.size(), 3U);
    // No vn directives => normals left empty so the renderer computes them.
    EXPECT_TRUE(mesh->normals.empty());
    // 1-based OBJ indices map to 0-based.
    EXPECT_EQ(mesh->triangleIndices[0], 0U);
    EXPECT_EQ(mesh->triangleIndices[1], 1U);
    EXPECT_EQ(mesh->triangleIndices[2], 2U);
}

TEST(ObjLoaderTest, TriangulatesQuadWithFan) {
    constexpr std::string_view obj = "v 0 0 0\n"
                                     "v 1 0 0\n"
                                     "v 1 1 0\n"
                                     "v 0 1 0\n"
                                     "f 1 2 3 4\n";
    const auto mesh = ObjLoader{}.parse(obj);
    ASSERT_TRUE(mesh.has_value());
    EXPECT_EQ(mesh->vertices.size(), 4U * 3U);
    // A quad fans into two triangles => 6 indices.
    ASSERT_EQ(mesh->triangleIndices.size(), 6U);
    EXPECT_EQ(mesh->triangleIndices[0], 0U);
    EXPECT_EQ(mesh->triangleIndices[1], 1U);
    EXPECT_EQ(mesh->triangleIndices[2], 2U);
    EXPECT_EQ(mesh->triangleIndices[3], 0U);
    EXPECT_EQ(mesh->triangleIndices[4], 2U);
    EXPECT_EQ(mesh->triangleIndices[5], 3U);
}

TEST(ObjLoaderTest, HandlesNormalsAndVDoubleSlashVn) {
    constexpr std::string_view obj = "v 0 0 0\n"
                                     "v 1 0 0\n"
                                     "v 0 1 0\n"
                                     "vn 0 0 1\n"
                                     "f 1//1 2//1 3//1\n";
    const auto mesh = ObjLoader{}.parse(obj);
    ASSERT_TRUE(mesh.has_value());
    ASSERT_EQ(mesh->normals.size(), 3U * 3U);
    EXPECT_FLOAT_EQ(mesh->normals[2], 1.0F);
    EXPECT_FLOAT_EQ(mesh->normals[5], 1.0F);
    EXPECT_FLOAT_EQ(mesh->normals[8], 1.0F);
}

TEST(ObjLoaderTest, DeduplicatesSharedCorners) {
    // Two triangles sharing an edge (v//vn identical) should share vertices.
    constexpr std::string_view obj = "v 0 0 0\n"
                                     "v 1 0 0\n"
                                     "v 1 1 0\n"
                                     "v 0 1 0\n"
                                     "vn 0 0 1\n"
                                     "f 1//1 2//1 3//1\n"
                                     "f 1//1 3//1 4//1\n";
    const auto mesh = ObjLoader{}.parse(obj);
    ASSERT_TRUE(mesh.has_value());
    // 4 unique (position, normal) corners, 2 triangles => 6 indices.
    EXPECT_EQ(mesh->vertices.size(), 4U * 3U);
    EXPECT_EQ(mesh->triangleIndices.size(), 6U);
}

TEST(ObjLoaderTest, IgnoresCommentsAndUnknownDirectives) {
    constexpr std::string_view obj = "# a comment\n"
                                     "mtllib scene.mtl\n"
                                     "o object\n"
                                     "v 0 0 0\n"
                                     "usemtl red\n"
                                     "v 1 0 0\n"
                                     "s off\n"
                                     "v 0 1 0\n"
                                     "f 1 2 3\n";
    const auto mesh = ObjLoader{}.parse(obj);
    ASSERT_TRUE(mesh.has_value());
    EXPECT_EQ(mesh->vertices.size(), 3U * 3U);
    EXPECT_EQ(mesh->triangleIndices.size(), 3U);
}

TEST(ObjLoaderTest, RejectsMalformedVertex) {
    constexpr std::string_view obj = "v 0 0\n" // missing z component
                                     "f 1 1 1\n";
    const auto mesh = ObjLoader{}.parse(obj);
    ASSERT_FALSE(mesh.has_value());
    EXPECT_EQ(mesh.error(), LoadError::parseError);
}

TEST(ObjLoaderTest, RejectsFaceWithTooFewCorners) {
    constexpr std::string_view obj = "v 0 0 0\n"
                                     "v 1 0 0\n"
                                     "f 1 2\n";
    const auto mesh = ObjLoader{}.parse(obj);
    ASSERT_FALSE(mesh.has_value());
    EXPECT_EQ(mesh.error(), LoadError::parseError);
}

TEST(ObjLoaderTest, EmptyInputIsEmptyError) {
    const auto mesh = ObjLoader{}.parse("# nothing here\n");
    ASSERT_FALSE(mesh.has_value());
    EXPECT_EQ(mesh.error(), LoadError::empty);
}

// A triangle whose first vertex is (1, 2, 3) with an explicit normal, used to
// verify the Y-up -> Z-up rotation applied by load_mesh.
constexpr std::string_view yUpTriangle = "v 1 2 3\n"
                                         "v 4 0 0\n"
                                         "v 0 4 0\n"
                                         "vn 0 1 0\n"
                                         "f 1//1 2//1 3//1\n";

TEST(ObjLoaderTest, LoadMeshRotatesObjFromYUpToZUpByDefault) {
    // .obj defaults to Y-up: (x, y, z) -> (x, -z, y). So (1, 2, 3) -> (1, -3, 2)
    // and the up normal (0, 1, 0) -> (0, 0, 1) (renderer Z-up).
    const auto mesh = renderer::load_mesh(".obj", std::string{yUpTriangle});
    ASSERT_TRUE(mesh.has_value());
    ASSERT_GE(mesh->vertices.size(), 3U);
    EXPECT_FLOAT_EQ(mesh->vertices[0], 1.0F);
    EXPECT_FLOAT_EQ(mesh->vertices[1], -3.0F);
    EXPECT_FLOAT_EQ(mesh->vertices[2], 2.0F);
    ASSERT_GE(mesh->normals.size(), 3U);
    EXPECT_FLOAT_EQ(mesh->normals[0], 0.0F);
    EXPECT_FLOAT_EQ(mesh->normals[1], 0.0F);
    EXPECT_FLOAT_EQ(mesh->normals[2], 1.0F);
}

TEST(ObjLoaderTest, LoadMeshLeavesGeometryUnchangedWhenUpAxisIsZ) {
    renderer::MeshLoadOptions options;
    options.upAxis = renderer::SourceUpAxis::Z;
    const auto mesh = renderer::load_mesh(".obj", std::string{yUpTriangle}, options);
    ASSERT_TRUE(mesh.has_value());
    ASSERT_GE(mesh->vertices.size(), 3U);
    EXPECT_FLOAT_EQ(mesh->vertices[0], 1.0F);
    EXPECT_FLOAT_EQ(mesh->vertices[1], 2.0F);
    EXPECT_FLOAT_EQ(mesh->vertices[2], 3.0F);
}

TEST(ObjLoaderTest, RetainsTextureCoordinatesParallelToVertices) {
    constexpr std::string_view obj = "v 0 0 0\n"
                                     "v 1 0 0\n"
                                     "v 0 1 0\n"
                                     "vt 0 0\n"
                                     "vt 1 0\n"
                                     "vt 0 1\n"
                                     "f 1/1 2/2 3/3\n";
    const auto mesh = ObjLoader{}.parse(obj);
    ASSERT_TRUE(mesh.has_value());
    // Two uv floats per vertex, parallel to positions.
    ASSERT_EQ(mesh->textureCoordinates.size(), (mesh->vertices.size() / 3U) * 2U);
    EXPECT_FLOAT_EQ(mesh->textureCoordinates[0], 0.0F);
    EXPECT_FLOAT_EQ(mesh->textureCoordinates[1], 0.0F);
    EXPECT_FLOAT_EQ(mesh->textureCoordinates[2], 1.0F);
    EXPECT_FLOAT_EQ(mesh->textureCoordinates[3], 0.0F);
    EXPECT_FLOAT_EQ(mesh->textureCoordinates[4], 0.0F);
    EXPECT_FLOAT_EQ(mesh->textureCoordinates[5], 1.0F);
}

TEST(ObjLoaderTest, SplitsCornersSharingPositionButDifferingUv) {
    // Same position/normal, different vt => distinct emitted vertices.
    constexpr std::string_view obj = "v 0 0 0\n"
                                     "v 1 0 0\n"
                                     "v 0 1 0\n"
                                     "vt 0 0\n"
                                     "vt 1 1\n"
                                     "f 1/1 2/1 3/1\n"
                                     "f 1/2 2/1 3/1\n";
    const auto mesh = ObjLoader{}.parse(obj);
    ASSERT_TRUE(mesh.has_value());
    // Corner 1 appears with two different uvs => 4 unique vertices, not 3.
    EXPECT_EQ(mesh->vertices.size(), 4U * 3U);
}

TEST(ObjLoaderTest, RecordsMaterialLibraryAndSplitsSubmeshesByUsemtl) {
    constexpr std::string_view obj = "mtllib plant.mtl\n"
                                     "v 0 0 0\n"
                                     "v 1 0 0\n"
                                     "v 0 1 0\n"
                                     "v 1 1 0\n"
                                     "usemtl pot\n"
                                     "f 1 2 3\n"
                                     "usemtl leaves\n"
                                     "f 2 4 3\n";
    const auto mesh = ObjLoader{}.parse(obj);
    ASSERT_TRUE(mesh.has_value());
    EXPECT_EQ(mesh->materialLibrary, "plant.mtl");
    ASSERT_EQ(mesh->subMeshes.size(), 2U);
    EXPECT_EQ(mesh->subMeshes[0].materialName, "pot");
    EXPECT_EQ(mesh->subMeshes[0].indexOffset, 0U);
    EXPECT_EQ(mesh->subMeshes[0].indexCount, 3U);
    EXPECT_EQ(mesh->subMeshes[1].materialName, "leaves");
    EXPECT_EQ(mesh->subMeshes[1].indexOffset, 3U);
    EXPECT_EQ(mesh->subMeshes[1].indexCount, 3U);
}

TEST(ObjLoaderTest, FacesBeforeAnyUsemtlFormADefaultSubmesh) {
    constexpr std::string_view obj = "v 0 0 0\n"
                                     "v 1 0 0\n"
                                     "v 0 1 0\n"
                                     "f 1 2 3\n";
    const auto mesh = ObjLoader{}.parse(obj);
    ASSERT_TRUE(mesh.has_value());
    ASSERT_EQ(mesh->subMeshes.size(), 1U);
    EXPECT_TRUE(mesh->subMeshes[0].materialName.empty());
    EXPECT_EQ(mesh->subMeshes[0].indexCount, 3U);
}

} // namespace
