cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
set( CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_CURRENT_LIST_DIR}/cmake.modules" )
project(comp)

option(COMP_TESTS "Build unit tests." OFF)
option(COMP_EXAMPLES "Build examples." OFF)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

set(COMP_SOURCES ${CMAKE_CURRENT_SOURCE_DIR})
set(COMP_BUILD_PATH ${CMAKE_CURRENT_BINARY_DIR})
include(configure-platform)

add_subdirectory(src)

if (COMP_EXAMPLES)
    add_subdirectory(examples)
endif()

if (COMP_TESTS)
    add_subdirectory(tests)
endif()

