#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

build_test_folder(test_project_ut)
build_test_folder(test_project_int)
build_test_folder(test_project_e2e)
build_test_folder(test_project_perf)
build_test_folder(test_project_with_custom_main_ut)
build_test_folder(ctest_2_cppunittest_ut)
build_test_folder(parameterized_tests_ut)

# Self-tests for the test_watchdog launcher (plain Win32 exe, not a
# c-testrunnerswitcher test suite). Must NOT be wrapped by the watchdog
# itself — that would cause infinite recursion if a self-test timed out.
# TEST_LAUNCHER_DISABLED prevents build_test_folder from injecting the
# launcher inside this subtree; the test_watchdog_int subdir doesn't go
# through build_test_folder anyway (it uses plain add_test) but setting
# the variable here is harmless and self-documenting.
if(WIN32 AND ${use_test_watchdog} AND ${run_int_tests})
    set(TEST_LAUNCHER_DISABLED ON)
    add_subdirectory(test_watchdog_int)
endif()
