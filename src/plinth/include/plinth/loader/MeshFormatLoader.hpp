#ifndef RENDERER_LOADER_MESHFORMATLOADER_HPP
#define RENDERER_LOADER_MESHFORMATLOADER_HPP

#include "plinth/loader/MeshData.hpp"
#include "plinth/loader/MeshLoader.hpp"
#include <algorithm>
#include <cctype>
#include <concepts>
#include <expected>
#include <ranges>
#include <string>
#include <string_view>

namespace renderer {

template <typename T>
concept MeshFormatLoader = requires(const T loader, std::string_view rawContents) {
    { T::extensions() } -> std::ranges::range;
    requires std::convertible_to<std::ranges::range_value_t<decltype(T::extensions())>, std::string_view>;
    { loader.parse(rawContents) } -> std::same_as<std::expected<MeshData, LoadError>>;
};

template <MeshFormatLoader... Loaders>
struct MeshLoaderList {};

namespace detail {

[[nodiscard]]
inline bool extension_matches(std::string_view claimed, std::string_view actual) {
    return std::ranges::equal(claimed, actual, [](char lhs, char rhs) {
        return std::tolower(static_cast<unsigned char>(lhs)) == std::tolower(static_cast<unsigned char>(rhs));
    });
}

template <MeshFormatLoader Loader>
[[nodiscard]]
bool loader_claims(std::string_view extension) {
    for (const std::string_view claimed: Loader::extensions()) {
        if (extension_matches(claimed, extension)) {
            return true;
        }
    }
    return false;
}

} // namespace detail

/// Dispatches parsing to the first loader in the list whose extensions() claims
/// \p extension. Returns LoadError::unsupportedFormat when no loader matches.
/// The fold over the parameter pack is the dispatch: no runtime registry, no
/// virtual calls, and unlisted formats are a compile-time-closed set.
template <MeshFormatLoader... Loaders>
[[nodiscard]]
std::expected<MeshData, LoadError>
load_mesh_with(MeshLoaderList<Loaders...> /*loaders*/, std::string_view extension, std::string_view rawContents) {
    std::expected<MeshData, LoadError> result = std::unexpected(LoadError::unsupportedFormat);
    // Short-circuiting fold: stop at the first loader that claims the extension.
    const bool handled = (... || [&] {
        if (detail::loader_claims<Loaders>(extension)) {
            result = Loaders{}.parse(rawContents);
            return true;
        }
        return false;
    }());
    (void)handled;
    return result;
}

} // namespace renderer

#endif // RENDERER_LOADER_MESHFORMATLOADER_HPP
