cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
include(configure-target)

project(sywu_lib VERSION 1.0.0 LANGUAGES CXX)

# get the files
include(files.cmake)

# BUILD_SHARED_LIBS drives the library type for add_Library
add_library(sywu "")
target_sources(sywu
    PRIVATE ${SOURCES} ${PRIVATE_HEADERS}
    PUBLIC ${HEADERS})
#target_include_directories(sywu PUBLIC ${SYWU_HEADER_PATH})
configure_target(sywu)
