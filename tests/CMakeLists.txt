cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_CURRENT_LIST_DIR}/../cmake.modules" CACHE STRING "module-path")
project(test CXX)
enable_testing()

include(configure-target)
include(gtest)
include_directories( ${COMP_SOURCES}/tests ${GTEST_INCLUDE_DIRS} )

set (SOURCES
    test_main.cpp
    test_base.hpp
    test_signal.cpp
    test_member_signal.cpp
    test_trackers.cpp
)

add_executable(unittests ${SOURCES})
target_link_libraries(unittests ${COMP_TEST_LIBS} comp_lib)
#target_include_directories(unittests PRIVATE ${COMP_SOURCES}/tests)
configure_target(unittests)
add_test(NAME test-1 COMMAND unittests "--gtest_output=xml:unittest.xml" WORKING_DIRECTORY "${COMP_BUILD_PATH}")
