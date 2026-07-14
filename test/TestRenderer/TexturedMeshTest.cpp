#include <gtest/gtest.h>
#include <plinth/Renderer.hpp>
#include <plinth/Texture.hpp>

TEST(TexturedMeshTest, InvalidTextureHandleRejectsMesh) {
    renderer::TextureHandle invalid;
    EXPECT_FALSE(invalid.is_valid());
}
