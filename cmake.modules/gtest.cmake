cmake_minimum_required(VERSION 3.6 FATAL_ERROR)

find_package(GTest)
if (${GTEST_FOUND})
    message( "Using GTest from System")
    set ( SYWU_TEST_LIBS  GTest::GTest GTest::Main )
else()

    #######################################
    # START OF GTEST DOWNLOAD
    #######################################
    include(configure-platform)

    # Do we have google test already downloaded?
    find_path(GTEST_PATH CMakeLists.txt PATHS ${GTEST_DOWNLOAD_DIR})

    if (NOT GTEST_PATH)
        message("Downloading GoogleTest to " ${GTEST_DOWNLOAD_DIR})
        # Download and unpack googletest at configure time
        configure_file(${CONFIG_CMAKE_MODULE_PATH}/gtest.in ${GTEST_DOWNLOAD_DIR}/CMakeLists.txt)
        execute_process(COMMAND "${CMAKE_COMMAND}" -G "${CMAKE_GENERATOR}" .
            WORKING_DIRECTORY "${GTEST_DOWNLOAD_DIR}"
        )
        execute_process(COMMAND "${CMAKE_COMMAND}" --build .
            WORKING_DIRECTORY "${GTEST_DOWNLOAD_DIR}"
        )

        # Prevent GoogleTest from overriding our compiler/linker options
        # when building with Visual Studio
        set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

        find_path(GTEST_PATH CMakeLists.txt PATHS ${GTEST_DOWNLOAD_DIR})
    endif()

    #######################################
    # END OF GTEST DOWNLOAD
    #######################################

    # disable missing field initializers
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++ -Wno-missing-field-initializers")
    endif()

    # Prevent overriding the parent project's compiler/linker
    # settings on Windows
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

    # Add googletest directly to our build. This adds the following targets:
    # gtest, gtest_main, gmock and gmock_main
    add_subdirectory("${GTEST_DOWNLOAD_DIR}/googletest-src"
                     "${GTEST_DOWNLOAD_DIR}/googletest-build"
                     EXCLUDE_FROM_ALL)

    message(GTEST_SOURCE = ${gtest_SOURCE_DIR})
    message(GMOCK_SOURCE = ${gmock_SOURCE_DIR})

    set(GTEST_INCLUDE_DIRS "${gtest_SOURCE_DIR}/include" "${gmock_SOURCE_DIR}/include")
    set(SYWU_TEST_LIBS gtest_main )
endif()
