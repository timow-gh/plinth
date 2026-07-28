#ifndef RENDERER_LOADER_MESHDATA_HPP
#define RENDERER_LOADER_MESHDATA_HPP

#include "plinth/plinth_export.h"
#include <cstdint>
#include <string>
#include <vector>

namespace renderer {

struct PLINTH_EXPORT MeshData {
    std::vector<float> vertices;
    std::vector<std::uint32_t> triangleIndices;
    std::vector<float> normals;
    std::vector<float> colors;
    std::string sourceName;

    [[nodiscard]]
    bool empty() const {
        return vertices.empty();
    }
};

} // namespace renderer

#endif // RENDERER_LOADER_MESHDATA_HPP
