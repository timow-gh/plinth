cmake_minimum_required(VERSION 3.26)

project(plinth_fetchcontent_test
        VERSION 0.1.0
        LANGUAGES CXX)

if (MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif ()

if (NOT DEFINED PLINTH_SOURCE_DIR OR NOT IS_DIRECTORY "${PLINTH_SOURCE_DIR}")
    message(FATAL_ERROR "PLINTH_SOURCE_DIR must name the Plinth source directory")
endif ()

include(FetchContent)

# Exercise the consumer-facing FetchContent path without reusing the parent build.
set(plinth_EXAMPLES OFF CACHE BOOL "" FORCE)
set(plinth_INSTALL OFF CACHE BOOL "" FORCE)
set(plinth_PACK OFF CACHE BOOL "" FORCE)
set(plinth_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_Declare(plinth SOURCE_DIR "${PLINTH_SOURCE_DIR}")
FetchContent_MakeAvailable(plinth)

add_executable(use_fetchcontent main.cpp)
target_link_libraries(use_fetchcontent PRIVATE plinth::plinth)
