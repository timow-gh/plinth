#ifndef PLINTH_LIGHTINGCONFIG_HPP
#define PLINTH_LIGHTINGCONFIG_HPP

#include <linal/vec.hpp>

namespace renderer {

struct LightingConfig {
    // Key light: slightly up-and-right of the camera.
    linal::float3 lightPosition{4.0F, 6.0F, 10.0F};
    linal::float3 lightColor{1.0F, 1.0F, 1.0F};
    // Fill light from the opposite side (down-and-left) so shadowed faces never
    // go fully black -- essential for viewing geometry.
    linal::float3 fillLightDir{-0.5F, -0.4F, 0.6F};
    linal::float3 fillLightColor{0.25F, 0.28F, 0.35F};
    // Ambient floor so nothing is pure black; kept low so it doesn't wash out form.
    linal::float3 ambientColor{0.30F, 0.30F, 0.32F};
    // Moderately tight highlight: enough to convey a machined/plastic surface,
    // not so sharp it flickers on tessellated meshes.
    float shininess{1.0F};
    // Constant attenuation only -- models are inspected at arbitrary
    // distances, so a point light should not dim with range.
    linal::float3 lightAttenuation{1.0F, 0.0F, 0.0F};
    linal::float3 materialAmbient{1.0F, 1.0F, 1.0F};
    linal::float3 materialDiffuse{0.5F, 0.5F, 0.5F};
    linal::float3 materialSpecular{0.2F, 0.2F, 0.2F};
};

} // namespace renderer

#endif // PLINTH_LIGHTINGCONFIG_HPP
