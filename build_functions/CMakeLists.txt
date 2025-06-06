#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

set(trsw_internal_dir ${CMAKE_CURRENT_LIST_DIR} CACHE INTERNAL "")

#build_lib produces a lib that can be used with an exe and/or dll for testing
#ARGN have the format
#ADDITIONAL_LIBS list of libraries. The libraries is a list consisting of repeating the following:
#   a library name
#   "debug" or "optimized" or "general" followed by a library name
#MOCK_PRECOMPILE_HEADERS is a list of headers that should be precompiled between #define ENABLE_MOCKS and #undef ENABLE_MOCKS
#NO_MOCK_PRECOMPILE_HEADERS is a list of headers that should be precompiled but before and outside of #define ENABLE_MOCKS and #undef ENABLE_MOCKS

function(build_lib whatIsBuilding solution_folder)

    add_library(${whatIsBuilding}_lib_${CMAKE_PROJECT_NAME}
        ${${whatIsBuilding}_c_files}
        ${${whatIsBuilding}_h_files}
        ${${whatIsBuilding}_cpp_files}
        ${${whatIsBuilding}_test_files}
    )

    #make sure to compile the test file as C!
    set_source_files_properties(${${whatIsBuilding}_test_files} PROPERTIES LANGUAGE C)

    set_target_properties(${whatIsBuilding}_lib_${CMAKE_PROJECT_NAME}
           PROPERTIES
           FOLDER ${solution_folder})

    target_compile_definitions(${whatIsBuilding}_lib_${CMAKE_PROJECT_NAME} PUBLIC -DUSE_CTEST)
    if(${use_cppunittest})
        target_compile_definitions(${whatIsBuilding}_lib_${CMAKE_PROJECT_NAME} PUBLIC -DCPP_UNITTEST)
    endif()
    target_compile_definitions(${whatIsBuilding}_lib_${CMAKE_PROJECT_NAME} PUBLIC -DTEST_SUITE_NAME_FROM_CMAKE=${whatIsBuilding})

    set(options) #there are no options
    set(oneValueArgs) #no single value arg
    set(multiValueArgs ADDITIONAL_LIBS MOCK_PRECOMPILE_HEADERS NO_MOCK_PRECOMPILE_HEADERS) # only ADDITIONAL_LIBS is a multi value arg

    cmake_parse_arguments("arg" "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN}) #"arg" is the prefix added to variables detected by cmake_parse_arguments

    if(arg_ADDITIONAL_LIBS)
        set(ARG_PREFIX "none") # "none" results in a link without prefix in function target_link_libraries_with_arg_prefix. Also see debug, optimized, or general keywords in https://cmake.org/cmake/help/latest/command/target_link_libraries.html
        foreach(f ${arg_ADDITIONAL_LIBS})
            if((${f} STREQUAL "debug") OR (${f} STREQUAL "optimized") OR (${f} STREQUAL "general"))
                SET(ARG_PREFIX ${f})
            else()
                target_link_libraries_with_arg_prefix(${ARG_PREFIX} ${whatIsBuilding}_lib_${CMAKE_PROJECT_NAME} ${f})
                set(ARG_PREFIX "none")
            endif()
        endforeach()
    else()
        #arg_ADDITIONAL_LIBS is not set therefore no need to do anything about libraries
    endif()

    if(arg_MOCK_PRECOMPILE_HEADERS OR arg_NO_MOCK_PRECOMPILE_HEADERS)

        string(TOUPPER ${whatIsBuilding} LIBRARY_NAME_UPPER)

        set(MOCKS_HEADER_FILE ${CMAKE_CURRENT_BINARY_DIR}/${whatIsBuilding}_mocks.h)
        set(MOCKS_SOURCE_FILE ${CMAKE_CURRENT_BINARY_DIR}/${whatIsBuilding}_mocks.c)

        set(MOCKS_HEADER_CONTENT "// Copyright (c) Microsoft. All rights reserved.\n")
        set(MOCKS_HEADER_CONTENT "${MOCKS_HEADER_CONTENT}// Licensed under the MIT license. See LICENSE file in the project root for full license information.\n")
        set(MOCKS_HEADER_CONTENT "${MOCKS_HEADER_CONTENT}// THIS IS A GENERATED FILE, DO NOT EDIT. It was generated using function build_lib from https://github.com/Azure/c-testrunnerswitcher/blob/master/build_functions/CMakeLists.txt.\n")
        set(MOCKS_HEADER_CONTENT "${MOCKS_HEADER_CONTENT}// To change this file content, edit build_test_artifacts's arguments from CMakeLists.txt\n\n")

        set(MOCKS_HEADER_CONTENT "${MOCKS_HEADER_CONTENT}#ifndef ${LIBRARY_NAME_UPPER}_MOCKS_H\n")
        set(MOCKS_HEADER_CONTENT "${MOCKS_HEADER_CONTENT}#define ${LIBRARY_NAME_UPPER}_MOCKS_H\n\n")

        foreach(HEADER_FILE ${arg_NO_MOCK_PRECOMPILE_HEADERS})
            set(MOCKS_HEADER_CONTENT "${MOCKS_HEADER_CONTENT}#include \"${HEADER_FILE}\"\n")
        endforeach()
        set(MOCKS_HEADER_CONTENT "${MOCKS_HEADER_CONTENT}\n")

        set(MOCKS_HEADER_CONTENT "${MOCKS_HEADER_CONTENT}#define ENABLE_MOCKS\n")
        set(MOCKS_HEADER_CONTENT "${MOCKS_HEADER_CONTENT}#define ENABLE_MOCKS_DECL\n")
        foreach(HEADER_FILE ${arg_MOCK_PRECOMPILE_HEADERS})
            set(MOCKS_HEADER_CONTENT "${MOCKS_HEADER_CONTENT}#include \"${HEADER_FILE}\"\n")
        endforeach()
        set(MOCKS_HEADER_CONTENT "${MOCKS_HEADER_CONTENT}#undef ENABLE_MOCKS_DECL\n")
        set(MOCKS_HEADER_CONTENT "${MOCKS_HEADER_CONTENT}#undef ENABLE_MOCKS\n\n")

        set(MOCKS_HEADER_CONTENT "${MOCKS_HEADER_CONTENT}#endif // ${LIBRARY_NAME_UPPER}_MOCKS_H\n")

        file(WRITE ${MOCKS_HEADER_FILE} ${MOCKS_HEADER_CONTENT})

        set(MOCKS_SOURCE_CONTENT "// Copyright (c) Microsoft. All rights reserved.\n")
        set(MOCKS_SOURCE_CONTENT "${MOCKS_SOURCE_CONTENT}// Licensed under the MIT license. See LICENSE file in the project root for full license information.\n")
        set(MOCKS_SOURCE_CONTENT "${MOCKS_SOURCE_CONTENT}// THIS IS A GENERATED FILE, DO NOT EDIT. It was generated using function build_lib in https://github.com/Azure/c-testrunnerswitcher/blob/master/build_functions/CMakeLists.txt.\n")
        set(MOCKS_SOURCE_CONTENT "${MOCKS_SOURCE_CONTENT}// To change this file content, edit build_test_artifacts's arguments from CMakeLists.txt\n\n")

        foreach(HEADER_FILE ${arg_NO_MOCK_PRECOMPILE_HEADERS})
            set(MOCKS_SOURCE_CONTENT "${MOCKS_SOURCE_CONTENT}#include \"${HEADER_FILE}\"\n")
        endforeach()
        set(MOCKS_SOURCE_CONTENT "${MOCKS_SOURCE_CONTENT}\n")

        set(MOCKS_SOURCE_CONTENT "${MOCKS_SOURCE_CONTENT}#define ENABLE_MOCKS\n")
        foreach(HEADER_FILE ${arg_MOCK_PRECOMPILE_HEADERS})
            set(MOCKS_SOURCE_CONTENT "${MOCKS_SOURCE_CONTENT}#include \"${HEADER_FILE}\"\n")
        endforeach()
        set(MOCKS_SOURCE_CONTENT "${MOCKS_SOURCE_CONTENT}#undef ENABLE_MOCKS\n")

        file(WRITE ${MOCKS_SOURCE_FILE} ${MOCKS_SOURCE_CONTENT})

        set_source_files_properties(${MOCKS_SOURCE_FILE} PROPERTIES SKIP_PRECOMPILE_HEADERS ON)

        target_precompile_headers(${whatIsBuilding}_lib_${CMAKE_PROJECT_NAME} PRIVATE ${MOCKS_HEADER_FILE})
        target_sources(${whatIsBuilding}_lib_${CMAKE_PROJECT_NAME} PRIVATE ${MOCKS_SOURCE_FILE})
    endif()

    if (TARGET c_logging_v2)
        target_link_libraries(${whatIsBuilding}_lib_${CMAKE_PROJECT_NAME} c_logging_v2)
    endif()
    if (TARGET umock_c)
        target_link_libraries(${whatIsBuilding}_lib_${CMAKE_PROJECT_NAME} umock_c)
    endif()
    if (TARGET ctest)
        target_link_libraries(${whatIsBuilding}_lib_${CMAKE_PROJECT_NAME} ctest)
    endif()
    if (TARGET testrunnerswitcher)
        target_link_libraries(${whatIsBuilding}_lib_${CMAKE_PROJECT_NAME} testrunnerswitcher)
    endif()

endfunction()

function(build_dll whatIsBuilding solution_folder custom_main)

    #lazily build _lib which is needed by both exe and dll
    if(NOT TARGET ${whatIsBuilding}_lib_${CMAKE_PROJECT_NAME})
        build_lib(${whatIsBuilding} ${solution_folder} ${ARGN})
    endif()

    #this is the .dll run by visual studio's test runner
    if(${custom_main})
        add_library(${whatIsBuilding}_dll_${CMAKE_PROJECT_NAME} SHARED
            main_cpp_unittest.cpp
        )
    else()
        # use the stock boilerplate main.c
        add_library(${whatIsBuilding}_dll_${CMAKE_PROJECT_NAME} SHARED
            ${trsw_internal_dir}/main_cpp_unittest.cpp
        )
    endif()

    #link with the common lib (exe an dll share the lib part)
    target_link_libraries(${whatIsBuilding}_dll_${CMAKE_PROJECT_NAME} ${whatIsBuilding}_lib_${CMAKE_PROJECT_NAME})

    target_link_directories(${whatIsBuilding}_dll_${CMAKE_PROJECT_NAME} PRIVATE $ENV{VCInstallDir}auxiliary/vs/unittest/lib $ENV{VCInstallDir}unittest/lib)

    target_include_directories(${whatIsBuilding}_dll_${CMAKE_PROJECT_NAME} PUBLIC ${sharedutil_include_directories} $ENV{VCInstallDir}auxiliary/vs/unittest/include $ENV{VCInstallDir}unittest/include)
    target_compile_definitions(${whatIsBuilding}_dll_${CMAKE_PROJECT_NAME} PUBLIC -DCPP_UNITTEST)
    if(${use_cppunittest})
        target_compile_definitions(${whatIsBuilding}_lib_${CMAKE_PROJECT_NAME} PUBLIC -DCPP_UNITTEST)
    endif()
    target_compile_definitions(${whatIsBuilding}_dll_${CMAKE_PROJECT_NAME} PUBLIC -DTEST_SUITE_NAME_FROM_CMAKE=${whatIsBuilding})
    target_compile_options(${whatIsBuilding}_dll_${CMAKE_PROJECT_NAME} PUBLIC /EHsc)

    set_target_properties(${whatIsBuilding}_dll_${CMAKE_PROJECT_NAME}
               PROPERTIES
               FOLDER ${solution_folder})

    target_compile_definitions(${whatIsBuilding}_dll_${CMAKE_PROJECT_NAME} PUBLIC -DUSE_CTEST)
    set_output_folder_properties(${whatIsBuilding}_dll_${CMAKE_PROJECT_NAME})

    if (TARGET c_logging_v2)
        target_link_libraries(${whatIsBuilding}_dll_${CMAKE_PROJECT_NAME} c_logging_v2)
    endif()
    if (TARGET umock_c)
        target_link_libraries(${whatIsBuilding}_dll_${CMAKE_PROJECT_NAME} umock_c)
    endif()
    if (TARGET ctest)
        target_link_libraries(${whatIsBuilding}_dll_${CMAKE_PROJECT_NAME} ctest)
    endif()
    if (TARGET testrunnerswitcher)
        target_link_libraries(${whatIsBuilding}_dll_${CMAKE_PROJECT_NAME} testrunnerswitcher)
    endif()

    copy_disable_vld_ini(${whatIsBuilding}_dll_${CMAKE_PROJECT_NAME} $<TARGET_FILE_DIR:${whatIsBuilding}_dll_${CMAKE_PROJECT_NAME}>)
endfunction()

function(build_exe whatIsBuilding solution_folder custom_main)

    #lazily build _lib which is needed by both exe and dll
    if(NOT TARGET ${whatIsBuilding}_lib_${CMAKE_PROJECT_NAME})
        build_lib(${whatIsBuilding} ${solution_folder} ${ARGN})
    endif()

    #this is the exe run by ctest (or directly from visual studio)
    if(${custom_main})
        add_executable(${whatIsBuilding}_exe_${CMAKE_PROJECT_NAME}
            main.c
        )
    else()
        # use the stock boilerplate main.c
        add_executable(${whatIsBuilding}_exe_${CMAKE_PROJECT_NAME}
            ${trsw_internal_dir}/main_ctest.c
        )
    endif()

    #link with the common lib (exe an dll share the lib part)
    target_link_libraries(${whatIsBuilding}_exe_${CMAKE_PROJECT_NAME} ${whatIsBuilding}_lib_${CMAKE_PROJECT_NAME})

    set_target_properties(${whatIsBuilding}_exe_${CMAKE_PROJECT_NAME}
               PROPERTIES
               FOLDER ${solution_folder})

    set_output_folder_properties(${whatIsBuilding}_exe_${CMAKE_PROJECT_NAME})

    target_compile_definitions(${whatIsBuilding}_exe_${CMAKE_PROJECT_NAME} PUBLIC -DUSE_CTEST)
    if(${use_cppunittest})
        target_compile_definitions(${whatIsBuilding}_lib_${CMAKE_PROJECT_NAME} PUBLIC -DCPP_UNITTEST)
    endif()
    target_compile_definitions(${whatIsBuilding}_exe_${CMAKE_PROJECT_NAME} PUBLIC -DTEST_SUITE_NAME_FROM_CMAKE=${whatIsBuilding})
    target_include_directories(${whatIsBuilding}_exe_${CMAKE_PROJECT_NAME} PUBLIC ${sharedutil_include_directories})

    if (TARGET c_logging_v2)
        target_link_libraries(${whatIsBuilding}_exe_${CMAKE_PROJECT_NAME} c_logging_v2)
    endif()
    if (TARGET umock_c)
        target_link_libraries(${whatIsBuilding}_exe_${CMAKE_PROJECT_NAME} umock_c)
    endif()
    if (TARGET ctest)
        target_link_libraries(${whatIsBuilding}_exe_${CMAKE_PROJECT_NAME} ctest)
    endif()
    if (TARGET testrunnerswitcher)
        target_link_libraries(${whatIsBuilding}_exe_${CMAKE_PROJECT_NAME} testrunnerswitcher)
    endif()

    #register the exe with ctest's list of tests
    add_test(NAME ${whatIsBuilding} COMMAND ${whatIsBuilding}_exe_${CMAKE_PROJECT_NAME})

    if(UNIX) #LINUX OR APPLE
        if(${run_valgrind} OR ${run_helgrind} OR ${run_drd})
            find_program(VALGRIND_FOUND NAMES valgrind)
            if(${VALGRIND_FOUND} STREQUAL VALGRIND_FOUND-NOTFOUND)
                message(WARNING "Running with run_valgrind/run_helgrind/run_drd, but valgrind was not found - there will be no tests run under valgrind")
            else()
                if(${run_valgrind})
                    # some tests really create more than 500 threads and valgrind/helgrind does not like that (it thinks the impossible happened :-))
                    add_test(NAME ${whatIsBuilding}_valgrind COMMAND valgrind                 --gen-suppressions=all --num-callers=100 --error-exitcode=1 --leak-check=full --track-origins=yes --max-threads=3000 ${VALGRIND_SUPPRESSIONS_FILE_EXTRA_PARAMETER} --suppressions=${trsw_internal_dir}/common_suppresions.sup $<TARGET_FILE:${whatIsBuilding}_exe_${CMAKE_PROJECT_NAME}>)
                endif()
                if(${run_helgrind})
                    # some tests really create more than 500 threads and valgrind/helgrind does not like that (it thinks the impossible happened :-))
                    add_test(NAME ${whatIsBuilding}_helgrind COMMAND valgrind --tool=helgrind --gen-suppressions=all --num-callers=100 --error-exitcode=1 --max-threads=3000 ${VALGRIND_SUPPRESSIONS_FILE_EXTRA_PARAMETER} --suppressions=${trsw_internal_dir}/common_suppresions.sup $<TARGET_FILE:${whatIsBuilding}_exe_${CMAKE_PROJECT_NAME}>)
                endif()
                if(${run_drd})
                    add_test(NAME ${whatIsBuilding}_drd      COMMAND valgrind --tool=drd      --gen-suppressions=all --num-callers=100 --error-exitcode=1 ${VALGRIND_SUPPRESSIONS_FILE_EXTRA_PARAMETER} --suppressions=${trsw_internal_dir}/common_suppresions.sup $<TARGET_FILE:${whatIsBuilding}_exe_${CMAKE_PROJECT_NAME}>)
                endif()
            endif()
        endif()
    endif()
endfunction()

#drop in replacement for add_subdirectory :)
#test_folder is the name of a folder that exists on the drive. For example "clds_hash_table_ut". It is not a complete path.
function(build_test_folder test_folder)

    if(
        (("${test_folder}" MATCHES ".*ut.*") AND ${run_unittests}) OR
        (("${test_folder}" MATCHES ".*e2e.*") AND ${run_e2e_tests}) OR
        (("${test_folder}" MATCHES ".*int.*") AND ${run_int_tests}) OR
        (("${test_folder}" MATCHES ".*perf.*") AND ${run_perf_tests})
    )

        set(options) #there are no options
        set(oneValueArgs BINARY_DIR) #there's single value arg (maybe)
        set(multiValueArgs) #no multi value args

        cmake_parse_arguments("arg" "${options}" "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN}) #"arg" is the prefix added to variables detected by cmake_parse_arguments

        set(building exe)
        add_subdirectory(${test_folder} ${test_folder}/${arg_BINARY_DIR}/${building})

        if ((${use_cppunittest}) AND (WIN32))
            set(building dll)
            add_subdirectory(${test_folder} ${test_folder}/${arg_BINARY_DIR}/${building})
        endif()
    else()
        #message("test_folder is ${test_folder} NOT BUILDING")
    endif()

endfunction()

#this is the function used to add all the needed targets for one test project
#(invoked from the CMakeLists where the test suite lives)
# Note: the test folder has to be added by using build_test_folder, not add_subdirectory!
function(build_test_artifacts whatIsBuilding solution_folder)
    if(${building} STREQUAL "lib")
        build_lib(${whatIsBuilding} ${solution_folder} ${ARGN})
    elseif(${building} STREQUAL "dll")
        build_dll(${whatIsBuilding} ${solution_folder} OFF ${ARGN})
    elseif(${building} STREQUAL "exe")
        build_exe(${whatIsBuilding} ${solution_folder} OFF ${ARGN})
    else()
        MESSAGE(FATAL_ERROR "not an expected value: building=${building} test_folder=${test_folder} ARGN=${ARGN}")
    endif()
endfunction()

#this is the function used to add all the needed targets for one test project where main.c is custom
#(invoked from the CMakeLists where the test suite lives)
# Note: the test folder has to be added by using build_test_folder, not add_subdirectory!
# Note: main.c is expected to live in the test folder
function(build_test_artifacts_with_custom_main whatIsBuilding solution_folder )
    if(${building} STREQUAL "lib")
        build_lib(${whatIsBuilding} ${solution_folder} ${ARGN})
    elseif(${building} STREQUAL "dll")
        build_dll(${whatIsBuilding} ${solution_folder} ON ${ARGN})
    elseif(${building} STREQUAL "exe")
        build_exe(${whatIsBuilding} ${solution_folder} ON ${ARGN})
    else()
        MESSAGE(FATAL_ERROR "not an expected value: building=${building} test_folder=${test_folder} ARGN=${ARGN}")
    endif()
endfunction()
