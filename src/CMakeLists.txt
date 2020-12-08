cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_CURRENT_LIST_DIR}/../cmake.modules" CACHE STRING "module-path")

include(configure-target)

project(sywu_lib VERSION 1.0.0 LANGUAGES CXX)

# get the files
include(files.cmake)

# BUILD_SHARED_LIBS drives the library type for add_Library
add_library(sywu "")
target_sources(sywu
    PRIVATE ${SOURCES} ${PRIVATE_HEADERS}
    PUBLIC ${HEADERS} ${CMAKE_CURRENT_SOURCE_DIR}/../include/config.hpp)
#target_include_directories(sywu PUBLIC ${SYWU_HEADER_PATH})
configure_target(sywu)
