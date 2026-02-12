# Copyright (c) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE file in the project root for full license information.

#
# Timed Test Suite Validation Tests
#
# Tests the validate_timed_test_suite.ps1 script to ensure it correctly detects
# integration test files using non-timed TEST_SUITE_INITIALIZE/CLEANUP macros
# and validates the fix functionality works properly.
#

set(POWERSHELL_EXECUTABLE "powershell.exe")
set(VALIDATE_TIMED_TEST_SUITE_SCRIPT "${trsw_internal_dir}/validate_timed_test_suite.ps1")

# Test 1: Verify script detects non-timed macros in files that have them
add_custom_target(test_validate_timed_test_suite_detection
    COMMAND ${POWERSHELL_EXECUTABLE} -ExecutionPolicy Bypass -File "${VALIDATE_TIMED_TEST_SUITE_SCRIPT}"
        -TestFolder "${CMAKE_CURRENT_SOURCE_DIR}/has_violations"
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Testing timed test suite detection on files with non-timed macros"
)

# Test 2: Verify script passes on files already using timed macros
add_custom_target(test_validate_timed_test_suite_clean
    COMMAND ${POWERSHELL_EXECUTABLE} -ExecutionPolicy Bypass -File "${VALIDATE_TIMED_TEST_SUITE_SCRIPT}"
        -TestFolder "${CMAKE_CURRENT_SOURCE_DIR}/no_violations"
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Testing timed test suite validation on clean files"
)

# Test 3: Verify fix functionality works (copy test files to temp directory first)
add_custom_target(test_validate_timed_test_suite_fix
    # Create a temporary directory and copy test files
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/temp_fix_test"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/has_violations"
        "${CMAKE_CURRENT_BINARY_DIR}/temp_fix_test"
    # Run the script with fix mode
    COMMAND ${POWERSHELL_EXECUTABLE} -ExecutionPolicy Bypass -File "${VALIDATE_TIMED_TEST_SUITE_SCRIPT}"
        -TestFolder "${CMAKE_CURRENT_BINARY_DIR}/temp_fix_test"
        -Fix
    # Verify violations were fixed (script should pass now)
    COMMAND ${POWERSHELL_EXECUTABLE} -ExecutionPolicy Bypass -File "${VALIDATE_TIMED_TEST_SUITE_SCRIPT}"
        -TestFolder "${CMAKE_CURRENT_BINARY_DIR}/temp_fix_test"
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Testing timed test suite fix functionality"
)

# Test 4: Verify fix correctly handles multi-arg macros
add_custom_target(test_validate_timed_test_suite_fix_multi_arg
    # Create a temporary directory and copy the multi-arg test file
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/temp_multi_arg_test"
    COMMAND ${CMAKE_COMMAND} -E copy
        "${CMAKE_CURRENT_SOURCE_DIR}/has_violations/multi_arg_int.c"
        "${CMAKE_CURRENT_BINARY_DIR}/temp_multi_arg_test/"
    # Run the script with fix mode
    COMMAND ${POWERSHELL_EXECUTABLE} -ExecutionPolicy Bypass -File "${VALIDATE_TIMED_TEST_SUITE_SCRIPT}"
        -TestFolder "${CMAKE_CURRENT_BINARY_DIR}/temp_multi_arg_test"
        -Fix
    # Compare the result with the expected file
    COMMAND ${CMAKE_COMMAND} -E compare_files
        "${CMAKE_CURRENT_BINARY_DIR}/temp_multi_arg_test/multi_arg_int.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/has_violations/multi_arg_int_expected.c"
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Testing that multi-arg macros are fixed correctly"
)

# Test 5: Verify fix correctly handles basic case (compare_files for sample_module_int)
add_custom_target(test_validate_timed_test_suite_fix_basic
    # Create a temporary directory and copy the basic test file
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/temp_basic_test"
    COMMAND ${CMAKE_COMMAND} -E copy
        "${CMAKE_CURRENT_SOURCE_DIR}/has_violations/sample_module_int.c"
        "${CMAKE_CURRENT_BINARY_DIR}/temp_basic_test/"
    # Run the script with fix mode
    COMMAND ${POWERSHELL_EXECUTABLE} -ExecutionPolicy Bypass -File "${VALIDATE_TIMED_TEST_SUITE_SCRIPT}"
        -TestFolder "${CMAKE_CURRENT_BINARY_DIR}/temp_basic_test"
        -Fix
    # Compare the result with the expected file
    COMMAND ${CMAKE_COMMAND} -E compare_files
        "${CMAKE_CURRENT_BINARY_DIR}/temp_basic_test/sample_module_int.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/has_violations/sample_module_int_expected.c"
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Testing that basic macros are fixed correctly and include is added"
)

# Test 6: Verify fix does not duplicate timed_test_suite.h include when already present
add_custom_target(test_validate_timed_test_suite_fix_no_duplicate_include
    # Create a temporary directory and copy the file that already has the include
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/temp_no_dup_include_test"
    COMMAND ${CMAKE_COMMAND} -E copy
        "${CMAKE_CURRENT_SOURCE_DIR}/has_violations/has_include_int.c"
        "${CMAKE_CURRENT_BINARY_DIR}/temp_no_dup_include_test/"
    # Run the script with fix mode
    COMMAND ${POWERSHELL_EXECUTABLE} -ExecutionPolicy Bypass -File "${VALIDATE_TIMED_TEST_SUITE_SCRIPT}"
        -TestFolder "${CMAKE_CURRENT_BINARY_DIR}/temp_no_dup_include_test"
        -Fix
    # Compare the result with the expected file (should NOT have duplicate include)
    COMMAND ${CMAKE_COMMAND} -E compare_files
        "${CMAKE_CURRENT_BINARY_DIR}/temp_no_dup_include_test/has_include_int.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/has_violations/has_include_int_expected.c"
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Testing that fix does not duplicate timed_test_suite.h include"
)

# Master target for all timed test suite validation tests
add_custom_target(test_validate_timed_test_suite
    COMMENT "Running all timed test suite validation tests"
)

# Add dependencies - detection test should fail (exit code 1), clean test should pass
add_dependencies(test_validate_timed_test_suite
    test_validate_timed_test_suite_clean
    test_validate_timed_test_suite_fix
    test_validate_timed_test_suite_fix_multi_arg
    test_validate_timed_test_suite_fix_basic
    test_validate_timed_test_suite_fix_no_duplicate_include
)

# Note: We don't add test_validate_timed_test_suite_detection as a dependency because
# it should fail with exit code 1 (which would fail the build).
# Instead, it's available to run manually to verify detection works.
