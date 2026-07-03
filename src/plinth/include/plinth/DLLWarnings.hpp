#ifndef RENDERER_DLL_WARNINGS_HPP
#define RENDERER_DLL_WARNINGS_HPP

#ifdef _MSC_VER

#define RENDERER_SUPPRESS_DLL_INTERFACE_WARNINGS_BEGIN                                                                 \
    __pragma(warning(push)) __pragma(warning(disable : 4251)) /* class 'std::xxx' needs to have dll-interface */       \
        __pragma(warning(disable : 4275))                     /* non dll-interface class 'std::xxx' used as base */

#define RENDERER_SUPPRESS_DLL_INTERFACE_WARNINGS_END __pragma(warning(pop))

#define RENDERER_SUPPRESS_STL_DLL_WARNINGS_BEGIN RENDERER_SUPPRESS_DLL_INTERFACE_WARNINGS_BEGIN
#define RENDERER_SUPPRESS_STL_DLL_WARNINGS_END RENDERER_SUPPRESS_DLL_INTERFACE_WARNINGS_END

#else
#define RENDERER_SUPPRESS_DLL_INTERFACE_WARNINGS_BEGIN
#define RENDERER_SUPPRESS_DLL_INTERFACE_WARNINGS_END
#define RENDERER_SUPPRESS_STL_DLL_WARNINGS_BEGIN
#define RENDERER_SUPPRESS_STL_DLL_WARNINGS_END
#endif

#endif // RENDERER_DLL_WARNINGS_HPP
