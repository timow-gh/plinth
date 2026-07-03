# Force fmt to always build as a static library regardless of the global
# BUILD_SHARED_LIBS setting.  fmt is a private implementation detail of geoqik
# (its types never appear in the public C API), so making it shared would only
# produce a stray fmtd.dll that consumers cannot easily redistribute.
set(_saved_BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS})
set(BUILD_SHARED_LIBS OFF)

CPMAddPackage(
        NAME fmt
        GITHUB_REPOSITORY "fmtlib/fmt"
        GIT_TAG 11.2.0
        GIT_SHALLOW TRUE
        OPTIONS
        "FMT_INSTALL OFF"
        "CMAKE_POSITION_INDEPENDENT_CODE ON"
)

set(BUILD_SHARED_LIBS ${_saved_BUILD_SHARED_LIBS})

