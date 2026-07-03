include(FetchContent)

if (${PROJECT_NAME}_TESTS)
  add_subdirectory(googletest)
endif ()

add_subdirectory(fmt)
add_subdirectory(linal)
