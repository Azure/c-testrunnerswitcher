#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

if(NOT DEFINED CMAKE_MINIMUM_REQUIRED_VERSION)
    cmake_minimum_required(VERSION 3.18)
endif()

# canon way of using c-testrunnerswitcher from another repo is below. It assumes the using repo has placed c-testrunnerswitcher in "deps"

#if ((NOT TARGET testrunnerswitcher) AND (EXISTS ${CMAKE_CURRENT_LIST_DIR}/deps/c-testrunnerswitcher/CMakeLists.txt))
#    add_subdirectory(deps/c-testrunnerswitcher)
#endif()

if(TARGET testrunnerswitcher)
    RETURN()
endif()

project(testrunnerswitcher)

set(TESTRUNNER_VERSION 1.1.19)

# Build with -fPIC always
set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)

include (CTest)

option(run_unittests "set run_unittests to ON to run unittests (default is OFF)" OFF)
option(run_int_tests "set run_int_tests to ON to run the integration tests (default is OFF)" OFF)
option(run_perf_tests "set run_perf_tests to ON to build performance tests (default is OFF)." OFF)
option(run_reals_check "set run_reals_check to ON to run reals check (default is OFF)." OFF)
option(use_installed_dependencies "set use_installed_dependencies to ON to use installed packages instead of building dependencies from submodules" OFF)
option(use_cppunittest "set use_cppunittest to ON to build CppUnitTest tests on Windows (default is OFF)" OFF)

#bring in dependencies
#do not add or build any tests of the dependencies
set(original_run_e2e_tests ${run_e2e_tests})
set(original_run_int_tests ${run_int_tests})
set(original_run_perf_tests ${run_perf_tests})
set(original_run_unittests ${run_unittests})
set(original_run_traceability ${run_traceability})
set(original_run_reals_check ${run_reals_check})

set(run_e2e_tests OFF)
set(run_int_tests OFF)
set(run_perf_tests OFF)
set(run_unittests OFF)
set(run_traceability OFF)
set(run_reals_check OFF)

if ((NOT TARGET c_build_tools) AND (EXISTS ${CMAKE_CURRENT_LIST_DIR}/deps/c-build-tools/CMakeLists.txt))
    add_subdirectory(deps/c-build-tools)
    set_default_build_options()
endif()

if ((NOT TARGET macro_utils_c) AND (EXISTS ${CMAKE_CURRENT_LIST_DIR}/deps/macro-utils-c/CMakeLists.txt))
    add_subdirectory(deps/macro-utils-c)
endif()

if ((NOT TARGET c_logging) AND (EXISTS ${CMAKE_CURRENT_LIST_DIR}/deps/c-logging/CMakeLists.txt))
    add_subdirectory(deps/c-logging)
    include_directories(deps/c-logging/v2/inc)
endif()

if ((NOT TARGET ctest) AND (EXISTS ${CMAKE_CURRENT_LIST_DIR}/deps/ctest/CMakeLists.txt))
    add_subdirectory(deps/ctest)
endif()

set(run_e2e_tests ${original_run_e2e_tests})
set(run_int_tests ${original_run_int_tests})
set(run_perf_tests ${original_run_perf_tests})
set(run_unittests ${original_run_unittests})
set(run_traceability ${original_run_traceability})
set(run_reals_check ${original_run_reals_check})

set(testrunnerswitcher_c_files
    ./src/ctrs_sprintf.c
)

if (WIN32)
    set(testrunnerswitcher_c_files
        ${testrunnerswitcher_c_files}
        ./src/testmutex.c
    )

    set(testrunnerswitcher_cpp_files
        ${testrunnerswitcher_cpp_files}
        ./src/cppunittest_mutex_fixtures.cpp
    )
endif()

set(testrunnerswitcher_h_files
    ./inc/testrunnerswitcher.h
    ./inc/cppunittest_mutex_fixtures.h
    ./inc/ctest_2_cppunittest.h
    ./inc/ctrs_sprintf.h
    ./inc/testmutex.h
    ./inc/run_cppunittest_suite.h
)

add_library(testrunnerswitcher ${testrunnerswitcher_c_files} ${testrunnerswitcher_cpp_files} ${testrunnerswitcher_h_files})

target_include_directories(testrunnerswitcher
    PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/inc>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(testrunnerswitcher c_logging_v2 ctest)

set_target_properties(testrunnerswitcher
               PROPERTIES
               FOLDER "test_tools")

add_subdirectory(build_functions)
add_subdirectory(test_projects)

if(${run_reals_check})
    add_reals_check_target()
endif()

#Insert vld in all executables if so required
add_vld_if_defined(${CMAKE_CURRENT_SOURCE_DIR})