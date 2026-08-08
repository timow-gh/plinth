#ifndef OPENGL_SPHEREINSTANCEDATA_HPP
#define OPENGL_SPHEREINSTANCEDATA_HPP

#include "OpenGL/InstanceBuffer.hpp"
#include "OpenGL/Programs/SphereImpostorProgram.hpp"

#include <array>
#include <cstddef>
#include <optional>
#include <span>
#include <vector>

namespace opengl {

// This is the shared CPU/GPU ABI for a sphere instance: (cx, cy, cz, radius, r, g, b, a).
// If it changes, also update the attribute offsets, SortableSphereInstance, both buffer-building
// paths in SphereImpostorDrawable.cpp, and the shader inputs. See docs/sphere-point-rendering.md.
inline constexpr std::size_t kSphereInstanceFloats = 8U;
inline constexpr GLsizei kSphereInstanceStride = static_cast<GLsizei>(kSphereInstanceFloats * sizeof(float));

[[nodiscard]] inline std::array<InstanceAttribSpec, 2>
make_sphere_instance_attribs(const SphereImpostorProgram& program) noexcept {
    return {InstanceAttribSpec{program.get_sphere_location(), 4, 0},
            InstanceAttribSpec{program.get_color_location(), 4, 4 * static_cast<GLsizei>(sizeof(float))}};
}

// Builds the flat interleaved instance blob from parallel arrays.
// N = centers.size() / 3 must equal radii.size() and colors.size() / 4.
// Returns nullopt on size mismatch or empty input.
[[nodiscard]] inline std::optional<std::vector<float>>
make_sphere_instance_data(std::span<const float> centers,
                          std::span<const float> radii,
                          std::span<const float> colors) {
    if (centers.size() % 3 != 0 || colors.size() % 4 != 0) {
        return std::nullopt;
    }

    const std::size_t sphereCount = centers.size() / 3U;
    if (sphereCount == 0 || sphereCount != radii.size() || sphereCount != colors.size() / 4U) {
        return std::nullopt;
    }

    std::vector<float> data;
    data.reserve(sphereCount * kSphereInstanceFloats);

    for (std::size_t i = 0; i < sphereCount; ++i) {
        data.push_back(centers[i * 3U]);
        data.push_back(centers[i * 3U + 1U]);
        data.push_back(centers[i * 3U + 2U]);
        data.push_back(radii[i]);
        data.push_back(colors[i * 4U]);
        data.push_back(colors[i * 4U + 1U]);
        data.push_back(colors[i * 4U + 2U]);
        data.push_back(colors[i * 4U + 3U]);
    }

    return data;
}

} // namespace opengl

#endif // OPENGL_SPHEREINSTANCEDATA_HPP
