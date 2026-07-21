#ifndef PLINTH_BUFFERACCESSPATTERN_HPP
#define PLINTH_BUFFERACCESSPATTERN_HPP

namespace renderer {

enum class BufferAccessPattern {
    // Data is replaced and consumed at most once.
    Stream,
    // Data is uploaded once and used many times.
    Static,
    // Data is replaced repeatedly and used many times between updates.
    Dynamic,
};

} // namespace renderer

#endif // PLINTH_BUFFERACCESSPATTERN_HPP
