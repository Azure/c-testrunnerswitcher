# Copyright (c) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE file in the project root for full license information.

#
# Timed Test Suite Validation Tests
#
# Tests the validate_timed_test_suite CMake function and underlying script.
#

# Test 1: Clean files pass validation (0 expected violations)
validate_timed_test_suite(no_violations)

# Test 2: Detection of violations (3 files with violations expected)
validate_timed_test_suite(has_violations NUM_EXPECTED_VIOLATIONS 3)

# Test 3: Skip regex causes check to be skipped
set(timed_test_suite_skip_regex "skipped_folder_int")
validate_timed_test_suite(skipped_folder_int)
if(TARGET skipped_folder_int_timed_suite_check)
    message(FATAL_ERROR "skipped_folder_int should have been skipped by regex but target was created")
endif()
unset(timed_test_suite_skip_regex)

# Master target for all timed test suite validation tests
add_custom_target(test_validate_timed_test_suite
    COMMENT "Running all timed test suite validation tests"
)

add_dependencies(test_validate_timed_test_suite
    no_violations_timed_suite_check
    has_violations_timed_suite_check
)
