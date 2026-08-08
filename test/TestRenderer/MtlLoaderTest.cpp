#include "plinth/loader/MtlLoader.hpp"

#include <array>
#include <filesystem>
#include <gtest/gtest.h>
#include <string_view>

namespace {

using renderer::LoadError;
using renderer::parse_mtl;

TEST(MtlLoaderTest, ParsesMaterialNamesAndDiffuseTextures) {
    constexpr std::string_view mtl =
        "newmtl IDP_Pot\n"
        "Kd 0.64 0.64 0.64\n"
        "map_Kd textures/pot.jpg\n"
        "newmtl IDP_leaves\n"
        "map_Kd textures/leaves.jpg\n";
    const auto materials = parse_mtl(mtl, std::filesystem::path("/base"));
    ASSERT_TRUE(materials.has_value());
    ASSERT_EQ(materials->size(), 2U);
    EXPECT_EQ((*materials)[0].name, "IDP_Pot");
    EXPECT_EQ((*materials)[1].name, "IDP_leaves");
    EXPECT_EQ((*materials)[0].diffuseColor, (std::array<float, 4>{0.64F, 0.64F, 0.64F, 1.0F}));
    // Resolved relative to the base directory.
    EXPECT_EQ(std::filesystem::path((*materials)[0].diffuseTexturePath),
              std::filesystem::path("/base/textures/pot.jpg"));
}

TEST(MtlLoaderTest, NormalizesBackslashesAndCollapsesDoubledSeparators) {
    // Blender exports Windows-style, sometimes-doubled backslash paths.
    constexpr std::string_view mtl =
        "newmtl m\n"
        "map_Kd textures\\\\indoor plant_2_COL.jpg\n";
    const auto materials = parse_mtl(mtl, std::filesystem::path("/base"));
    ASSERT_TRUE(materials.has_value());
    ASSERT_EQ(materials->size(), 1U);
    // Doubled separators collapse; interior spaces are preserved.
    EXPECT_EQ(std::filesystem::path((*materials)[0].diffuseTexturePath),
              std::filesystem::path("/base/textures/indoor plant_2_COL.jpg"));
}

TEST(MtlLoaderTest, MaterialWithoutMapKdHasEmptyPath) {
    constexpr std::string_view mtl =
        "newmtl plain\n"
        "Kd 1 1 1\n";
    const auto materials = parse_mtl(mtl, std::filesystem::path("/base"));
    ASSERT_TRUE(materials.has_value());
    ASSERT_EQ(materials->size(), 1U);
    EXPECT_TRUE((*materials)[0].diffuseTexturePath.empty());
    EXPECT_EQ((*materials)[0].diffuseColor, (std::array<float, 4>{1.0F, 1.0F, 1.0F, 1.0F}));
}

TEST(MtlLoaderTest, RetainsDiffuseColorsForUntexturedMaterials) {
    constexpr std::string_view mtl =
        "newmtl Bark\n"
        "Kd 0.207595 0.138513 0.055181\n"
        "newmtl Tree\n"
        "Kd 0.256861 0.440506 0.110769\n";
    const auto materials = parse_mtl(mtl, std::filesystem::path("/base"));
    ASSERT_TRUE(materials.has_value());
    ASSERT_EQ(materials->size(), 2U);
    EXPECT_EQ((*materials)[0].diffuseColor, (std::array<float, 4>{0.207595F, 0.138513F, 0.055181F, 1.0F}));
    EXPECT_EQ((*materials)[1].diffuseColor, (std::array<float, 4>{0.256861F, 0.440506F, 0.110769F, 1.0F}));
}

TEST(MtlLoaderTest, EmptyLibraryIsEmptyError) {
    const auto materials = parse_mtl("# just a comment\n", std::filesystem::path("/base"));
    ASSERT_FALSE(materials.has_value());
    EXPECT_EQ(materials.error(), LoadError::empty);
}

} // namespace
