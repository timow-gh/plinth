#ifndef RENDERER_COLORCONVERSION_HPP
#define RENDERER_COLORCONVERSION_HPP

#include <cmath>
#include <cstddef>
#include <plinth/Color.hpp>
#include <span>
#include <vector>

namespace renderer {

// sRGB <-> linear transfer functions using the exact piecewise IEC 61966-2-1
// definition (not a pow(x, 2.2) approximation). linear_to_srgb is the inverse of
// srgb_to_linear and matches the srgbEncode() used by the post-processing shader,
// so a value round-trips through the render pipeline unchanged.

inline float srgb_to_linear(float c) {
    if (c <= 0.04045F) {
        return c / 12.92F;
    }
    return std::pow((c + 0.055F) / 1.055F, 2.4F);
}

inline float linear_to_srgb(float c) {
    if (c <= 0.0031308F) {
        return c * 12.92F;
    }
    return 1.055F * std::pow(c, 1.0F / 2.4F) - 0.055F;
}

// Convert the RGB channels of an RGBA color from sRGB to linear, leaving alpha
// (index 3) untouched.
inline Color srgb_to_linear(const Color& color) {
    return Color{srgb_to_linear(color[0]), srgb_to_linear(color[1]), srgb_to_linear(color[2]), color[3]};
}

// Convert a packed RGBA color array (stride ColorChannelCount) from sRGB to
// linear in place, leaving every alpha component untouched.
inline void srgb_to_linear_inplace(std::span<float> packedRgba) {
    for (std::size_t i = 0; i + ColorChannelCount <= packedRgba.size(); i += ColorChannelCount) {
        packedRgba[i + 0] = srgb_to_linear(packedRgba[i + 0]);
        packedRgba[i + 1] = srgb_to_linear(packedRgba[i + 1]);
        packedRgba[i + 2] = srgb_to_linear(packedRgba[i + 2]);
        // packedRgba[i + 3] (alpha) is passed through unchanged.
    }
}

// Return a linearized copy of a packed RGBA color array (stride
// ColorChannelCount), leaving every alpha component untouched.
inline std::vector<float> srgb_to_linear_copy(std::span<const float> packedRgba) {
    std::vector<float> result(packedRgba.begin(), packedRgba.end());
    srgb_to_linear_inplace(result);
    return result;
}

} // namespace renderer

#endif // RENDERER_COLORCONVERSION_HPP
