#ifndef OPENGL_FRAMEUNIFORMS_HPP
#define OPENGL_FRAMEUNIFORMS_HPP

#include "OpenGL/OpenGL.hpp"
#include "plinth/LightingConfig.hpp"

#include <linal/hmat.hpp>
#include <linal/vec.hpp>

#include <array>
#include <cstddef>
#include <span>

namespace opengl {

// Binding point shared by every program's `FrameBlock` uniform block. The renderer uploads the
// block once per frame and binds it here, so per-drawable uploads no longer repeat the frame-constant
// camera matrices and lighting.
inline constexpr GLuint kFrameUniformBinding = 0U;

// CPU mirror of the std140 `FrameBlock` GLSL uniform block. Every member is a 16-byte-aligned
// mat4/vec4 so the C++ layout matches std140 exactly with no manual padding. See
// frame_uniform_block_glsl() in ShaderSources.hpp for the corresponding GLSL declaration.
struct FrameUniforms {
    std::array<float, 16> viewProjection{};    // mat4 projection * view
    std::array<float, 16> view{};              // mat4 world -> view
    std::array<float, 16> projection{};        // mat4 view -> clip
    std::array<float, 16> invProjection{};     // mat4 clip -> view
    std::array<float, 4>  viewPos{};           // vec4 .xyz = camera world position
    std::array<float, 4>  lightPos{};          // vec4 .xyz = light world position
    std::array<float, 4>  lightColor{};        // vec4 .rgb
    std::array<float, 4>  fillLightDirection{}; // vec4 .xyz world-space direction
    std::array<float, 4>  fillLightColor{};    // vec4 .rgb
    std::array<float, 4>  ambientColor{};      // vec4 .rgb
    std::array<float, 4>  materialAmbient{};   // vec4 .rgb
    std::array<float, 4>  materialDiffuse{};   // vec4 .rgb
    std::array<float, 4>  materialSpecular{};  // vec4 .rgb
    std::array<float, 4>  lightAttenuation{};  // vec4 .xyz
    std::array<float, 4>  viewportSize{};      // vec4 .xy
    std::array<float, 4>  parameters{};        // vec4 .x = shininess, .y = zeroToOneDepth (0/1)
};

static_assert(sizeof(FrameUniforms) == (4U * 16U + 12U * 4U) * sizeof(float),
              "FrameUniforms must match the std140 FrameBlock layout");

// The struct is a contiguous, padding-free run of floats (see the static_assert), so it can be
// uploaded to the UBO directly.
[[nodiscard]] inline std::span<const float> frame_uniforms_data(const FrameUniforms& uniforms) noexcept {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    return {reinterpret_cast<const float*>(&uniforms), sizeof(FrameUniforms) / sizeof(float)};
}

// Converts a row-major linal matrix to the column-major float[16] a GLSL std140 mat4 expects.
[[nodiscard]] inline std::array<float, 16> to_column_major(const linal::hmatf& matrix) noexcept {
    std::array<float, 16> out{};
    for (linal::hmatf::size_type column = 0; column < 4; ++column) {
        for (linal::hmatf::size_type row = 0; row < 4; ++row) {
            out[static_cast<std::size_t>(column * 4 + row)] = matrix(row, column);
        }
    }
    return out;
}

inline void assign_vec4(std::array<float, 4>& dst,
                        const linal::float3& src,
                        float w = 0.0F) noexcept {
    dst = {src[0], src[1], src[2], w};
}

// Builds the per-frame UBO contents from the current camera and lighting state.
[[nodiscard]] inline FrameUniforms make_frame_uniforms(const linal::hmatf& viewProjection,
                                                       const linal::hmatf& view,
                                                       const linal::hmatf& projection,
                                                       const linal::hmatf& invProjection,
                                                       const linal::float3& viewPos,
                                                       const linal::float3& lightPos,
                                                       const renderer::LightingConfig& lighting,
                                                       const linal::float2& viewportSize,
                                                       bool zeroToOneDepth) {
    FrameUniforms uniforms;
    uniforms.viewProjection = to_column_major(viewProjection);
    uniforms.view = to_column_major(view);
    uniforms.projection = to_column_major(projection);
    uniforms.invProjection = to_column_major(invProjection);
    assign_vec4(uniforms.viewPos, viewPos);
    assign_vec4(uniforms.lightPos, lightPos);
    assign_vec4(uniforms.lightColor, lighting.lightColor);
    assign_vec4(uniforms.fillLightDirection, lighting.fillLightDir);
    assign_vec4(uniforms.fillLightColor, lighting.fillLightColor);
    assign_vec4(uniforms.ambientColor, lighting.ambientColor);
    assign_vec4(uniforms.materialAmbient, lighting.materialAmbient);
    assign_vec4(uniforms.materialDiffuse, lighting.materialDiffuse);
    assign_vec4(uniforms.materialSpecular, lighting.materialSpecular);
    assign_vec4(uniforms.lightAttenuation, lighting.lightAttenuation);
    uniforms.viewportSize = {viewportSize[0], viewportSize[1], 0.0F, 0.0F};
    uniforms.parameters = {lighting.shininess, zeroToOneDepth ? 1.0F : 0.0F, 0.0F, 0.0F};
    return uniforms;
}

} // namespace opengl

#endif // OPENGL_FRAMEUNIFORMS_HPP
