#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

if(NOT DEFINED CMAKE_MINIMUM_REQUIRED_VERSION)
    cmake_minimum_required(VERSION 3.18)
endif()

# canon way of using c-testrunnerswitcher from another repo is below. It assumes the using repo has placed c-testrunnerswitcher in "deps"

#if ((NOT TARGET testrunnerswitcher) AND (EXISTS ${CMAKE_CURRENT_LIST_DIR}/deps/c-testrunnerswitcher/CMakeLists.txt))
#    add_subdirectory(deps/c-testrunnerswitcher)
#    include_directories(${TESTRUNNERSWITCHER_INC_FOLDER})
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
option(use_installed_dependencies "set use_installed_dependencies to ON to use installed packages instead of building dependencies from submodules" OFF)
option(use_cppunittest "set use_cppunittest to ON to build CppUnitTest tests on Windows (default is OFF)" OFF)

#bring in dependencies
#do not add or build any tests of the dependencies
set(original_run_e2e_tests ${run_e2e_tests})
set(original_run_int_tests ${run_int_tests})
set(original_run_perf_tests ${run_perf_tests})
set(original_run_unittests ${run_unittests})
set(original_run_traceability ${run_traceability})

set(run_e2e_tests OFF)
set(run_int_tests OFF)
set(run_perf_tests OFF)
set(run_unittests OFF)
set(run_traceability OFF)

if ((NOT TARGET c_build_tools) AND (EXISTS ${CMAKE_CURRENT_LIST_DIR}/deps/c-build-tools/CMakeLists.txt))
    add_subdirectory(deps/c-build-tools)
    set_default_build_options()
endif()

if ((NOT TARGET macro_utils_c) AND (EXISTS ${CMAKE_CURRENT_LIST_DIR}/deps/macro-utils-c/CMakeLists.txt))
    add_subdirectory(deps/macro-utils-c)
    include_directories(${MACRO_UTILS_INC_FOLDER})
endif()

if ((NOT TARGET c_logging) AND (EXISTS ${CMAKE_CURRENT_LIST_DIR}/deps/c-logging/CMakeLists.txt))
    add_subdirectory(deps/c-logging)
    include_directories(deps/c-logging/inc)
endif() 

if ((NOT TARGET ctest) AND (EXISTS ${CMAKE_CURRENT_LIST_DIR}/deps/ctest/CMakeLists.txt))
    add_subdirectory(deps/ctest)
    include_directories(${CTEST_INC_FOLDER})
endif() 

set(run_e2e_tests ${original_run_e2e_tests})
set(run_int_tests ${original_run_int_tests})
set(run_perf_tests ${original_run_perf_tests})
set(run_unittests ${original_run_unittests})
set(run_traceability ${original_run_traceability})

set(testrunnerswitcher_c_files
    ./src/ctrs_sprintf.c
)

if (WIN32)
    set(testrunnerswitcher_c_files
        ./src/testmutex.c
        ${testrunnerswitcher_c_files}
    )
endif()

set(testrunnerswitcher_h_files
    ./inc/testrunnerswitcher.h
    ./inc/ctest_2_cppunittest.h
    ./inc/ctrs_sprintf.h
    ./inc/testmutex.h
    ./inc/run_cppunittest_suite.h
)

add_library(testrunnerswitcher ${testrunnerswitcher_c_files} ${testrunnerswitcher_h_files})

target_include_directories(testrunnerswitcher
    PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/inc>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

set_target_properties(testrunnerswitcher
               PROPERTIES
               FOLDER "test_tools")

#these are the include folders
#the following "set" statetement exports across the project a global variable called TESTRUNNERSWITCHER_INC_FOLDER that expands to whatever needs to included when using tesrtrunnerswitcher library
set(TESTRUNNERSWITCHER_INC_FOLDER ${CMAKE_CURRENT_LIST_DIR}/inc CACHE INTERNAL "this is what needs to be included if using testrunnerswitcher header" FORCE)

include_directories(${TESTRUNNERSWITCHER_INC_FOLDER})

# Set CMAKE_INSTALL_* if not defined
include(GNUInstallDirs)

if(NOT DEFINED CMAKE_INSTALL_LIBDIR)
    set(CMAKE_INSTALL_LIBDIR "lib")
endif()

# Install Azure Test Runner Switcher
set(package_location "cmake")

install (TARGETS testrunnerswitcher EXPORT testrunnerswitcherTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_LIBDIR}/../bin
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install (FILES ${testrunnerswitcher_h_files} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

include(CMakePackageConfigHelpers)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}/${PROJECT_NAME}ConfigVersion.cmake"
    VERSION ${TESTRUNNER_VERSION}
    COMPATIBILITY SameMajorVersion
)

configure_file("configs/${PROJECT_NAME}Config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}/${PROJECT_NAME}Config.cmake"
    COPYONLY
)

install(EXPORT testrunnerswitcherTargets
    FILE
        "${PROJECT_NAME}Targets.cmake"
    DESTINATION
        ${package_location}
)
install(
    FILES
        "configs/${PROJECT_NAME}Config.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}/${PROJECT_NAME}ConfigVersion.cmake"
    DESTINATION
        ${package_location}
)

add_subdirectory(build_functions)
add_subdirectory(test_projects)

if(${run_reals_check})
    add_reals_check_target()
endif()
