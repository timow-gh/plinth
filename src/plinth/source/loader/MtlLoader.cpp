#include "plinth/loader/MtlLoader.hpp"

#include <array>
#include <charconv>
#include <cstddef>
#include <filesystem>
#include <string>
#include <string_view>
#include <utility>

namespace renderer {

namespace {

[[nodiscard]] bool is_space(char c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '\f' || c == '\v';
}

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

// Returns the first whitespace-delimited token and the untrimmed remainder of
// the line after it. Used to split a directive keyword from its operand.
[[nodiscard]] std::pair<std::string_view, std::string_view> split_keyword(std::string_view line) {
    const std::string_view trimmed = trim(line);
    std::size_t end = 0;
    while (end < trimmed.size() && !is_space(trimmed[end])) {
        ++end;
    }
    return {trimmed.substr(0, end), trimmed.substr(end)};
}

// Normalizes a Wavefront texture path: collapses doubled separators (Blender
// emits `textures\\name.jpg`), converts backslashes to the platform-native
// separator, and resolves relative to \p baseDir. Interior spaces (common in
// exported filenames) are preserved. An empty or absolute path is used as-is.
[[nodiscard]] std::string resolve_texture_path(std::string_view raw, const std::filesystem::path& baseDir) {
    const std::string_view value = trim(raw);
    if (value.empty()) {
        return {};
    }
    std::string normalized;
    normalized.reserve(value.size());
    char previous = '\0';
    for (const char c: value) {
        const char mapped = (c == '\\') ? '/' : c;
        // Collapse consecutive separators produced by doubled backslashes.
        if (mapped == '/' && previous == '/') {
            continue;
        }
        normalized.push_back(mapped);
        previous = mapped;
    }
    std::filesystem::path path(normalized);
    if (path.is_absolute()) {
        return path.lexically_normal().string();
    }
    return (baseDir / path).lexically_normal().string();
}

[[nodiscard]] bool parse_diffuse_color(std::string_view operand, std::array<float, 4>& color) {
    std::array<float, 3> rgb{};
    std::string_view remainder = trim(operand);
    for (float& component: rgb) {
        if (remainder.empty()) {
            return false;
        }
        const std::size_t tokenEnd = remainder.find_first_of(" \t\r\n\f\v");
        const std::string_view token = remainder.substr(0, tokenEnd);
        const auto [end, error] = std::from_chars(token.data(), token.data() + token.size(), component);
        if (error != std::errc{} || end != token.data() + token.size()) {
            return false;
        }
        remainder = tokenEnd == std::string_view::npos ? std::string_view{} : trim(remainder.substr(tokenEnd));
    }
    color = {rgb[0], rgb[1], rgb[2], 1.0F};
    return true;
}

} // namespace

std::expected<std::vector<MeshData::Material>, LoadError> parse_mtl(std::string_view contents,
                                                                    const std::filesystem::path& baseDir) {
    std::vector<MeshData::Material> materials;

    std::size_t lineStart = 0;
    while (lineStart <= contents.size()) {
        std::size_t lineEnd = contents.find('\n', lineStart);
        if (lineEnd == std::string_view::npos) {
            lineEnd = contents.size();
        }
        const std::string_view line = contents.substr(lineStart, lineEnd - lineStart);
        lineStart = lineEnd + 1;

        const auto [keyword, operand] = split_keyword(line);
        if (keyword.empty() || keyword.starts_with('#')) {
            continue;
        }

        if (keyword == "newmtl") {
            materials.push_back({.name = std::string(trim(operand)),
                                 .diffuseColor = {1.0F, 1.0F, 1.0F, 1.0F},
                                 .diffuseTexturePath = {}});
        } else if (keyword == "Kd" && !materials.empty()) {
            // Keep the default white color when a malformed Kd line is encountered.
            (void)parse_diffuse_color(operand, materials.back().diffuseColor);
        } else if (keyword == "map_Kd" && !materials.empty()) {
            materials.back().diffuseTexturePath = resolve_texture_path(operand, baseDir);
        }
        // All other directives are ignored.
    }

    if (materials.empty()) {
        return std::unexpected(LoadError::empty);
    }
    return materials;
}

} // namespace renderer
