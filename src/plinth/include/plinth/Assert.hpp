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

namespace plinth_assert_detail {

// The if-statement lives in a dedicated inline function rather than being expanded inline by
// the macro so that callers do not accumulate cognitive complexity for every RENDERER_ASSERT.
[[maybe_unused]] inline void
assert_check(bool condition, const char* file, int line, const char* func, const char* expr) {
    if (!condition) [[unlikely]] {
        std::string msg{file};
        msg += ":";
        msg += std::to_string(line);
        msg += ": internal check failed in '";
        msg += func;
        msg += "': '";
        msg += expr;
        msg += "'\n";
        std::fputs(msg.c_str(), stderr);
        RENDERER_ASSERT_TRAP();
    }
}

} // namespace plinth_assert_detail

#define RENDERER_ASSERT(...)                                                                                           \
    plinth_assert_detail::assert_check((__VA_ARGS__), __FILE__, __LINE__, __func__, #__VA_ARGS__)

#endif

#endif // RENDERER_ASSERT_HPP
