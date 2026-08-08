#include "plinth/loader/MeshLoader.hpp"

#include <filesystem>
#include <gtest/gtest.h>
#include <string>

namespace {

using renderer::load_mesh;
using renderer::LoadError;

TEST(MeshLoaderDispatchTest, DispatchesObjByExtension) {
    const auto mesh = load_mesh(".obj", "v 0 0 0\nv 1 0 0\nv 0 1 0\nf 1 2 3\n");
    ASSERT_TRUE(mesh.has_value());
    EXPECT_EQ(mesh->triangleIndices.size(), 3U);
}

TEST(MeshLoaderDispatchTest, ExtensionMatchIsCaseInsensitive) {
    const auto mesh = load_mesh(".OBJ", "v 0 0 0\nv 1 0 0\nv 0 1 0\nf 1 2 3\n");
    ASSERT_TRUE(mesh.has_value());
    EXPECT_EQ(mesh->triangleIndices.size(), 3U);
}

TEST(MeshLoaderDispatchTest, UnknownExtensionIsUnsupported) {
    const auto mesh = load_mesh(".ply", "irrelevant contents");
    ASSERT_FALSE(mesh.has_value());
    EXPECT_EQ(mesh.error(), LoadError::unsupportedFormat);
}

TEST(MeshLoaderDispatchTest, MissingFileIsFileNotFound) {
    const auto mesh = load_mesh(std::filesystem::path{"does_not_exist_12345.obj"});
    ASSERT_FALSE(mesh.has_value());
    EXPECT_EQ(mesh.error(), LoadError::fileNotFound);
}

} // namespace
