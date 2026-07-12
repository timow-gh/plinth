#ifndef PLINTH_FRAMESTATE_HPP
#define PLINTH_FRAMESTATE_HPP

namespace renderer {

struct ClearColor {
    float r{0.0f};
    float g{0.0f};
    float b{0.0f};
    float a{1.0f};
};

struct ViewportRect {
    int x{0};
    int y{0};
    int width{0};
    int height{0};
};

} // namespace renderer

#endif // PLINTH_FRAMESTATE_HPP
