#ifndef OPENGL_POSITION_HPP
#define OPENGL_POSITION_HPP

#include "OpenGL.hpp"
#include "OpenGL/SignedIntId.hpp"
#include "OpenGL/opengl_export.h"

namespace opengl {

class OPENGL_EXPORT Location : public SignedIntId {
  public:
    using SignedIntId::SignedIntId;
};

} // namespace opengl

#endif // OPENGL_POSITION_HPP
