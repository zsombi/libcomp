if (CONFIG_PLATFORM)
    return()
endif()
set(CONFIG_PLATFORM true)

function (print_config arg_target)
    get_target_property(target_type ${arg_target} TYPE)
    get_target_property(ldefs ${arg_target} LINK_FLAGS)
    get_target_property(lopts ${arg_target} LINK_OPTIONS)
    get_target_property(cdefs ${arg_target} COMPILE_DEFINITIONS)
    get_target_property(copts ${arg_target} COMPILE_OPTIONS)
    message(STATUS "---" ${arg_target} " build summary ---")
    message(STATUS "target_type     : ${target_type}")
    message(STATUS "System          : ${CMAKE_SYSTEM_NAME}")
    message(STATUS "Build           : ${CMAKE_BUILD_TYPE}")
    message(STATUS "LibDir          : ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")
    message(STATUS "ArcDir          : ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}")
    message(STATUS "cxx             : ${CMAKE_CXX_COMPILER_ID}")
    message(STATUS "cxxflags        : ${CMAKE_CXX_FLAGS}")
    message(STATUS "cdefs           : ${cdefs}")
    message(STATUS "copts           : ${copts}")
    message(STATUS "ldefs           : ${ldefs}")
    message(STATUS "lopts           : ${lopts}")
    message(STATUS "--- End " ${arg_target} " build summary ---")
endfunction()

# function available from 3.13.5 onwards
function(target_link_options arg_target arg_scope)
    set(tmp "")
    foreach(arg IN LISTS ARGN)
        set(tmp "${tmp} ${arg}")
    endforeach()
    set_target_properties(${arg_target} PROPERTIES LINK_OPTIONS ${tmp})
endfunction()


if(CMAKE_SYSTEM_NAME MATCHES "Darwin")
    if(NOT DEFINED CMAKE_MACOSX_RPATH)
        set(CMAKE_MACOSX_RPATH ON)
    endif()
endif()

set(CONFIG_HEADER_PATH ${PROJECT_SOURCE_DIR}/include)
set(CONFIG_LIB_PATH ${PROJECT_BINARY_DIR}/lib/${CMAKE_SYSTEM_NAME})
set(CONFIG_BIN_PATH ${PROJECT_BINARY_DIR}/bin)
set(GTEST_DOWNLOAD_DIR ${PROJECT_SOURCE_DIR}/3rdparty/gtest)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CONFIG_LIB_PATH})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CONFIG_LIB_PATH})

set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 17 CACHE STRING "C++ standard")
set(CMAKE_CXX_STANDARD_REQUIRED ON)
