#ifndef OPENGL_PICKID_HPP
#define OPENGL_PICKID_HPP

#include <array>
#include <cstdint>

namespace opengl {

// Color-ID picking encodes a small, per-pass sequential index (not the 64-bit DrawableId) into the
// low 24 bits of an RGB8 color. Index 0 is reserved for "no hit" (the pick framebuffer is cleared to
// black), so the first drawable in a pass is assigned index 1. 24 bits addresses ~16.7M drawables
// per pass, far beyond any realistic scene.
inline constexpr std::uint32_t maxPickIndex = 0x00FFFFFFu;

// Encode a pass index into a normalized RGB triple suitable for a shader uniform. Each channel holds
// 8 bits: r = bits[0..7], g = bits[8..15], b = bits[16..23]. Dividing by 255 maps each byte to the
// exact float value glReadPixels returns when read back as GL_UNSIGNED_BYTE.
[[nodiscard]] inline constexpr std::array<float, 3> encode_pick_index(std::uint32_t index) noexcept {
    const auto r = static_cast<float>(index & 0xFFu);
    const auto g = static_cast<float>((index >> 8) & 0xFFu);
    const auto b = static_cast<float>((index >> 16) & 0xFFu);
    return {r / 255.0f, g / 255.0f, b / 255.0f};
}

// Decode an RGB8 pixel (as read back by glReadPixels) into a pass index. Returns 0 for the reserved
// "no hit" clear color.
[[nodiscard]] inline constexpr std::uint32_t
decode_pick_index(std::uint8_t r, std::uint8_t g, std::uint8_t b) noexcept {
    return static_cast<std::uint32_t>(r) | (static_cast<std::uint32_t>(g) << 8) | (static_cast<std::uint32_t>(b) << 16);
}

} // namespace opengl

#endif // OPENGL_PICKID_HPP
