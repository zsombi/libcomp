cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
set( CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_CURRENT_LIST_DIR}/cmake.modules" )
project(sywu)

#option(SYWU_TESTS "Build unit tests." OFF)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

set(SYWU_SOURCES ${CMAKE_CURRENT_SOURCE_DIR})
set(SYWU_BUILD_PATH ${CMAKE_CURRENT_BINARY_DIR})
include(configure-platform)

add_subdirectory(src)

#if (SYWU_TESTS)
    add_subdirectory(tests)
#endif()

