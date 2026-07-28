#include "plinth/loader/MeshShading.hpp"
#include <array>
#include <cstdint>
#include <linal/vec.hpp>
#include <unordered_map>
#include <vector>

namespace renderer {

namespace {

[[nodiscard]] linal::float3 position_at(const MeshData& mesh, std::uint32_t index) {
    const std::size_t base = static_cast<std::size_t>(index) * 3U;
    return linal::float3{mesh.vertices[base], mesh.vertices[base + 1U], mesh.vertices[base + 2U]};
}

[[nodiscard]] bool has_color(const MeshData& mesh) {
    return !mesh.colors.empty();
}

void push_position(MeshData& out, const linal::float3& p) {
    out.vertices.push_back(p[0]);
    out.vertices.push_back(p[1]);
    out.vertices.push_back(p[2]);
}

void push_normal(MeshData& out, const linal::float3& n) {
    out.normals.push_back(n[0]);
    out.normals.push_back(n[1]);
    out.normals.push_back(n[2]);
}

// Copies the 4 color components for source vertex \p index, if the mesh carries
// per-vertex colors, into \p out.
void push_color(MeshData& out, const MeshData& mesh, std::uint32_t index) {
    if (!has_color(mesh)) {
        return;
    }
    const std::size_t base = static_cast<std::size_t>(index) * 4U;
    for (std::size_t c = 0; c < 4U; ++c) {
        out.colors.push_back(mesh.colors[base + c]);
    }
}

[[nodiscard]] linal::float3 face_normal(const linal::float3& v0, const linal::float3& v1, const linal::float3& v2) {
    // Un-normalized cross product => magnitude proportional to triangle area,
    // which gives area weighting when accumulated for smooth shading.
    return linal::cross(v1 - v0, v2 - v0);
}

[[nodiscard]] linal::float3 safe_normalize(const linal::float3& v) {
    return linal::length(v) > 1.0e-6F ? linal::normalize(v) : linal::float3{0.0F, 0.0F, 1.0F};
}

[[nodiscard]] MeshData make_flat(const MeshData& mesh) {
    MeshData out;
    out.sourceName = mesh.sourceName;
    const std::size_t vertexCount = mesh.vertices.size() / 3U;

    std::uint32_t emitted = 0;
    for (std::size_t t = 0; t + 2U < mesh.triangleIndices.size(); t += 3U) {
        const std::uint32_t i0 = mesh.triangleIndices[t];
        const std::uint32_t i1 = mesh.triangleIndices[t + 1U];
        const std::uint32_t i2 = mesh.triangleIndices[t + 2U];
        if (i0 >= vertexCount || i1 >= vertexCount || i2 >= vertexCount) {
            continue;
        }
        const linal::float3 v0 = position_at(mesh, i0);
        const linal::float3 v1 = position_at(mesh, i1);
        const linal::float3 v2 = position_at(mesh, i2);
        const linal::float3 normal = safe_normalize(face_normal(v0, v1, v2));

        push_position(out, v0);
        push_position(out, v1);
        push_position(out, v2);
        push_normal(out, normal);
        push_normal(out, normal);
        push_normal(out, normal);
        push_color(out, mesh, i0);
        push_color(out, mesh, i1);
        push_color(out, mesh, i2);

        out.triangleIndices.push_back(emitted);
        out.triangleIndices.push_back(emitted + 1U);
        out.triangleIndices.push_back(emitted + 2U);
        emitted += 3U;
    }

    return out;
}

// Welds vertices by exact position and emits one area-weighted averaged normal
// per unique position. Colors are taken from the first vertex seen at a
// position.
[[nodiscard]] MeshData make_smooth(const MeshData& mesh) {
    struct PositionKey {
        float x{0.0F};
        float y{0.0F};
        float z{0.0F};
        bool operator==(const PositionKey&) const = default;
    };
    struct PositionKeyHash {
        std::size_t operator()(const PositionKey& key) const {
            const std::size_t hx = std::hash<float>{}(key.x);
            const std::size_t hy = std::hash<float>{}(key.y);
            const std::size_t hz = std::hash<float>{}(key.z);
            return hx ^ (hy << 1U) ^ (hz << 2U);
        }
    };

    const std::size_t vertexCount = mesh.vertices.size() / 3U;
    std::unordered_map<PositionKey, std::uint32_t, PositionKeyHash> welded;

    MeshData out;
    out.sourceName = mesh.sourceName;
    std::vector<linal::float3> accum; // accumulated (area-weighted) normals per welded vertex

    const auto weld = [&](std::uint32_t source) -> std::uint32_t {
        const linal::float3 p = position_at(mesh, source);
        const PositionKey key{p[0], p[1], p[2]};
        if (const auto it = welded.find(key); it != welded.end()) {
            return it->second;
        }
        const auto index = static_cast<std::uint32_t>(accum.size());
        push_position(out, p);
        push_color(out, mesh, source);
        accum.push_back(linal::float3{0.0F, 0.0F, 0.0F});
        welded.emplace(key, index);
        return index;
    };

    for (std::size_t t = 0; t + 2U < mesh.triangleIndices.size(); t += 3U) {
        const std::uint32_t i0 = mesh.triangleIndices[t];
        const std::uint32_t i1 = mesh.triangleIndices[t + 1U];
        const std::uint32_t i2 = mesh.triangleIndices[t + 2U];
        if (i0 >= vertexCount || i1 >= vertexCount || i2 >= vertexCount) {
            continue;
        }
        const std::uint32_t w0 = weld(i0);
        const std::uint32_t w1 = weld(i1);
        const std::uint32_t w2 = weld(i2);
        const linal::float3 normal = face_normal(position_at(mesh, i0), position_at(mesh, i1), position_at(mesh, i2));
        accum[w0] = accum[w0] + normal;
        accum[w1] = accum[w1] + normal;
        accum[w2] = accum[w2] + normal;
        out.triangleIndices.push_back(w0);
        out.triangleIndices.push_back(w1);
        out.triangleIndices.push_back(w2);
    }

    for (const linal::float3& n : accum) {
        push_normal(out, safe_normalize(n));
    }

    return out;
}

} // namespace

MeshData apply_shading(const MeshData& mesh, ShadingMode mode) {
    if (mode == ShadingMode::Preserve || mesh.triangleIndices.size() < 3U) {
        return mesh;
    }
    return mode == ShadingMode::Flat ? make_flat(mesh) : make_smooth(mesh);
}

} // namespace renderer
