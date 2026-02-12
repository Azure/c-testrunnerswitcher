# Copyright (c) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE file in the project root for full license information.

#
# Timed Test Suite Validation Tests
#
# Tests the validate_timed_test_suite.ps1 script to ensure it correctly detects
# integration test files using non-timed TEST_SUITE_INITIALIZE/CLEANUP macros.
#

set(POWERSHELL_EXECUTABLE "powershell.exe")
set(VALIDATE_TIMED_TEST_SUITE_SCRIPT "${trsw_internal_dir}/scripts/validate_timed_test_suite.ps1")

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

# Master target for all timed test suite validation tests
add_custom_target(test_validate_timed_test_suite
    COMMENT "Running all timed test suite validation tests"
)

add_dependencies(test_validate_timed_test_suite
    test_validate_timed_test_suite_clean
)

# Note: We don't add test_validate_timed_test_suite_detection as a dependency because
# it should fail with exit code 1 (which would fail the build).
# Instead, it's available to run manually to verify detection works.
