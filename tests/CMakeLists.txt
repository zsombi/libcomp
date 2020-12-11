cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_CURRENT_LIST_DIR}/../cmake.modules" CACHE STRING "module-path")
project(test CXX)
enable_testing()

include(configure-target)
include(gtest)
include_directories( ${SYWU_SOURCES}/tests ${GTEST_INCLUDE_DIRS} )

set (SOURCES
    test_main.cpp
    test_base.hpp
    test_signal.cpp
    test_member_signal.cpp
)

add_executable(unittests ${SOURCES})
target_link_libraries(unittests ${SYWU_TEST_LIBS} sywu)
#target_include_directories(unittests PRIVATE ${SYWU_SOURCES}/tests)
configure_target(unittests)
add_test(NAME test-1 COMMAND unittests "--gtest_output=xml:unittest.xml" WORKING_DIRECTORY "${SYWU_BUILD_PATH}")
