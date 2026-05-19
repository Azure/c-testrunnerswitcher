#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

# test_watchdog: per-test timeout watchdog that captures a full-memory
# minidump before killing a hung test process. Used by the build_exe
# launcher hook in c-testrunnerswitcher's build_functions when
# `use_test_watchdog` is ON.
#
# Self-contained on purpose: depends only on Windows system libraries
# (kernel32 via the CRT, dbghelp, psapi). No project libraries — this
# lets the watchdog build even when the rest of a consuming repo is
# broken, and avoids circular dependencies when wrapping the very test
# binaries it would normally link to.

if(NOT WIN32)
    return()
endif()

add_executable(test_watchdog test_watchdog.c)

# Use the wide-character entry point.
if(MSVC)
    set_target_properties(test_watchdog PROPERTIES
        LINK_FLAGS "/ENTRY:wmainCRTStartup")
endif()

target_link_libraries(test_watchdog
    dbghelp
    psapi)

set_target_properties(test_watchdog PROPERTIES
    FOLDER "test_tools")

# Disable VLD for this tool: it's a small Win32 wrapper, the VLD banner
# would pollute ctest -V output, and we explicitly want the watchdog to
# remain lean and self-contained. copy_disable_vld_ini comes from
# c-build-tools and is only defined when use_vld is ON.
if(${use_vld})
    if(COMMAND copy_disable_vld_ini)
        copy_disable_vld_ini(test_watchdog $<TARGET_FILE_DIR:test_watchdog>)
    endif()
endif()
