#include "OpenGL/Drawable/SphereImpostorDrawable.hpp"

#include "OpenGL/Drawable/SphereInstanceData.hpp"
#include "OpenGL/OpenGL.hpp"
#include "OpenGL/VertexArray.hpp"

#include <linal/hmat.hpp>

#include <algorithm>
#include <cstddef>
#include <glm/gtc/matrix_inverse.hpp>
#include <utility>
#include <vector>

namespace opengl {

namespace {

[[nodiscard]] linal::hmatf inverse_matrix(const linal::hmatf& matrix) {
    // linal::hmatf::inverse is optimized for orthogonal transforms. Sphere drawables accept
    // general non-singular affine model transforms, so use GLM's general inverse here. Keep the
    // explicit copies: linal is row-major while GLM indexes column first.
    glm::mat4 glmMatrix{0.0F};
    for (linal::hmatf::size_type row = 0; row < 4; ++row) {
        for (linal::hmatf::size_type column = 0; column < 4; ++column) {
            glmMatrix[column][row] = matrix(row, column);
        }
    }

    const glm::mat4 glmInverse = glm::inverse(glmMatrix);
    linal::hmatf inverse;
    for (linal::hmatf::size_type row = 0; row < 4; ++row) {
        for (linal::hmatf::size_type column = 0; column < 4; ++column) {
            inverse(row, column) = glmInverse[column][row];
        }
    }
    return inverse;
}

} // namespace

SphereImpostorDrawable::SphereImpostorDrawable(SphereImpostorProgram& program,
                                               VertexArray vertexArray,
                                               InstanceBuffer opaqueInstanceBuffer,
                                               InstanceBuffer translucentInstanceBuffer,
                                               DrawableTransparencyInfo transparencyInfo,
                                               std::vector<SortableSphereInstance> translucentSpheres,
                                               std::vector<linal::float3> centers)
    : m_program{&program}
    , m_vertexArray{std::move(vertexArray)}
    , m_opaqueInstanceBuffer{std::move(opaqueInstanceBuffer)}
    , m_translucentInstanceBuffer{std::move(translucentInstanceBuffer)}
    , m_transparencyInfo{transparencyInfo}
    , m_translucentSpheres{std::move(translucentSpheres)}
    , m_centers{std::move(centers)} {
}

SphereImpostorDrawable::SphereImpostorDrawable(SphereImpostorDrawable&& other) noexcept
    : m_program{other.m_program}
    , m_vertexArray{std::move(other.m_vertexArray)}
    , m_opaqueInstanceBuffer{std::move(other.m_opaqueInstanceBuffer)}
    , m_translucentInstanceBuffer{std::move(other.m_translucentInstanceBuffer)}
    , m_transparencyInfo{other.m_transparencyInfo}
    , m_translucentSpheres{std::move(other.m_translucentSpheres)}
    , m_centers{std::move(other.m_centers)} {
    other.m_program = nullptr;
}

SphereImpostorDrawable& SphereImpostorDrawable::operator=(SphereImpostorDrawable&& other) noexcept {
    if (this != &other) {
        m_program = other.m_program;
        m_vertexArray = std::move(other.m_vertexArray);
        m_opaqueInstanceBuffer = std::move(other.m_opaqueInstanceBuffer);
        m_translucentInstanceBuffer = std::move(other.m_translucentInstanceBuffer);
        m_transparencyInfo = other.m_transparencyInfo;
        m_translucentSpheres = std::move(other.m_translucentSpheres);
        m_centers = std::move(other.m_centers);
        other.m_program = nullptr;
    }
    return *this;
}

void SphereImpostorDrawable::set_common_uniforms(const linal::hmatf& viewMatrix,
                                                 const linal::hmatf& projectionMatrix,
                                                 const linal::hmatf& inverseProjectionMatrix,
                                                 const linal::hmatf& modelMatrix,
                                                 const linal::float2& viewportSize,
                                                 bool zeroToOneDepth,
                                                 const LightingConfig& lighting) const {
    // The fragment shader has two deliberate working spaces: intersection in local space and
    // lighting in view space. These matrices and transformed lights are the bridge between them.
    // A position uses w=1 so camera translation affects it; a direction uses w=0 so it does not.
    const linal::hmatf modelViewMatrix = viewMatrix * modelMatrix;
    const linal::hmatf inverseModelViewMatrix = inverse_matrix(modelViewMatrix);
    const linal::hmatf normalMatrix = inverseModelViewMatrix.transpose();
    const linal::float3 lightPositionView = linal::to_vec(viewMatrix * linal::to_hvec(lighting.lightPosition));
    const linal::float3 fillLightDirectionView =
        linal::to_vec(viewMatrix * linal::to_hvec(lighting.fillLightDir, 0.0F));

    // Matrices: linal is row-major; GL expects column-major → GL_TRUE transposes on upload.
    glUniformMatrix4fv(m_program->get_model_matrix_location().get_value(), 1, GL_TRUE, modelMatrix.data());
    glUniformMatrix4fv(m_program->get_view_matrix_location().get_value(), 1, GL_TRUE, viewMatrix.data());
    glUniformMatrix4fv(m_program->get_projection_matrix_location().get_value(), 1, GL_TRUE, projectionMatrix.data());
    glUniformMatrix4fv(m_program->get_inverse_model_view_matrix_location().get_value(),
                       1,
                       GL_TRUE,
                       inverseModelViewMatrix.data());
    glUniformMatrix4fv(m_program->get_normal_matrix_location().get_value(), 1, GL_TRUE, normalMatrix.data());
    glUniformMatrix4fv(m_program->get_inv_projection_location().get_value(),
                       1,
                       GL_TRUE,
                       inverseProjectionMatrix.data());
    glUniform2f(m_program->get_viewport_size_location().get_value(), viewportSize[0], viewportSize[1]);
    glUniform1i(m_program->get_zero_to_one_depth_location().get_value(), zeroToOneDepth ? 1 : 0);

    glUniform3f(m_program->get_light_pos_location().get_value(),
                lightPositionView[0],
                lightPositionView[1],
                lightPositionView[2]);
    glUniform3f(m_program->get_light_color_location().get_value(),
                lighting.lightColor[0],
                lighting.lightColor[1],
                lighting.lightColor[2]);
    glUniform3f(m_program->get_fill_light_direction_location().get_value(),
                fillLightDirectionView[0],
                fillLightDirectionView[1],
                fillLightDirectionView[2]);
    glUniform3f(m_program->get_fill_light_color_location().get_value(),
                lighting.fillLightColor[0],
                lighting.fillLightColor[1],
                lighting.fillLightColor[2]);
    glUniform3f(m_program->get_ambient_color_location().get_value(),
                lighting.ambientColor[0],
                lighting.ambientColor[1],
                lighting.ambientColor[2]);
    glUniform1f(m_program->get_shininess_location().get_value(), lighting.shininess);
    glUniform3f(m_program->get_light_attenuation_location().get_value(),
                lighting.lightAttenuation[0],
                lighting.lightAttenuation[1],
                lighting.lightAttenuation[2]);
    glUniform3f(m_program->get_material_ambient_location().get_value(),
                lighting.materialAmbient[0],
                lighting.materialAmbient[1],
                lighting.materialAmbient[2]);
    glUniform3f(m_program->get_material_diffuse_location().get_value(),
                lighting.materialDiffuse[0],
                lighting.materialDiffuse[1],
                lighting.materialDiffuse[2]);
    glUniform3f(m_program->get_material_specular_location().get_value(),
                lighting.materialSpecular[0],
                lighting.materialSpecular[1],
                lighting.materialSpecular[2]);

    glUniform1i(m_program->get_pick_mode_location().get_value(), 0);
}

void SphereImpostorDrawable::draw_instances(const InstanceBuffer& instanceBuffer) const {
    const GLsizei instanceCount = instanceBuffer.get_instance_count();
    if (instanceCount == 0) {
        return;
    }
    m_vertexArray.bind();
    instanceBuffer.bind();
    // 6 vertices per sphere (2 triangles), no divisor-0 buffer — geometry from gl_VertexID.
    glDrawArraysInstanced(GL_TRIANGLES, 0, 6, instanceCount);
    m_vertexArray.unbind();
}

void SphereImpostorDrawable::draw(const linal::hmatf& viewMatrix,
                                  const linal::hmatf& projectionMatrix,
                                  const linal::hmatf& inverseProjectionMatrix,
                                  const linal::hmatf& modelMatrix,
                                  const linal::float2& viewportSize,
                                  bool zeroToOneDepth,
                                  const LightingConfig& lighting) const {
    m_program->use();
    set_common_uniforms(viewMatrix,
                        projectionMatrix,
                        inverseProjectionMatrix,
                        modelMatrix,
                        viewportSize,
                        zeroToOneDepth,
                        lighting);
    draw_instances(m_opaqueInstanceBuffer);
    draw_instances(m_translucentInstanceBuffer);
}

void SphereImpostorDrawable::draw_opaque(const linal::hmatf& viewMatrix,
                                         const linal::hmatf& projectionMatrix,
                                         const linal::hmatf& inverseProjectionMatrix,
                                         const linal::hmatf& modelMatrix,
                                         const linal::float2& viewportSize,
                                         bool zeroToOneDepth,
                                         const LightingConfig& lighting) const {
    if (!has_opaque_primitives()) {
        return;
    }
    m_program->use();
    set_common_uniforms(viewMatrix,
                        projectionMatrix,
                        inverseProjectionMatrix,
                        modelMatrix,
                        viewportSize,
                        zeroToOneDepth,
                        lighting);
    draw_instances(m_opaqueInstanceBuffer);
}

void SphereImpostorDrawable::draw_translucent(const linal::hmatf& viewMatrix,
                                              const linal::hmatf& projectionMatrix,
                                              const linal::hmatf& inverseProjectionMatrix,
                                              const linal::hmatf& modelMatrix,
                                              const linal::float2& viewportSize,
                                              bool zeroToOneDepth,
                                              const LightingConfig& lighting,
                                              const linal::double3& viewPosition) {
    if (!has_translucent_primitives()) {
        return;
    }

    // This is the inner of two transparency sorts. DrawablesManager first orders entire sphere
    // drawables; this pass then orders instances inside one drawable. Re-sort back-to-front in
    // world space because the stored centers are local while the camera position is world space.
    struct SortedSphere {
        const SortableSphereInstance* sphere{nullptr};
        double distanceSquared{0.0};
    };
    std::vector<SortedSphere> sorted;
    sorted.reserve(m_translucentSpheres.size());
    for (const auto& sphere: m_translucentSpheres) {
        const linal::float3 worldCenter = linal::to_vec(modelMatrix * linal::to_hvec(sphere.sortCenter));
        sorted.push_back(SortedSphere{&sphere, opengl::distance_squared_to(worldCenter, viewPosition)});
    }
    std::sort(sorted.begin(), sorted.end(), [](const SortedSphere& lhs, const SortedSphere& rhs) {
        return lhs.distanceSquared > rhs.distanceSquared;
    });

    // Build a sorted flat instance blob and re-upload.
    std::vector<float> sortedData;
    sortedData.reserve(sorted.size() * kSphereInstanceFloats);
    for (const auto& sortedSphere: sorted) {
        sortedData.insert(sortedData.end(), sortedSphere.sphere->data.begin(), sortedSphere.sphere->data.end());
    }

    m_translucentInstanceBuffer.update(sortedData, BufferAccessPattern::Stream);

    m_program->use();
    set_common_uniforms(viewMatrix,
                        projectionMatrix,
                        inverseProjectionMatrix,
                        modelMatrix,
                        viewportSize,
                        zeroToOneDepth,
                        lighting);
    draw_instances(m_translucentInstanceBuffer);
}

void SphereImpostorDrawable::draw_pick(const linal::hmatf& viewMatrix,
                                       const linal::hmatf& projectionMatrix,
                                       const linal::hmatf& inverseProjectionMatrix,
                                       const linal::hmatf& modelMatrix,
                                       const linal::float2& viewportSize,
                                       bool zeroToOneDepth,
                                       const std::array<float, 3>& pickColor) const {
    const GLsizei opaqueCount = m_opaqueInstanceBuffer.get_instance_count();
    const GLsizei translucentCount = m_translucentInstanceBuffer.get_instance_count();
    if (opaqueCount == 0 && translucentCount == 0) {
        return;
    }

    m_program->use();

    const LightingConfig defaultLighting{};

    // Picking must use the same model-aware intersection and projected surface depth as the color
    // pass. Otherwise the rectangular proxy, rather than the transformed sphere, would be picked.
    glUniformMatrix4fv(m_program->get_model_matrix_location().get_value(), 1, GL_TRUE, modelMatrix.data());
    glUniformMatrix4fv(m_program->get_view_matrix_location().get_value(), 1, GL_TRUE, viewMatrix.data());
    glUniformMatrix4fv(m_program->get_projection_matrix_location().get_value(), 1, GL_TRUE, projectionMatrix.data());
    const linal::hmatf inverseModelViewMatrix = inverse_matrix(viewMatrix * modelMatrix);
    glUniformMatrix4fv(m_program->get_inverse_model_view_matrix_location().get_value(),
                       1,
                       GL_TRUE,
                       inverseModelViewMatrix.data());
    glUniformMatrix4fv(m_program->get_inv_projection_location().get_value(),
                       1,
                       GL_TRUE,
                       inverseProjectionMatrix.data());
    glUniform2f(m_program->get_viewport_size_location().get_value(), viewportSize[0], viewportSize[1]);
    glUniform1i(m_program->get_zero_to_one_depth_location().get_value(), zeroToOneDepth ? 1 : 0);

    // Lighting uniforms must be set even in pick mode (they're queried by location at compile time).
    glUniform3f(m_program->get_light_pos_location().get_value(), 0.0F, 0.0F, 0.0F);
    glUniform3f(m_program->get_light_color_location().get_value(), 1.0F, 1.0F, 1.0F);
    glUniform3f(m_program->get_fill_light_direction_location().get_value(), 0.0F, 0.0F, 0.0F);
    glUniform3f(m_program->get_fill_light_color_location().get_value(), 0.0F, 0.0F, 0.0F);
    glUniform3f(m_program->get_ambient_color_location().get_value(), 0.0F, 0.0F, 0.0F);
    glUniform1f(m_program->get_shininess_location().get_value(), defaultLighting.shininess);
    glUniform3f(m_program->get_light_attenuation_location().get_value(), 1.0F, 0.0F, 0.0F);
    glUniform3f(m_program->get_material_ambient_location().get_value(), 0.0F, 0.0F, 0.0F);
    glUniform3f(m_program->get_material_diffuse_location().get_value(), 0.0F, 0.0F, 0.0F);
    glUniform3f(m_program->get_material_specular_location().get_value(), 0.0F, 0.0F, 0.0F);

    glUniform1i(m_program->get_pick_mode_location().get_value(), 1);
    glUniform3f(m_program->get_pick_color_location().get_value(), pickColor[0], pickColor[1], pickColor[2]);

    draw_instances(m_opaqueInstanceBuffer);
    draw_instances(m_translucentInstanceBuffer);
}

std::optional<SphereImpostorDrawable> make_sphere_impostor_drawable(SphereImpostorProgram& program,
                                                                    std::span<const float> centers,
                                                                    std::span<const float> radii,
                                                                    std::span<const float> colors,
                                                                    BufferAccessPattern accessPattern) {
    if (centers.size() % 3 != 0 || colors.size() % 4 != 0) {
        return std::nullopt;
    }
    const std::size_t sphereCount = centers.size() / 3U;
    if (sphereCount == 0 || sphereCount != radii.size() || sphereCount != colors.size() / 4U) {
        return std::nullopt;
    }

    const auto attribs = make_sphere_instance_attribs(program);
    const bool hasTranslucent = contains_translucent_alpha(colors, 4);

    // Split once at construction so opaque rendering never pays for blending or per-frame sorting.
    // A translucent CPU copy is retained below because its GPU order changes with the camera.
    std::vector<float> opaqueData;
    std::vector<float> translucentData;
    std::vector<SphereImpostorDrawable::SortableSphereInstance> translucentSpheres;
    opaqueData.reserve(sphereCount * kSphereInstanceFloats);
    translucentData.reserve(sphereCount * kSphereInstanceFloats);

    for (std::size_t i = 0; i < sphereCount; ++i) {
        const float alpha = colors[i * 4U + 3U];
        std::array<float, 8> inst{centers[i * 3U],
                                  centers[i * 3U + 1U],
                                  centers[i * 3U + 2U],
                                  radii[i],
                                  colors[i * 4U],
                                  colors[i * 4U + 1U],
                                  colors[i * 4U + 2U],
                                  alpha};
        if (alpha < 1.0F) {
            translucentData.insert(translucentData.end(), inst.begin(), inst.end());
            translucentSpheres.push_back(SphereImpostorDrawable::SortableSphereInstance{
                inst,
                linal::float3{centers[i * 3U], centers[i * 3U + 1U], centers[i * 3U + 2U]}});
        } else {
            opaqueData.insert(opaqueData.end(), inst.begin(), inst.end());
        }
    }

    auto vertexArray = VertexArray::create();
    if (!vertexArray) {
        return std::nullopt;
    }
    vertexArray->bind();

    // Create opaque buffer (may be empty — create with a placeholder to keep the VAO consistent).
    const std::span<const float> opaqueSpan =
        opaqueData.empty() ? std::span<const float>{} : std::span<const float>{opaqueData};
    auto opaqueBuffer = InstanceBuffer::create(opaqueSpan, kSphereInstanceStride, attribs, accessPattern);
    if (!opaqueBuffer) {
        vertexArray->unbind();
        return std::nullopt;
    }

    const std::span<const float> translucentSpan =
        translucentData.empty() ? std::span<const float>{} : std::span<const float>{translucentData};
    auto translucentBuffer = InstanceBuffer::create(translucentSpan,
                                                    kSphereInstanceStride,
                                                    attribs,
                                                    hasTranslucent ? BufferAccessPattern::Stream : accessPattern);
    if (!translucentBuffer) {
        vertexArray->unbind();
        return std::nullopt;
    }

    vertexArray->unbind();

    // Build DrawableTransparencyInfo using centers as the vertex array.
    const DrawableTransparencyInfo transparencyInfo = make_drawable_transparency_info(centers, 3, colors, 4);

    // Collect all sphere centers for get_vertex_positions().
    std::vector<linal::float3> centerVec;
    centerVec.reserve(sphereCount);
    for (std::size_t i = 0; i < sphereCount; ++i) {
        centerVec.push_back(linal::float3{centers[i * 3U], centers[i * 3U + 1U], centers[i * 3U + 2U]});
    }

    return SphereImpostorDrawable{program,
                                  std::move(*vertexArray),
                                  std::move(*opaqueBuffer),
                                  std::move(*translucentBuffer),
                                  transparencyInfo,
                                  std::move(translucentSpheres),
                                  std::move(centerVec)};
}

} // namespace opengl
