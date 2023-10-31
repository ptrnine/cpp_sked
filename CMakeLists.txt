cmake_minimum_required(VERSION 3.27)

project(cpp_sked)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

option(BUILD_TESTS "Build tests" ON)
option(BUILD_EXAMPLES "Build EXAMPLES" ON)
option(USE_SYSTEM_3RD "Use system third-party libraries for the tests" ON)

if (BUILD_TESTS)
    if (NOT USE_SYSTEM_3RD)
        set(CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH};${CMAKE_BINARY_DIR}/3rd/lib64/cmake")
        set(CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH};${CMAKE_BINARY_DIR}/3rd/lib/cmake")
        include(cmake/git_submodule_build.cmake)

        git_submodule_update()
        git_submodule_build(Catch2)
    endif()

    find_package(Catch2 REQUIRED)
    add_subdirectory(tests)
endif()

if (BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

