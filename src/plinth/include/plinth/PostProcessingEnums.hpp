#ifndef PLINTH_POSTPROCESSINGENUMS_HPP
#define PLINTH_POSTPROCESSINGENUMS_HPP

namespace renderer {

enum class ToneMapMode { None = 0, Reinhard = 1 };
enum class FogMode { Linear = 0, Exponential = 1 };
enum class VisualizationMode {
    Final = 0, RawHdr = 1, LinearLdr = 2, Luminance = 3,
    LogLuminance = 4, Depth = 5, Overexposure = 6,
    Underexposure = 7, NaNAndInfinity = 8, Grayscale = 9
};

} // namespace renderer

#endif
