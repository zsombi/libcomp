cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_CURRENT_LIST_DIR}/../cmake.modules" CACHE STRING "module-path")
set(PROJECT track_tracker)
project(${PROJECT} CXX)

include(configure-target)

set (SOURCES
    example.cpp
)

add_executable(${PROJECT} ${SOURCES})
target_link_libraries(track_tracker ${COMP_TEST_LIBS} comp_lib)
configure_target(${PROJECT})
