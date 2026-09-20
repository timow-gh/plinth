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

// Splits parallel center/radius/color arrays into opaque and translucent interleaved instance blobs
// (ABI: cx, cy, cz, radius, r, g, b, a). A sphere is translucent when its alpha < 1. The translucent
// spheres also get a SortableSphereInstance copy so their GPU order can be resorted per frame.
// Shared by make_sphere_impostor_drawable and SphereImpostorDrawable::update_colors so the split
// stays identical in both paths.
struct SphereInstanceSplit {
    std::vector<float> opaqueData;
    std::vector<float> translucentData;
    std::vector<SphereImpostorDrawable::SortableSphereInstance> translucentSpheres;
};

[[nodiscard]] SphereInstanceSplit split_sphere_instances(std::span<const float> centers,
                                                         std::span<const float> radii,
                                                         std::span<const float> colors) {
    const std::size_t sphereCount = centers.size() / 3U;
    SphereInstanceSplit split;
    split.opaqueData.reserve(sphereCount * kSphereInstanceFloats);
    split.translucentData.reserve(sphereCount * kSphereInstanceFloats);

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
            split.translucentData.insert(split.translucentData.end(), inst.begin(), inst.end());
            split.translucentSpheres.push_back(SphereImpostorDrawable::SortableSphereInstance{
                inst,
                linal::float3{centers[i * 3U], centers[i * 3U + 1U], centers[i * 3U + 2U]}});
        } else {
            split.opaqueData.insert(split.opaqueData.end(), inst.begin(), inst.end());
        }
    }
    return split;
}

} // namespace

SphereImpostorDrawable::SphereImpostorDrawable(SphereImpostorProgram& program,
                                               VertexArray vertexArray,
                                               InstanceBuffer opaqueInstanceBuffer,
                                               InstanceBuffer translucentInstanceBuffer,
                                               DrawableTransparencyInfo transparencyInfo,
                                               std::vector<SortableSphereInstance> translucentSpheres,
                                               std::vector<linal::float3> centers,
                                               std::vector<float> radii)
    : m_program{&program}
    , m_vertexArray{std::move(vertexArray)}
    , m_opaqueInstanceBuffer{std::move(opaqueInstanceBuffer)}
    , m_translucentInstanceBuffer{std::move(translucentInstanceBuffer)}
    , m_transparencyInfo{transparencyInfo}
    , m_translucentSpheres{std::move(translucentSpheres)}
    , m_centers{std::move(centers)}
    , m_radii{std::move(radii)} {
}

SphereImpostorDrawable::SphereImpostorDrawable(SphereImpostorDrawable&& other) noexcept
    : m_program{other.m_program}
    , m_vertexArray{std::move(other.m_vertexArray)}
    , m_opaqueInstanceBuffer{std::move(other.m_opaqueInstanceBuffer)}
    , m_translucentInstanceBuffer{std::move(other.m_translucentInstanceBuffer)}
    , m_transparencyInfo{other.m_transparencyInfo}
    , m_translucentSpheres{std::move(other.m_translucentSpheres)}
    , m_centers{std::move(other.m_centers)}
    , m_radii{std::move(other.m_radii)}
    , m_sizeSpace{other.m_sizeSpace} {
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
        m_radii = std::move(other.m_radii);
        m_sizeSpace = other.m_sizeSpace;
        other.m_program = nullptr;
    }
    return *this;
}

bool SphereImpostorDrawable::update_colors(std::span<const float> colors, BufferAccessPattern accessPattern) {
    const std::size_t sphereCount = m_radii.size();
    if (colors.size() % 4U != 0 || colors.size() / 4U != sphereCount) {
        return false;
    }

    // Flatten retained centers back to the (x, y, z) triplet layout the split helper expects.
    std::vector<float> centers;
    centers.reserve(sphereCount * 3U);
    for (const linal::float3& center: m_centers) {
        centers.push_back(center[0]);
        centers.push_back(center[1]);
        centers.push_back(center[2]);
    }

    SphereInstanceSplit split = split_sphere_instances(centers, m_radii, colors);

    // Re-upload both buffers. update() re-orphans storage and recomputes the instance count, so a
    // sphere whose alpha crossed 1.0 correctly migrates between the opaque and translucent buffers.
    // The translucent draw path re-sorts m_translucentSpheres each frame, so replacing it is enough.
    m_opaqueInstanceBuffer.update(split.opaqueData, accessPattern);
    m_translucentInstanceBuffer.update(split.translucentData,
                                       split.translucentData.empty() ? accessPattern : BufferAccessPattern::Stream);
    m_translucentSpheres = std::move(split.translucentSpheres);
    m_transparencyInfo.isTranslucent = contains_translucent_alpha(colors, 4);
    return true;
}

void SphereImpostorDrawable::set_common_uniforms(const linal::hmatf& viewMatrix, const linal::hmatf& modelMatrix) const {
    // The fragment shader has two deliberate working spaces: intersection in local space and
    // lighting in view space. The inverse model-view and its inverse-transpose (normal matrix)
    // bridge them; view/projection/inverse-projection and the lights live in the FrameBlock UBO.
    const linal::hmatf modelViewMatrix = viewMatrix * modelMatrix;
    const linal::hmatf inverseModelViewMatrix = inverse_matrix(modelViewMatrix);
    const linal::hmatf normalMatrix = inverseModelViewMatrix.transpose();

    // Matrices: linal is row-major; GL expects column-major → GL_TRUE transposes on upload.
    glUniformMatrix4fv(m_program->get_model_matrix_location().get_value(), 1, GL_TRUE, modelMatrix.data());
    glUniformMatrix4fv(m_program->get_inverse_model_view_matrix_location().get_value(),
                       1,
                       GL_TRUE,
                       inverseModelViewMatrix.data());
    glUniformMatrix4fv(m_program->get_normal_matrix_location().get_value(), 1, GL_TRUE, normalMatrix.data());
    glUniform1i(m_program->get_size_space_location().get_value(), static_cast<GLint>(m_sizeSpace));

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

void SphereImpostorDrawable::draw_opaque(const linal::hmatf& viewMatrix, const linal::hmatf& modelMatrix) const {
    if (!has_opaque_primitives()) {
        return;
    }
    m_program->use();
    set_common_uniforms(viewMatrix, modelMatrix);
    draw_instances(m_opaqueInstanceBuffer);
}

void SphereImpostorDrawable::draw_translucent(const linal::hmatf& viewMatrix,
                                              const linal::hmatf& modelMatrix,
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
    set_common_uniforms(viewMatrix, modelMatrix);
    draw_instances(m_translucentInstanceBuffer);
}

void SphereImpostorDrawable::draw_pick(const linal::hmatf& viewMatrix,
                                       const linal::hmatf& modelMatrix,
                                       const std::array<float, 3>& pickColor) const {
    const GLsizei opaqueCount = m_opaqueInstanceBuffer.get_instance_count();
    const GLsizei translucentCount = m_translucentInstanceBuffer.get_instance_count();
    if (opaqueCount == 0 && translucentCount == 0) {
        return;
    }

    m_program->use();
    set_common_uniforms(viewMatrix, modelMatrix);

    // Picking must use the same model-aware intersection and projected surface depth as the color
    // pass. Override the visible-color state with the flat pick color.
    glUniform1i(m_program->get_pick_mode_location().get_value(), 1);
    glUniform3f(m_program->get_pick_color_location().get_value(), pickColor[0], pickColor[1], pickColor[2]);

    draw_instances(m_opaqueInstanceBuffer);
    draw_instances(m_translucentInstanceBuffer);

    // Leave pick mode disabled so a subsequent normal draw is unaffected.
    glUniform1i(m_program->get_pick_mode_location().get_value(), 0);
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
    SphereInstanceSplit split = split_sphere_instances(centers, radii, colors);
    std::vector<float>& opaqueData = split.opaqueData;
    std::vector<float>& translucentData = split.translucentData;
    std::vector<SphereImpostorDrawable::SortableSphereInstance>& translucentSpheres = split.translucentSpheres;

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

    // Collect all sphere centers for get_vertex_positions() and retain radii so a later color-only
    // update can rebuild instance data without the caller re-supplying geometry.
    std::vector<linal::float3> centerVec;
    centerVec.reserve(sphereCount);
    for (std::size_t i = 0; i < sphereCount; ++i) {
        centerVec.push_back(linal::float3{centers[i * 3U], centers[i * 3U + 1U], centers[i * 3U + 2U]});
    }
    std::vector<float> radiiVec{radii.begin(), radii.end()};

    return SphereImpostorDrawable{program,
                                  std::move(*vertexArray),
                                  std::move(*opaqueBuffer),
                                  std::move(*translucentBuffer),
                                  transparencyInfo,
                                  std::move(translucentSpheres),
                                  std::move(centerVec),
                                  std::move(radiiVec)};
}

} // namespace opengl
