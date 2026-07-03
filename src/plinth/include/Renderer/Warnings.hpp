#ifndef RENDERER_WARNINGS_HPP
#define RENDERER_WARNINGS_HPP

#define RENDERER_DO_PRAGMA(X) _Pragma(#X)

#if defined(_MSC_VER)
#define RENDERER_DISABLE_ALL_WARNINGS RENDERER_DO_PRAGMA(warning(push, 0))
#elif defined(__clang__)
#define RENDERER_DISABLE_ALL_WARNINGS                                                                                  \
    RENDERER_DO_PRAGMA(clang diagnostic push)                                                                          \
    RENDERER_DO_PRAGMA(clang diagnostic ignored "-Weverything")
#elif defined(__GNUC__)
#define RENDERER_DISABLE_ALL_WARNINGS                                                                                  \
    RENDERER_DO_PRAGMA(GCC diagnostic push)                                                                            \
    RENDERER_DO_PRAGMA(GCC diagnostic ignored "-Wall")                                                                 \
    RENDERER_DO_PRAGMA(GCC diagnostic ignored "-Wextra")                                                               \
    RENDERER_DO_PRAGMA(GCC diagnostic ignored "-Wpedantic")                                                            \
    RENDERER_DO_PRAGMA(GCC diagnostic ignored "-Wuseless-cast")                                                        \
    RENDERER_DO_PRAGMA(GCC diagnostic ignored "-Wold-style-cast")                                                      \
    RENDERER_DO_PRAGMA(GCC diagnostic ignored "-Wsign-conversion")                                                     \
    RENDERER_DO_PRAGMA(GCC diagnostic ignored "-Wconversion")                                                          \
    RENDERER_DO_PRAGMA(GCC diagnostic ignored "-Wdouble-promotion")                                                    \
    RENDERER_DO_PRAGMA(GCC diagnostic ignored "-Wnull-dereference")
#endif

#if defined(_MSC_VER)
#define RENDERER_ENABLE_ALL_WARNINGS RENDERER_DO_PRAGMA(warning(pop))
#elif defined(__clang__)
#define RENDERER_ENABLE_ALL_WARNINGS RENDERER_DO_PRAGMA(clang diagnostic pop)
#elif defined(__GNUC__)
#define RENDERER_ENABLE_ALL_WARNINGS RENDERER_DO_PRAGMA(GCC diagnostic pop)
#endif

#endif // RENDERER_WARNINGS_HPP
