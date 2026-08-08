#ifndef PLINTH_MESHCULLFACEMODE_HPP
#define PLINTH_MESHCULLFACEMODE_HPP

namespace renderer {

enum class MeshCullFaceMode {
    BACK,  // default: cull back faces (GL_BACK)
    FRONT, // cull front faces (GL_FRONT)
    NONE,  // disable culling (draw both sides)
};

} // namespace renderer

#endif // PLINTH_MESHCULLFACEMODE_HPP
