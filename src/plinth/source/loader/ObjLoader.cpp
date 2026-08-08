#include "plinth/loader/ObjLoader.hpp"

#include <charconv>
#include <cstdint>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace renderer {

namespace {

// A face corner references a position and, optionally, a texture coordinate and
// a normal by OBJ index.
struct Corner {
    int position{0};  // resolved to 0-based; -1 when absent (never valid for a corner)
    int texcoord{-1}; // resolved to 0-based; -1 when absent
    int normal{-1};   // resolved to 0-based; -1 when absent
};

// Key identifying a unique emitted vertex: a (position, texcoord, normal) tuple.
// Distinct normals or texture coordinates on the same position must become
// distinct vertices so per-vertex normal and uv buffers line up with positions.
struct CornerKey {
    int position{0};
    int texcoord{-1};
    int normal{-1};
    bool operator==(const CornerKey&) const = default;
};

struct CornerKeyHash {
    static constexpr std::uint64_t kHashCombineMagic = 0x9E3779B97F4A7C15ULL;

    std::size_t operator()(const CornerKey& key) const {
        std::uint64_t hash = static_cast<std::uint32_t>(key.position);
        hash = hash * kHashCombineMagic + static_cast<std::uint32_t>(key.texcoord);
        hash = hash * kHashCombineMagic + static_cast<std::uint32_t>(key.normal);
        return std::hash<std::uint64_t>{}(hash);
    }
};

[[nodiscard]] bool is_space(char c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '\f' || c == '\v';
}

// Strips leading and trailing whitespace. Used for directive operands (e.g.
// mtllib / usemtl names) that may contain interior spaces and so cannot be read
// as a single whitespace-delimited token.
[[nodiscard]] std::string_view trim(std::string_view text) {
    std::size_t begin = 0;
    while (begin < text.size() && is_space(text[begin])) {
        ++begin;
    }
    std::size_t end = text.size();
    while (end > begin && is_space(text[end - 1])) {
        --end;
    }
    return text.substr(begin, end - begin);
}

// Advances past leading whitespace and returns the next whitespace-delimited
// token, consuming it from \p cursor. Returns an empty view at end of input.
[[nodiscard]] std::string_view next_token(std::string_view& cursor) {
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

[[nodiscard]] bool parse_float(std::string_view token, float& out) {
    const char* first = token.data();
    const char* last = token.data() + token.size();
    const auto [ptr, ec] = std::from_chars(first, last, out);
    return ec == std::errc{} && ptr == last;
}

// Resolves an OBJ index token to a 0-based index. OBJ indices are 1-based and
// may be negative (relative to the end of the current list). \p count is the
// number of elements parsed so far for that attribute.
[[nodiscard]] bool resolve_index(std::string_view token, std::size_t count, int& out) {
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
// "v/vt/vn" into a Corner. Missing position is a parse error. A present but
// malformed texture-coordinate or normal index is a parse error.
[[nodiscard]] bool parse_corner(std::string_view token,
                                std::size_t positionCount,
                                std::size_t texcoordCount,
                                std::size_t normalCount,
                                Corner& out) {
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
    const std::string_view texcoordTok = rest.substr(0, secondSlash);
    if (!texcoordTok.empty() && !resolve_index(texcoordTok, texcoordCount, out.texcoord)) {
        return false; // "v//vn" leaves texcoordTok empty; a present index must resolve.
    }
    if (secondSlash == std::string_view::npos) {
        return true; // "v/vt": no normal.
    }
    const std::string_view normalTok = rest.substr(secondSlash + 1);
    if (normalTok.empty()) {
        return true; // "v/vt/" (trailing slash) - no normal.
    }
    // "v//vn" or "v/vt/vn": normal present.
    return resolve_index(normalTok, normalCount, out.normal);
}

[[nodiscard]] std::expected<std::vector<Corner>, LoadError> parse_face_corners(std::string_view cursor,
                                                                               std::size_t positionCount,
                                                                               std::size_t texcoordCount,
                                                                               std::size_t normalCount) {
    std::vector<Corner> face;
    for (std::string_view token = next_token(cursor); !token.empty(); token = next_token(cursor)) {
        Corner corner;
        if (!parse_corner(token, positionCount, texcoordCount, normalCount, corner)) {
            return std::unexpected(LoadError::parseError);
        }
        face.push_back(corner);
    }
    if (face.size() < 3U) {
        return std::unexpected(LoadError::parseError);
    }
    return face;
}

// NOLINTBEGIN(readability-function-cognitive-complexity)
[[nodiscard]] std::expected<void, LoadError>
process_obj_keyword(std::string_view keyword,
                    std::string_view cursor,
                    std::vector<std::array<float, 3>>& positions,
                    std::vector<std::array<float, 3>>& normals,
                    std::vector<std::array<float, 2>>& texcoords,
                    std::string& materialLibrary,
                    std::string& currentMaterial,
                    MeshData& mesh,
                    std::unordered_map<CornerKey, std::uint32_t, CornerKeyHash>& emitted,
                    bool haveAnyNormals,
                    bool haveAnyTexcoords,
                    bool& haveOpenSubMesh) {
    const auto emitCorner = [&](const Corner& corner) -> std::uint32_t {
        const CornerKey key{corner.position, corner.texcoord, corner.normal};
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
                mesh.normals.insert(mesh.normals.end(), {0.0F, 0.0F, 1.0F});
            }
        }
        if (haveAnyTexcoords) {
            if (corner.texcoord >= 0) {
                const std::array<float, 2>& uv = texcoords[static_cast<std::size_t>(corner.texcoord)];
                mesh.textureCoordinates.insert(mesh.textureCoordinates.end(), uv.begin(), uv.end());
            } else {
                mesh.textureCoordinates.insert(mesh.textureCoordinates.end(), {0.0F, 0.0F});
            }
        }
        emitted.emplace(key, index);
        return index;
    };

    const auto closeSubMesh = [&] {
        if (!haveOpenSubMesh) {
            return;
        }
        const auto end = static_cast<std::uint32_t>(mesh.triangleIndices.size());
        MeshData::SubMesh& sub = mesh.subMeshes.back();
        sub.indexCount = end - sub.indexOffset;
        if (sub.indexCount == 0U) {
            mesh.subMeshes.pop_back();
        }
        haveOpenSubMesh = false;
    };
    const auto openSubMesh = [&] {
        closeSubMesh();
        mesh.subMeshes.push_back({.indexOffset = static_cast<std::uint32_t>(mesh.triangleIndices.size()),
                                  .indexCount = 0U,
                                  .materialName = currentMaterial});
        haveOpenSubMesh = true;
    };

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
    } else if (keyword == "vt") {
        std::array<float, 2> uv{0.0F, 0.0F};
        bool ok = true;
        for (float& component: uv) {
            ok = ok && parse_float(next_token(cursor), component);
        }
        if (!ok) {
            return std::unexpected(LoadError::parseError);
        }
        texcoords.push_back(uv);
    } else if (keyword == "mtllib") {
        if (materialLibrary.empty()) {
            materialLibrary = std::string(trim(cursor));
        }
    } else if (keyword == "usemtl") {
        currentMaterial = std::string(trim(cursor));
        openSubMesh();
    } else if (keyword == "f") {
        if (!haveOpenSubMesh) {
            openSubMesh();
        }
        auto faceResult = parse_face_corners(cursor, positions.size(), texcoords.size(), normals.size());
        if (!faceResult) {
            return std::unexpected(faceResult.error());
        }
        const auto& face = *faceResult;
        const std::uint32_t anchor = emitCorner(face[0]);
        std::uint32_t previous = emitCorner(face[1]);
        for (std::size_t i = 2; i < face.size(); ++i) {
            const std::uint32_t current = emitCorner(face[i]);
            mesh.triangleIndices.push_back(anchor);
            mesh.triangleIndices.push_back(previous);
            mesh.triangleIndices.push_back(current);
            previous = current;
        }
    }
    return {};
}
// NOLINTEND(readability-function-cognitive-complexity)

} // namespace

std::expected<MeshData, LoadError> ObjLoader::parse(std::string_view rawContents) {
    std::vector<std::array<float, 3>> positions;
    std::vector<std::array<float, 3>> normals;
    std::vector<std::array<float, 2>> texcoords;

    MeshData mesh;
    mesh.sourceName = "obj";

    std::unordered_map<CornerKey, std::uint32_t, CornerKeyHash> emitted;
    const bool haveAnyNormals = rawContents.find("\nvn") != std::string_view::npos || rawContents.starts_with("vn");
    const bool haveAnyTexcoords = rawContents.find("\nvt") != std::string_view::npos || rawContents.starts_with("vt");

    std::string currentMaterial;
    bool haveOpenSubMesh = false;

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

        auto result = process_obj_keyword(keyword,
                                          cursor,
                                          positions,
                                          normals,
                                          texcoords,
                                          mesh.materialLibrary,
                                          currentMaterial,
                                          mesh,
                                          emitted,
                                          haveAnyNormals,
                                          haveAnyTexcoords,
                                          haveOpenSubMesh);
        if (!result) {
            return std::unexpected(result.error());
        }
    }

    if (haveOpenSubMesh) {
        const auto end = static_cast<std::uint32_t>(mesh.triangleIndices.size());
        MeshData::SubMesh& sub = mesh.subMeshes.back();
        sub.indexCount = end - sub.indexOffset;
        if (sub.indexCount == 0U) {
            mesh.subMeshes.pop_back();
        }
    }

    if (mesh.empty()) {
        return std::unexpected(LoadError::empty);
    }
    return mesh;
}

} // namespace renderer
