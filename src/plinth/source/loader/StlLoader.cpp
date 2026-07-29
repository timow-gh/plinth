#include "plinth/loader/StlLoader.hpp"
#include <array>
#include <charconv>
#include <cstdint>
#include <cstring>
#include <numeric>
#include <string_view>

namespace renderer {

namespace {

constexpr std::size_t kBinaryHeaderSize = 80;
constexpr std::size_t kBinaryCountSize = 4;
constexpr std::size_t kBinaryTriangleSize = 50; // 12 floats (normal + 3 verts) + 2-byte attribute
constexpr std::ptrdiff_t kBinaryFloatSize = 4;
constexpr std::ptrdiff_t kBinaryVertex0Offset = 12; // offset of first vertex from triangle start
constexpr std::ptrdiff_t kBinaryVertexStride = 12;  // bytes between consecutive vertices
constexpr std::ptrdiff_t kBinaryVertexZOffset = 8;   // offset of z component from vertex start
constexpr std::size_t kVertexFloatCount = 9;         // 3 vertices * 3 floats per vertex

[[nodiscard]]
bool is_space(char c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '\f' || c == '\v';
}

[[nodiscard]]
std::string_view next_token(std::string_view& cursor) {
    std::size_t begin = 0;
    while (begin < cursor.size() && is_space(cursor[begin])) {
        ++begin;
    }
    std::size_t end = begin;
    while (end < cursor.size() && !is_space(cursor[end])) {
        ++end;
    }
    const std::string_view token = cursor.substr(begin, end - begin);
    cursor.remove_prefix(end);
    return token;
}

[[nodiscard]]
bool parse_float(std::string_view token, float& out) {
    const char* first = token.data();
    const char* last = token.data() + token.size();
    const auto [ptr, ec] = std::from_chars(first, last, out);
    return ec == std::errc{} && ptr == last;
}

[[nodiscard]]
float read_le_float(const char* bytes) {
    // STL binary is little-endian IEEE-754. Copy through memcpy to avoid
    // aliasing/alignment issues; hosts are assumed little-endian (as are all
    // supported targets).
    float value = 0.0F;
    std::memcpy(&value, bytes, sizeof(float));
    return value;
}

[[nodiscard]]
std::uint32_t read_le_uint32(const char* bytes) {
    std::uint32_t value = 0;
    std::memcpy(&value, bytes, sizeof(std::uint32_t));
    return value;
}

// Binary STL is detected by size: an 80-byte header, a 4-byte triangle count,
// then exactly count * 50 bytes. ASCII files rarely satisfy this exactly.
[[nodiscard]]
bool looks_binary(std::string_view data) {
    if (data.size() < kBinaryHeaderSize + kBinaryCountSize) {
        return false;
    }
    const std::uint32_t count = read_le_uint32(data.data() + kBinaryHeaderSize);
    const std::size_t expected =
        kBinaryHeaderSize + kBinaryCountSize + (static_cast<std::size_t>(count) * kBinaryTriangleSize);
    return data.size() == expected;
}

void push_vec3(MeshData& mesh, const std::array<float, 3>& v) {
    mesh.vertices.insert(mesh.vertices.end(), v.begin(), v.end());
}

void push_normal(MeshData& mesh, const std::array<float, 3>& n) {
    mesh.normals.insert(mesh.normals.end(), n.begin(), n.end());
}

[[nodiscard]]
std::expected<MeshData, LoadError> parse_binary(std::string_view data) {
    MeshData mesh;
    mesh.sourceName = "stl";
    const std::uint32_t count = read_le_uint32(data.data() + kBinaryHeaderSize);
    const char* triangle = data.data() + kBinaryHeaderSize + kBinaryCountSize;
    for (std::uint32_t t = 0; t < count; ++t) {
        const std::array<float, 3> normal{read_le_float(triangle),
                                          read_le_float(triangle + 4),
                                          read_le_float(triangle + 8)};
        for (int v = 0; v < 3; ++v) {
            const char* vertex = triangle + kBinaryVertex0Offset + (static_cast<std::ptrdiff_t>(v) * kBinaryVertexStride);
            push_vec3(mesh, {read_le_float(vertex), read_le_float(vertex + kBinaryFloatSize), read_le_float(vertex + kBinaryVertexZOffset)});
            push_normal(mesh, normal);
        }
        triangle += kBinaryTriangleSize;
    }
    if (mesh.empty()) {
        return std::unexpected(LoadError::empty);
    }
    mesh.triangleIndices.resize(mesh.vertices.size() / 3U);
    std::iota(mesh.triangleIndices.begin(), mesh.triangleIndices.end(), 0U);
    return mesh;
}

[[nodiscard]]
std::expected<MeshData, LoadError> parse_ascii(std::string_view data) {
    MeshData mesh;
    mesh.sourceName = "stl";
    std::array<float, 3> facetNormal{0.0F, 0.0F, 1.0F};

    std::size_t lineStart = 0;
    while (lineStart <= data.size()) {
        std::size_t lineEnd = data.find('\n', lineStart);
        if (lineEnd == std::string_view::npos) {
            lineEnd = data.size();
        }
        std::string_view cursor = data.substr(lineStart, lineEnd - lineStart);
        lineStart = lineEnd + 1;

        const std::string_view keyword = next_token(cursor);
        if (keyword == "facet") {
            // "facet normal nx ny nz"
            (void)next_token(cursor); // "normal"
            bool ok = true;
            for (float& c: facetNormal) {
                ok = ok && parse_float(next_token(cursor), c);
            }
            if (!ok) {
                return std::unexpected(LoadError::parseError);
            }
        } else if (keyword == "vertex") {
            std::array<float, 3> v{0.0F, 0.0F, 0.0F};
            bool ok = true;
            for (float& c: v) {
                ok = ok && parse_float(next_token(cursor), c);
            }
            if (!ok) {
                return std::unexpected(LoadError::parseError);
            }
            push_vec3(mesh, v);
            push_normal(mesh, facetNormal);
        }
        // solid/outer/endloop/endfacet/endsolid ignored.
    }

    if (mesh.empty()) {
        return std::unexpected(LoadError::empty);
    }
    if (mesh.vertices.size() % kVertexFloatCount != 0U) {
        // Vertices must group into whole triangles (3 vertices * 3 floats).
        return std::unexpected(LoadError::parseError);
    }
    mesh.triangleIndices.resize(mesh.vertices.size() / 3U);
    std::iota(mesh.triangleIndices.begin(), mesh.triangleIndices.end(), 0U);
    return mesh;
}

} // namespace

std::expected<MeshData, LoadError> StlLoader::parse(std::string_view rawContents) {
    if (rawContents.empty()) {
        return std::unexpected(LoadError::empty);
    }
    return looks_binary(rawContents) ? parse_binary(rawContents) : parse_ascii(rawContents);
}

} // namespace renderer
