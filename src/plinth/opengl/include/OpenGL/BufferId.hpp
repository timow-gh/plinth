#ifndef OPENGL_BUFFERID_HPP
#define OPENGL_BUFFERID_HPP

#include "OpenGL/opengl_export.h"
#include <OpenGL/UnsignedIntId.hpp>

namespace opengl {

class OPENGL_EXPORT BufferId : public UnsignedIntId {
  public:
    using UnsignedIntId::UnsignedIntId;
};

} // namespace opengl

#endif // OPENGL_BUFFERID_HPP
