#ifndef RENDERER_ASSERT_HPP
#define RENDERER_ASSERT_HPP

#include <cstdio>
#include <string>

#if defined(_WIN32) && defined(__has_include) && __has_include(<intrin.h>)
#include <intrin.h>
#define RENDERER_ASSERT_TRAP() ::__debugbreak()
#elif defined(__GNUC__) || defined(__clang__)
#define RENDERER_ASSERT_TRAP() __builtin_trap()
#else
#define RENDERER_ASSERT_TRAP() ::std::abort()
#endif

#if defined(NDEBUG) && NDEBUG
#define RENDERER_ASSERT(...)
#else
#define RENDERER_ASSERT(...)                                                                                           \
    do {                                                                                                               \
        if (__VA_ARGS__) [[likely]] {                                                                                  \
        } else {                                                                                                       \
            const std::string _renderer_assert_msg = std::string(__FILE__) + ":" + std::to_string(__LINE__) +          \
                                                     ": internal check failed in '" + __func__ +                       \
                                                     "': '" #__VA_ARGS__ "'\n";                                        \
            std::fputs(_renderer_assert_msg.c_str(), stderr);                                                          \
            RENDERER_ASSERT_TRAP();                                                                                    \
        }                                                                                                              \
    } while (false)
#endif

#endif // RENDERER_ASSERT_HPP
