#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

set(trw_internal_dir ${CMAKE_CURRENT_LIST_DIR} CACHE INTERNAL "")

function(build_lib whatIsBuilding solution_folder)

    add_library(${whatIsBuilding}_lib
        ${${whatIsBuilding}_c_files}
        ${${whatIsBuilding}_h_files}
        ${${whatIsBuilding}_cpp_files}
        ${${whatIsBuilding}_test_files}
    )

    #make sure to compile the test file as C!
    set_source_files_properties(${${whatIsBuilding}_test_files} PROPERTIES LANGUAGE C)

    set_target_properties(${whatIsBuilding}_lib
           PROPERTIES
           FOLDER ${solution_folder})

    target_compile_definitions(${whatIsBuilding}_lib PUBLIC -DUSE_CTEST)
    target_compile_definitions(${whatIsBuilding}_lib PUBLIC -DTEST_SUITE_NAME_FROM_CMAKE=${whatIsBuilding})

    set(PARSING_ADDITIONAL_LIBS OFF)
    set(PARSING_VALGRIND_SUPPRESSIONS_FILE OFF)
    set(VALGRIND_SUPPRESSIONS_FILE_EXTRA_PARAMETER)
    set(ARG_PREFIX "none")
    foreach(f ${ARGN})
        set(skip_to_next FALSE)
        if(${f} STREQUAL "ADDITIONAL_LIBS")
            SET(PARSING_ADDITIONAL_LIBS ON)
            SET(PARSING_VALGRIND_SUPPRESSIONS_FILE OFF)
            set(ARG_PREFIX "none")
            #also unset all the other states
            set(skip_to_next TRUE)
        elseif(${f} STREQUAL "VALGRIND_SUPPRESSIONS_FILE")
            SET(PARSING_ADDITIONAL_LIBS OFF)
            SET(PARSING_VALGRIND_SUPPRESSIONS_FILE ON)
            set(skip_to_next TRUE)
        endif()

        if(NOT skip_to_next)
            if(PARSING_ADDITIONAL_LIBS)
                if((${f} STREQUAL "debug") OR (${f} STREQUAL "optimized") OR (${f} STREQUAL "general"))
                    SET(ARG_PREFIX ${f})
                else()
                    target_link_libraries_with_arg_prefix(${ARG_PREFIX} ${whatIsBuilding}_lib ${f})
                    set(ARG_PREFIX "none")
                endif()
            endif()
        endif()

    endforeach()
endfunction()

function(build_dll whatIsBuilding solution_folder)

    #lazily build _lib which is needed by both exe and dll
    if(NOT TARGET ${whatIsBuilding}_lib)
        build_lib(${whatIsBuilding} ${solution_folder} ${ARGN})
    endif()

    #this is the .dll run by visual studio's test runner
    add_library(${whatIsBuilding}_dll SHARED
        ${trw_internal_dir}/main_cpp_unittest.cpp
    )

    #link with the common lib (exe an dll share the lib part)
    target_link_libraries(${whatIsBuilding}_dll ${whatIsBuilding}_lib)

    target_link_directories(${whatIsBuilding}_dll PRIVATE $ENV{VCInstallDir}auxiliary/vs/unittest/lib $ENV{VCInstallDir}unittest/lib)

    target_include_directories(${whatIsBuilding}_dll PUBLIC ${sharedutil_include_directories} $ENV{VCInstallDir}auxiliary/vs/unittest/include $ENV{VCInstallDir}unittest/include)
    target_compile_definitions(${whatIsBuilding}_dll PUBLIC -DCPP_UNITTEST)
    target_compile_definitions(${whatIsBuilding}_dll PUBLIC -DTEST_SUITE_NAME_FROM_CMAKE=${whatIsBuilding})
    target_compile_options(${whatIsBuilding}_dll PUBLIC /EHsc)

    set_target_properties(${whatIsBuilding}_dll
               PROPERTIES
               FOLDER ${solution_folder})

    target_compile_definitions(${whatIsBuilding}_dll PUBLIC -DUSE_CTEST)
    set_output_folder_properties(${whatIsBuilding}_dll)

    if (TARGET c_logging)
        target_link_libraries(${whatIsBuilding}_dll c_logging)
    endif()
    if (TARGET umock_c)
        target_link_libraries(${whatIsBuilding}_dll umock_c)
    endif()
    if (TARGET ctest)
        target_link_libraries(${whatIsBuilding}_dll ctest)
    endif()
    if (TARGET testrunnerswitcher)
        target_link_libraries(${whatIsBuilding}_dll testrunnerswitcher)
    endif()

    copy_disable_vld_ini(${whatIsBuilding}_dll $<TARGET_FILE_DIR:${whatIsBuilding}_dll>)
endfunction()

function(build_exe whatIsBuilding solution_folder)

    #lazily build _lib which is needed by both exe and dll
    if(NOT TARGET ${whatIsBuilding}_lib)
        build_lib(${whatIsBuilding} ${solution_folder} ${ARGN})
    endif()

    #this is the exe run by ctest (or directly from visual studio)
    add_executable(${whatIsBuilding}_exe
        main.c
    )

    #link with the common lib (exe an dll share the lib part)
    target_link_libraries(${whatIsBuilding}_exe ${whatIsBuilding}_lib)

    set_target_properties(${whatIsBuilding}_exe
               PROPERTIES
               FOLDER ${solution_folder})

    set_output_folder_properties(${whatIsBuilding}_exe)

    target_compile_definitions(${whatIsBuilding}_exe PUBLIC -DUSE_CTEST)
    target_compile_definitions(${whatIsBuilding}_exe PUBLIC -DTEST_SUITE_NAME_FROM_CMAKE=${whatIsBuilding})
    target_include_directories(${whatIsBuilding}_exe PUBLIC ${sharedutil_include_directories})

    if (TARGET c_logging)
        target_link_libraries(${whatIsBuilding}_exe c_logging)
    endif()
    if (TARGET umock_c)
        target_link_libraries(${whatIsBuilding}_exe umock_c)
    endif()
    if (TARGET ctest)
        target_link_libraries(${whatIsBuilding}_exe ctest)
    endif()
    if (TARGET testrunnerswitcher)
        target_link_libraries(${whatIsBuilding}_exe testrunnerswitcher)
    endif()

    #register the exe with ctest's list of tests
    add_test(NAME ${whatIsBuilding} COMMAND ${whatIsBuilding}_exe)

    if(UNIX) #LINUX OR APPLE
        if(${run_valgrind} OR ${run_helgrind} OR ${run_drd})
            find_program(VALGRIND_FOUND NAMES valgrind)
            if(${VALGRIND_FOUND} STREQUAL VALGRIND_FOUND-NOTFOUND)
                message(WARNING "Running with run_valgrind/run_helgrind/run_drd, but valgrind was not found - there will be no tests run under valgrind")
            else()
                if(${run_valgrind})
                    add_test(NAME ${whatIsBuilding}_valgrind COMMAND valgrind                 --gen-suppressions=all --num-callers=100 --error-exitcode=1 --leak-check=full --track-origins=yes ${VALGRIND_SUPPRESSIONS_FILE_EXTRA_PARAMETER} --suppressions=${trw_internal_dir}/common_suppresions.sup $<TARGET_FILE:${whatIsBuilding}_exe>)
                endif()
                if(${run_helgrind})
                    add_test(NAME ${whatIsBuilding}_helgrind COMMAND valgrind --tool=helgrind --gen-suppressions=all --num-callers=100 --error-exitcode=1 ${VALGRIND_SUPPRESSIONS_FILE_EXTRA_PARAMETER} --suppressions=${trw_internal_dir}/common_suppresions.sup $<TARGET_FILE:${whatIsBuilding}_exe>)
                endif()
                if(${run_drd})
                    add_test(NAME ${whatIsBuilding}_drd      COMMAND valgrind --tool=drd      --gen-suppressions=all --num-callers=100 --error-exitcode=1 ${VALGRIND_SUPPRESSIONS_FILE_EXTRA_PARAMETER} --suppressions=${trw_internal_dir}/common_suppresions.sup $<TARGET_FILE:${whatIsBuilding}_exe>)
                endif()
            endif()
        endif()
    endif()
endfunction()

#drop in replacement for add_subdirectory :)
function(build_test_folder test_folder)

    if(
        (("${test_folder}" MATCHES ".*ut.*") AND ${run_unittests}) OR
        (("${test_folder}" MATCHES ".*e2e.*") AND ${run_e2e_tests}) OR
        (("${test_folder}" MATCHES ".*int.*") AND ${run_int_tests}) OR
        (("${test_folder}" MATCHES ".*perf.*") AND ${run_perf_tests})
    )

        set(BINARY_DIR) #BINARY_DIR is added to every target (because targets need to have different names).
        set(PARSING_BINARY_DIR OFF) #see https://cmake.org/cmake/help/latest/command/add_subdirectory.html for "binary_dir"

        foreach(f ${ARGN})
            if(${f} STREQUAL "BINARY_DIR")
                set(PARSING_BINARY_DIR ON)
            elseif(PARSING_BINARY_DIR)
                set(BINARY_DIR ${f})
                set(PARSING_BINARY_DIR OFF)
            endif()

        endforeach()

        set(building exe)
        add_subdirectory(${test_folder} ${test_folder}/${BINARY_DIR}/${building})

        if ((${use_cppunittest}) AND (WIN32))
            set(building dll)
            add_subdirectory(${test_folder} ${test_folder}/${BINARY_DIR}/${building})
        endif()
    else()
        #message("test_folder is ${test_folder} NOT BUILDING")
    endif()

endfunction()

#this is the function used to add all the needed targets for one test project
#(invoked from the CMakeLists where the test suite lives)
# Note: the test folder has to be added by using build_test_folder, not add_subdirectory!
function(build_test_artifacts whatIsBuilding use_gballoc solution_folder )
    if(${use_gballoc})
        add_definitions(-DGB_MEASURE_MEMORY_FOR_THIS -DGB_DEBUG_ALLOC)
    endif()

    if(${building} STREQUAL "lib")
        build_lib(${whatIsBuilding} ${solution_folder} ${ARGN})
    elseif(${building} STREQUAL "dll")
        build_dll(${whatIsBuilding} ${solution_folder} ${ARGN})
    elseif(${building} STREQUAL "exe")
        build_exe(${whatIsBuilding} ${solution_folder} ${ARGN})
    else()
        MESSAGE(FATAL_ERROR "not an expected value: building=${building} test_folder=${test_folder} ARGN=${ARGN}")
    endif()
endfunction()