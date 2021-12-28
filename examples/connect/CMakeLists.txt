cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_CURRENT_LIST_DIR}/../cmake.modules" CACHE STRING "module-path")
set(PROJECT connect)
project(${PROJECT} CXX)

include(configure-target)

set (SOURCES
    example_connect.cpp
)

add_executable(${PROJECT} ${SOURCES})
target_link_libraries(connect ${COMP_TEST_LIBS} comp_lib)
#target_include_directories(unittests PRIVATE ${COMP_SOURCES}/tests)
configure_target(${PROJECT})
