#ifndef RENDERER_COLOR_HPP
#define RENDERER_COLOR_HPP

#include <array>
#include <cstddef>

namespace renderer {

inline constexpr std::size_t ColorChannelCount = 4;
using Color = std::array<float, ColorChannelCount>;

} // namespace renderer

#endif // RENDERER_COLOR_HPP
