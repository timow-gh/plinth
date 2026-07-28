#include "plinth/loader/ObjLoader.hpp"
#include <charconv>
#include <cstdint>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace renderer {

namespace {

// A face corner references a position and, optionally, a normal by OBJ index.
// Texture coordinate indices are parsed but not retained.
struct Corner {
    int position{0}; // resolved to 0-based; -1 when absent (never valid for a corner)
    int normal{-1};  // resolved to 0-based; -1 when absent
};

// Key identifying a unique emitted vertex: a (position, normal) pair. Distinct
// normals on the same position must become distinct vertices so per-vertex
// normal buffers line up with positions.
struct CornerKey {
    int position{0};
    int normal{-1};
    bool operator==(const CornerKey&) const = default;
};

struct CornerKeyHash {
    std::size_t operator()(const CornerKey& key) const {
        const std::uint64_t packed = (static_cast<std::uint64_t>(static_cast<std::uint32_t>(key.position)) << 32U) ^
                                     static_cast<std::uint32_t>(key.normal);
        return std::hash<std::uint64_t>{}(packed);
    }
};

[[nodiscard]]
bool is_space(char c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '\f' || c == '\v';
}

// Advances past leading whitespace and returns the next whitespace-delimited
// token, consuming it from \p cursor. Returns an empty view at end of input.
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

// Resolves an OBJ index token to a 0-based index. OBJ indices are 1-based and
// may be negative (relative to the end of the current list). \p count is the
// number of elements parsed so far for that attribute.
[[nodiscard]]
bool resolve_index(std::string_view token, std::size_t count, int& out) {
    long value = 0;
    const char* first = token.data();
    const char* last = token.data() + token.size();
    const auto [ptr, ec] = std::from_chars(first, last, value);
    if (ec != std::errc{} || ptr != last || value == 0) {
        return false;
    }
    const long resolved = value > 0 ? value - 1 : static_cast<long>(count) + value;
    if (resolved < 0 || resolved >= static_cast<long>(count)) {
        return false;
    }
    out = static_cast<int>(resolved);
    return true;
}

// Parses a single face vertex token of the form "v", "v/vt", "v//vn", or
// "v/vt/vn" into a Corner. Missing position is a parse error.
[[nodiscard]]
bool parse_corner(std::string_view token, std::size_t positionCount, std::size_t normalCount, Corner& out) {
    const std::size_t firstSlash = token.find('/');
    const std::string_view positionTok = token.substr(0, firstSlash);
    if (!resolve_index(positionTok, positionCount, out.position)) {
        return false;
    }
    if (firstSlash == std::string_view::npos) {
        return true;
    }
    const std::string_view rest = token.substr(firstSlash + 1);
    const std::size_t secondSlash = rest.find('/');
    if (secondSlash == std::string_view::npos) {
        return true; // "v/vt": texture coordinate ignored, no normal.
    }
    const std::string_view normalTok = rest.substr(secondSlash + 1);
    if (normalTok.empty()) {
        return true; // "v/vt/" (trailing slash) - no normal.
    }
    // "v//vn" or "v/vt/vn": normal present.
    return resolve_index(normalTok, normalCount, out.normal);
}

} // namespace

std::expected<MeshData, LoadError> ObjLoader::parse(std::string_view rawContents) const {
    std::vector<std::array<float, 3>> positions;
    std::vector<std::array<float, 3>> normals;

    MeshData mesh;
    mesh.sourceName = "obj";

    std::unordered_map<CornerKey, std::uint32_t, CornerKeyHash> emitted;
    const bool haveAnyNormals = rawContents.find("\nvn") != std::string_view::npos || rawContents.starts_with("vn");

    // Emits (deduplicated) the vertex for a corner and returns its index.
    const auto emit_corner = [&](const Corner& corner) -> std::uint32_t {
        const CornerKey key{corner.position, corner.normal};
        if (const auto it = emitted.find(key); it != emitted.end()) {
            return it->second;
        }
        const auto index = static_cast<std::uint32_t>(mesh.vertices.size() / 3U);
        const std::array<float, 3>& pos = positions[static_cast<std::size_t>(corner.position)];
        mesh.vertices.insert(mesh.vertices.end(), pos.begin(), pos.end());
        if (haveAnyNormals) {
            if (corner.normal >= 0) {
                const std::array<float, 3>& nrm = normals[static_cast<std::size_t>(corner.normal)];
                mesh.normals.insert(mesh.normals.end(), nrm.begin(), nrm.end());
            } else {
                // A face without a normal in a file that otherwise has them:
                // leave a placeholder so buffers stay aligned. The renderer's
                // normal computation only runs when normals are entirely empty,
                // so fill with a neutral up vector here.
                mesh.normals.insert(mesh.normals.end(), {0.0F, 0.0F, 1.0F});
            }
        }
        emitted.emplace(key, index);
        return index;
    };

    std::size_t lineStart = 0;
    while (lineStart <= rawContents.size()) {
        std::size_t lineEnd = rawContents.find('\n', lineStart);
        if (lineEnd == std::string_view::npos) {
            lineEnd = rawContents.size();
        }
        std::string_view line = rawContents.substr(lineStart, lineEnd - lineStart);
        lineStart = lineEnd + 1;

        std::string_view cursor = line;
        const std::string_view keyword = next_token(cursor);
        if (keyword.empty() || keyword.starts_with('#')) {
            continue;
        }

        if (keyword == "v") {
            std::array<float, 3> p{0.0F, 0.0F, 0.0F};
            bool ok = true;
            for (float& component: p) {
                ok = ok && parse_float(next_token(cursor), component);
            }
            if (!ok) {
                return std::unexpected(LoadError::parseError);
            }
            positions.push_back(p);
        } else if (keyword == "vn") {
            std::array<float, 3> n{0.0F, 0.0F, 0.0F};
            bool ok = true;
            for (float& component: n) {
                ok = ok && parse_float(next_token(cursor), component);
            }
            if (!ok) {
                return std::unexpected(LoadError::parseError);
            }
            normals.push_back(n);
        } else if (keyword == "f") {
            std::vector<Corner> face;
            for (std::string_view token = next_token(cursor); !token.empty(); token = next_token(cursor)) {
                Corner corner;
                if (!parse_corner(token, positions.size(), normals.size(), corner)) {
                    return std::unexpected(LoadError::parseError);
                }
                face.push_back(corner);
            }
            if (face.size() < 3U) {
                return std::unexpected(LoadError::parseError);
            }
            // Triangle fan around the first corner triangulates a convex polygon.
            const std::uint32_t anchor = emit_corner(face[0]);
            std::uint32_t previous = emit_corner(face[1]);
            for (std::size_t i = 2; i < face.size(); ++i) {
                const std::uint32_t current = emit_corner(face[i]);
                mesh.triangleIndices.push_back(anchor);
                mesh.triangleIndices.push_back(previous);
                mesh.triangleIndices.push_back(current);
                previous = current;
            }
        }
        // Other directives (vt, mtllib, usemtl, g, o, s, ...) are ignored.
    }

    if (mesh.empty()) {
        return std::unexpected(LoadError::empty);
    }
    return mesh;
}

} // namespace renderer
