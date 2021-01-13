cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
include(configure-target)

project(comp_lib VERSION 1.0.0 LANGUAGES CXX)

# get the files
include(files.cmake)

# BUILD_SHARED_LIBS drives the library type for add_Library
add_library(comp_lib "")
target_sources(comp_lib
    PRIVATE ${SOURCES} ${PRIVATE_HEADERS}
    PUBLIC ${HEADERS})

#target_include_directories(comp PUBLIC ${COMP_HEADER_PATH})
configure_target(comp_lib)
