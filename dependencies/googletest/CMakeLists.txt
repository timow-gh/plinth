CPMAddPackage(
        NAME googletest
        GITHUB_REPOSITORY google/googletest
        GIT_TAG v1.17.0
        GIT_SHALLOW TRUE
        OPTIONS 
        "BUILD_SHARED_LIBS OFF"
        "INSTALL_GTEST OFF"
        EXCLUDE_FROM_ALL YES
)

if(googletest_ADDED)
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
        target_compile_options(gtest_main INTERFACE
                "-Wno-weak-vtables"
                )
    endif ()
endif()