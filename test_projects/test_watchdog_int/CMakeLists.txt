#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

# Self-tests for test_watchdog (plain Win32 exe; ctest framework not used
# here because the watchdog deliberately has no project-library deps and
# we want a self-contained pass/fail exe).
#
# Windows-only: the watchdog tool itself is Windows-only.
# Skip if test_watchdog target was not built (e.g. use_test_watchdog=OFF).

if(NOT WIN32)
    return()
endif()
if(NOT TARGET test_watchdog)
    return()
endif()

add_executable(test_watchdog_int test_watchdog_int.c)

set_target_properties(test_watchdog_int PROPERTIES
    FOLDER "test_tools")

# Land in the same directory as test_watchdog.exe so the self-test can
# resolve the watchdog at runtime via GetModuleFileNameW + path join.
set_target_properties(test_watchdog_int PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY $<TARGET_FILE_DIR:test_watchdog>)

add_dependencies(test_watchdog_int test_watchdog)

# VLD bytecode + banner would pollute test output. The watchdog itself
# also opts out via copy_disable_vld_ini; do the same here.
if(${use_vld})
    if(COMMAND copy_disable_vld_ini)
        copy_disable_vld_ini(test_watchdog_int $<TARGET_FILE_DIR:test_watchdog_int>)
    endif()
endif()

# Register the self-test with CMake's ctest. Do NOT route through the
# watchdog launcher (set TEST_LAUNCHER_DISABLED in the parent so
# build_test_folder also opts out for this subtree).
add_test(NAME test_watchdog_int
         COMMAND test_watchdog_int
         WORKING_DIRECTORY $<TARGET_FILE_DIR:test_watchdog_int>)
