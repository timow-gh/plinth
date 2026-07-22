#ifndef RENDERER_TEXTURE_HPP
#define RENDERER_TEXTURE_HPP

#include <plinth/plinth_export.h>
#include <cstdint>
#include <span>

namespace renderer {

enum class TextureColorSpace { linear, srgb };
enum class TextureFilter { nearest, linear };

struct PLINTH_EXPORT TextureData {
    /// RGBA8 pixel data is copied during Renderer::create_texture_2d; the span
    /// need only remain valid for that call. It must contain width * height * 4 bytes.
    std::uint32_t width{};
    std::uint32_t height{};
    std::span<const std::uint8_t> rgba8{};
    TextureColorSpace colorSpace{TextureColorSpace::srgb};
    TextureFilter magnificationFilter{TextureFilter::linear};
};

struct PLINTH_EXPORT TextureHandle {
    /// Handles are renderer-specific. Invalid, foreign, removed, and stale
    /// handles are rejected by texture and textured-mesh operations.
    std::uint64_t id{};
    // Opaque identity of the Renderer that created this handle.
    std::uint64_t rendererInstance{};
    [[nodiscard]] constexpr bool is_valid() const noexcept { return id != 0U && rendererInstance != 0U; }
};

} // namespace renderer

#endif // RENDERER_TEXTURE_HPP
