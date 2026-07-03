#ifndef RENDERER_PICKRAY_HPP
#define RENDERER_PICKRAY_HPP

#include <linal/linal.hpp>

namespace renderer {

struct PickRay {
    linal::double3 origin;
    linal::double3 direction;
};

} // namespace renderer

#endif // RENDERER_PICKRAY_HPP
